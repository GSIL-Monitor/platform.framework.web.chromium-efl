// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_media_player_private_tizen.h"

#include <Elementary.h>
#include <emeCDM/IEME.h>
#include <unistd.h>

#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop/message_loop.h"
#include "content/browser/renderer_host/pepper/media_player/buffering_listener_private.h"
#include "content/browser/renderer_host/pepper/media_player/drm_listener_private.h"
#include "content/browser/renderer_host/pepper/media_player/media_events_listener_private.h"
#include "content/browser/renderer_host/pepper/media_player/pepper_media_data_source_private.h"
#include "content/browser/renderer_host/pepper/media_player/subtitle_listener_private.h"
#include "content/browser/renderer_host/pepper/media_player/tizen/platform_context_tizen.h"
#include "ewk/efl_integration/common/content_switches_efl.h"
#include "media/base/tizen/media_player_efl.h"
#include "ppapi/c/samsung/pp_media_common_samsung.h"

namespace content {

namespace {

// MediaEventsListener::OnTimeUpdate() event will be sent to a plugin every
// kTimeUpdatePeriod seconds.
static const double kTimeUpdatePeriod = 0.5;  // in seconds

class DRMListener : public PepperTizenDRMListener {
 public:
  static scoped_refptr<PepperTizenDRMListener> Create(
      base::WeakPtr<PepperMediaPlayerPrivateTizen> player) {
    return scoped_refptr<PepperTizenDRMListener>{new DRMListener(player)};
  }

  virtual ~DRMListener() = default;

  void OnLicenseRequest(uint32_t size, const void* request) override {
    auto request_begin = reinterpret_cast<const uint8_t*>(request);
    auto request_end = request_begin + size;
    io_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&PepperMediaPlayerPrivateTizen::OnLicenseRequest,
                   media_player_,
                   std::vector<uint8_t>{request_begin, request_end}));
  }

 private:
  explicit DRMListener(base::WeakPtr<PepperMediaPlayerPrivateTizen> player)
      : media_player_(std::move(player)),
        io_task_runner_(base::MessageLoop::current()->task_runner()) {}

  base::WeakPtr<PepperMediaPlayerPrivateTizen> media_player_;

  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(DRMListener);
};

int32_t GetAudioTrackListHelper(PepperPlayerAdapterInterface* player,
                                std::vector<PP_AudioTrackInfo>* result) {
  return player->GetAudioTracksList(result);
}

int32_t GetTextTrackListHelper(PepperPlayerAdapterInterface* player,
                               std::vector<PP_TextTrackInfo>* result) {
  return player->GetTextTracksList(result);
}

int32_t GetVideoTrackListHelper(PepperPlayerAdapterInterface* player,
                                std::vector<PP_VideoTrackInfo>* result) {
  return player->GetVideoTracksList(result);
}

template <typename TrackType>
struct TrackTypeTraits {
  static const PP_ElementaryStream_Type_Samsung kPlayerStreamType =
      PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_UNKNOWN;
};

template <>
struct TrackTypeTraits<PP_VideoTrackInfo> {
  static const PP_ElementaryStream_Type_Samsung kPlayerStreamType =
      PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_VIDEO;
  static int32_t GetTrackList(PepperPlayerAdapterInterface* player,
                              std::vector<PP_VideoTrackInfo>* result) {
    return GetVideoTrackListHelper(player, result);
  }
};

template <>
struct TrackTypeTraits<PP_AudioTrackInfo> {
  static const PP_ElementaryStream_Type_Samsung kPlayerStreamType =
      PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_AUDIO;
  static int32_t GetTrackList(PepperPlayerAdapterInterface* player,
                              std::vector<PP_AudioTrackInfo>* result) {
    return GetAudioTrackListHelper(player, result);
  }
};

template <>
struct TrackTypeTraits<PP_TextTrackInfo> {
  static const PP_ElementaryStream_Type_Samsung kPlayerStreamType =
      PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_TEXT;
  static int32_t GetTrackList(PepperPlayerAdapterInterface* player,
                              std::vector<PP_TextTrackInfo>* result) {
    return GetTextTrackListHelper(player, result);
  }
};

template <typename TrackType>
int32_t GetCurrentTrackInfoOnPlayerThread(PepperPlayerAdapterInterface* player,
                                          TrackType* info) {
  typedef TrackTypeTraits<TrackType> TrackTraits;
  static_assert(TrackTraits::kPlayerStreamType !=
                    PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_UNKNOWN,
                "Invalid TrackType!");
  memset(info, 0, sizeof(TrackType));

  int32_t cur_index = 0;
  int32_t error_code =
      player->GetCurrentTrack(TrackTraits::kPlayerStreamType, &cur_index);

  if (error_code != PP_OK)
    return error_code;

  std::vector<TrackType> result;
  int32_t pp_error_code = TrackTraits::GetTrackList(player, &result);

  if (pp_error_code == PP_OK) {
    for (auto i : result) {
      if (static_cast<int>(i.index) == cur_index) {
        *info = i;
        return PP_OK;
      }
    }
  }
  LOG(ERROR) << "Current track ID == " << cur_index << " is not present on"
             << " a track list (track list size == " << result.size() << ")";
  return PP_ERROR_FAILED;
}

int32_t SetDisplayRectOnPlayerThread(const PP_Rect& display_rect,
                                     const PP_FloatRect& crop_ratio_rect,
                                     Evas_Object* video_window,
                                     PepperPlayerAdapterInterface* player) {
  int32_t error_code = PP_OK;
#if defined(TIZEN_VIDEO_HOLE)
  if (media::MediaPlayerEfl::is_video_window()) {
    evas_object_move(video_window, display_rect.point.x, display_rect.point.y);
    evas_object_resize(video_window, display_rect.size.width,
                       display_rect.size.height);
    return error_code;
  }
#endif
  error_code = player->SetDisplayRect(display_rect, crop_ratio_rect);

  return error_code;
}

void OnTimeNeedsUpdateOnPlayerThread(PepperPlayerAdapterInterface* player) {
  PP_MediaPlayerState state = PP_MEDIAPLAYERSTATE_NONE;
  if (player->GetPlayerState(&state) != PP_OK)
    return;
  if (state == PP_MEDIAPLAYERSTATE_READY ||
      state == PP_MEDIAPLAYERSTATE_PLAYING)
    player->NotifyCurrentTimeListener();
}

}  // namespace

// static
scoped_refptr<PepperMediaPlayerPrivateTizen>
PepperMediaPlayerPrivateTizen::Create(
    std::unique_ptr<PepperPlayerAdapterInterface> player) {
  return new PepperMediaPlayerPrivateTizen(std::move(player));
}

PepperMediaPlayerPrivateTizen::PepperMediaPlayerPrivateTizen(
    std::unique_ptr<PepperPlayerAdapterInterface> player)
    : dispatcher_(PepperTizenPlayerDispatcher::Create(std::move(player))),
      main_cb_runner_(base::MessageLoop::current()->task_runner()),
      display_mode_(PP_MEDIAPLAYERDISPLAYMODE_STRETCH),
      current_time_(-1.0),
      weak_ptr_factory_(this),
      drm_manager_(DRMListener::Create(weak_ptr_factory_.GetWeakPtr())),
      video_window_(nullptr) {
#if defined(TIZEN_VIDEO_HOLE)
  auto parent_window = media::MediaPlayerEfl::main_window_handle();
  if (media::MediaPlayerEfl::is_video_window()) {
    // Does not set transient relation(child window).
    video_window_ = elm_win_add(nullptr, "PepperVideo", ELM_WIN_BASIC);
    if (!video_window_) {
      LOG(ERROR) << "createVideoWindow: video window is null";
      return;
    }

    elm_win_title_set(video_window_, "PepperVideo");
    elm_win_borderless_set(video_window_, EINA_TRUE);

    if (parent_window)
      elm_win_alpha_set(static_cast<Elm_Win*>(parent_window), true);
    elm_win_alpha_set(video_window_, true);

    Evas_Object* bg = evas_object_rectangle_add(video_window_);
    evas_object_color_set(bg, 0, 0, 0, 0);

    elm_win_aux_hint_add(video_window_, "wm.policy.win.user.geometry", "1");
  }
#endif
  if (dispatcher_ && dispatcher_->player()) {
    dispatcher_->player()->SetErrorCallback(
        base::Bind(&PepperMediaPlayerPrivateTizen::OnErrorEvent,
                   weak_ptr_factory_.GetWeakPtr()));
    dispatcher_->player()->SetDisplayRectUpdateCallback(
        base::Bind(static_cast<void (PepperMediaPlayerPrivateTizen::*)()>(
                       &PepperMediaPlayerPrivateTizen::UpdateDisplayRect),
                   weak_ptr_factory_.GetWeakPtr()));
  }
}

PepperMediaPlayerPrivateTizen::~PepperMediaPlayerPrivateTizen() {
  InvalidatePlayer();
}

void PepperMediaPlayerPrivateTizen::InvalidatePlayer() {
  StopCurrentTimeNotifier();

  if (data_source_) {
    data_source_->SourceDetached(
        PepperMediaDataSourcePrivate::PlatformDetachPolicy::kRetainPlayer);
    data_source_->GetContext().set_dispatcher(nullptr);
    data_source_->GetContext().set_drm_manager(nullptr);
  }

  drm_listener_ = nullptr;

  // Clean dispatcher (and delete platform player)
  // before deleting this object as we might get events from the player
  dispatcher_.reset();

  if (video_window_)
    evas_object_del(video_window_);
  video_window_ = nullptr;
}

void PepperMediaPlayerPrivateTizen::AbortCallbacks(int32_t reason) {
  ExecuteCallback(reason, &attach_callback_);
  ExecuteCallback(reason, &seek_callback_);
  ExecuteCallback(reason, &stop_callback_);
}

void PepperMediaPlayerPrivateTizen::StartCurrentTimeNotifier() {
  if (!time_update_timer_.IsRunning()) {
    time_update_timer_.Start(
        FROM_HERE, base::TimeDelta::FromSecondsD(kTimeUpdatePeriod),
        base::Bind(&PepperMediaPlayerPrivateTizen::OnTimeNeedsUpdate,
                   weak_ptr_factory_.GetWeakPtr()));
  }
}

void PepperMediaPlayerPrivateTizen::StopCurrentTimeNotifier() {
  if (time_update_timer_.IsRunning())
    time_update_timer_.Stop();
}

void PepperMediaPlayerPrivateTizen::ExecuteCallback(
    int32_t result,
    base::Callback<void(int32_t)>* callback,
    const char* on_cb_empty_message) {
  DCHECK(main_cb_runner_->BelongsToCurrentThread());
  if (callback->is_null()) {
    if (on_cb_empty_message)
      LOG(WARNING) << on_cb_empty_message;
    return;
  }
  base::Callback<void(int32_t)> original_callback = *callback;
  callback->Reset();
  original_callback.Run(result);
}

void PepperMediaPlayerPrivateTizen::SetMediaEventsListener(
    std::unique_ptr<MediaEventsListenerPrivate> media_events_listener) {
  if (!dispatcher_)
    return;

  // TODO(p.balut): event listeners probably can be held by unique_ptr and this
  //                code can be simplified in consequence
  bool will_set = !!media_events_listener;
  PepperTizenPlayerDispatcher::DispatchOperation(
      dispatcher_.get(), FROM_HERE,
      base::Bind(&PepperPlayerAdapterInterface::SetMediaEventsListener),
      // Ownership is assigned again in SetMediaEventsListener method
      media_events_listener.release());

  if (will_set) {
    // We don't want to send current time notifications before a data source is
    // attached.
    if (data_source_)
      StartCurrentTimeNotifier();
  } else {
    StopCurrentTimeNotifier();
  }
}

void PepperMediaPlayerPrivateTizen::SetDRMListener(
    std::unique_ptr<DRMListenerPrivate> drm_listener) {
  drm_listener_ = std::move(drm_listener);
}

void PepperMediaPlayerPrivateTizen::SetSubtitleListener(
    std::unique_ptr<SubtitleListenerPrivate> subtitle_listener) {
  if (!dispatcher_)
    return;
  PepperTizenPlayerDispatcher::DispatchOperation(
      dispatcher_.get(), FROM_HERE,
      base::Bind(&PepperPlayerAdapterInterface::SetSubtitleListener),
      // Ownership is assigned again in SetSubtitleListener method
      subtitle_listener.release());
}

void PepperMediaPlayerPrivateTizen::SetBufferingListener(
    std::unique_ptr<BufferingListenerPrivate> buffering_listener) {
  if (!dispatcher_)
    return;
  PepperTizenPlayerDispatcher::DispatchOperation(
      dispatcher_.get(), FROM_HERE,
      base::Bind(&PepperPlayerAdapterInterface::SetBufferingListener),
      // Ownership is assigned again in SetBufferingListener method
      buffering_listener.release());
}

void PepperMediaPlayerPrivateTizen::AttachDataSource(
    const scoped_refptr<PepperMediaDataSourcePrivate>& data_source,
    const base::Callback<void(int32_t)>& callback) {
  if (!attach_callback_.is_null()) {
    base::MessageLoop::current()->task_runner()->PostTask(
        FROM_HERE,
        base::Bind(callback, static_cast<int32_t>(PP_ERROR_INPROGRESS)));
    return;
  }

  if (!dispatcher_) {
    base::MessageLoop::current()->task_runner()->PostTask(
        FROM_HERE, base::Bind(callback, static_cast<int32_t>(PP_ERROR_FAILED)));
    return;
  }

  if (data_source_)
    DetachDataSource();

  // we need to reinitialize display and set display mode before each attach
  PepperTizenPlayerDispatcher::DispatchOperation(
      dispatcher_.get(), FROM_HERE,
      base::Bind(
          &PepperMediaPlayerPrivateTizen::InitializeDisplayOnPlayerThread,
          base::Unretained(this)));
  PepperTizenPlayerDispatcher::DispatchOperation(
      dispatcher_.get(), FROM_HERE,
      base::Bind(
          base::IgnoreResult(&PepperPlayerAdapterInterface::SetDisplayMode)),
      display_mode_);

  attach_callback_ = callback;
  data_source_ = std::move(data_source);
  data_source_->GetContext().set_dispatcher(dispatcher_.get());
  data_source_->GetContext().set_drm_manager(&drm_manager_);
  data_source_->SourceAttached(
      base::Bind(&PepperMediaPlayerPrivateTizen::OnDataSourceAttached,
                 weak_ptr_factory_.GetWeakPtr()));
}

void PepperMediaPlayerPrivateTizen::DetachDataSource() {
  StopCurrentTimeNotifier();
  auto previous_data_source = std::move(data_source_);
  previous_data_source->SourceDetached();
  previous_data_source->GetContext().set_dispatcher(nullptr);
  previous_data_source->GetContext().set_drm_manager(nullptr);
}

void PepperMediaPlayerPrivateTizen::OnDataSourceAttached(int32_t result) {
  if (result != PP_OK) {
    DetachDataSource();

    ExecuteCallback(result, &attach_callback_,
                    "An error ocurred when attaching data source, "
                    "but a callback for this operation is not "
                    "registered.");
    return;
  }

  UpdateDisplayRect();
  // Once data source is attached, current time notifications should start.
  StartCurrentTimeNotifier();

  ExecuteCallback(result, &attach_callback_,
                  "Source has been attached, but a "
                  "callback for this operation is not "
                  "registered.");
}

void PepperMediaPlayerPrivateTizen::Play(
    const base::Callback<void(int32_t)>& callback) {
  PepperTizenPlayerDispatcher::DispatchOperation(
      dispatcher_.get(), FROM_HERE,
      base::Bind(&PepperPlayerAdapterInterface::Play), callback);
}

void PepperMediaPlayerPrivateTizen::Pause(
    const base::Callback<void(int32_t)>& callback) {
  PepperTizenPlayerDispatcher::DispatchOperation(
      dispatcher_.get(), FROM_HERE,
      base::Bind(&PepperPlayerAdapterInterface::Pause), callback);
}

void PepperMediaPlayerPrivateTizen::Stop(
    const base::Callback<void(int32_t)>& callback) {
  if (!stop_callback_.is_null()) {
    base::MessageLoop::current()->task_runner()->PostTask(
        FROM_HERE,
        base::Bind(callback, static_cast<int32_t>(PP_ERROR_INPROGRESS)));
    return;
  }

  stop_callback_ = callback;

  PepperTizenPlayerDispatcher::DispatchAsyncOperation(
      dispatcher_.get(), FROM_HERE,
      base::Bind(&PepperPlayerAdapterInterface::Stop),
      base::Bind(&PepperMediaPlayerPrivateTizen::OnStoppedReply,
                 weak_ptr_factory_.GetWeakPtr()));
}

void PepperMediaPlayerPrivateTizen::OnStoppedReply(int32_t result) {
  base::Callback<void(int32_t)> callback = stop_callback_;
  stop_callback_.Reset();
  if (!callback.is_null())
    callback.Run(result);
  else
    LOG(WARNING) << "Stop has completed, but a callback for this operation "
                    "is not registred.";
}

void PepperMediaPlayerPrivateTizen::Seek(
    PP_TimeTicks time,
    const base::Callback<void(int32_t)>& callback) {
  if (FailIfNotAttached(FROM_HERE, callback))
    return;

  if (!seek_callback_.is_null()) {
    base::MessageLoop::current()->task_runner()->PostTask(
        FROM_HERE,
        base::Bind(callback, static_cast<int32_t>(PP_ERROR_INPROGRESS)));
    return;
  }

  seek_callback_ = callback;
  PepperTizenPlayerDispatcher::DispatchOperation(
      dispatcher_.get(), FROM_HERE,
      base::Bind(&PepperMediaPlayerPrivateTizen::SeekOnPlayerThread,
                 base::Unretained(this), time));
  PepperTizenPlayerDispatcher::DispatchOperationOnAppendThread(
      dispatcher_.get(), FROM_HERE,
      base::Bind(&PepperMediaPlayerPrivateTizen::SeekOnAppendThread,
                 base::Unretained(this), time));
}

void PepperMediaPlayerPrivateTizen::SeekOnAppendThread(
    PP_TimeTicks time,
    PepperPlayerAdapterInterface* player) {
  player->SeekOnAppendThread(time);
}

void PepperMediaPlayerPrivateTizen::SeekOnPlayerThread(
    PP_TimeTicks time,
    PepperPlayerAdapterInterface* player) {
  DCHECK(dispatcher_->IsRunningOnPlayerThread());

  int32_t error_code = player->Seek(
      time, base::Bind(&PepperMediaPlayerPrivateTizen::OnSeekCompleted,
                       weak_ptr_factory_.GetWeakPtr()));
  if (error_code != PP_OK && !seek_callback_.is_null()) {
    base::Callback<void(int32_t)> callback = seek_callback_;
    seek_callback_.Reset();
    main_cb_runner_->PostTask(FROM_HERE, base::Bind(callback, error_code));
  }
}

void PepperMediaPlayerPrivateTizen::OnSeekCompleted() {
  base::Callback<void(int32_t)> callback = seek_callback_;
  seek_callback_.Reset();

  if (!callback.is_null()) {
    main_cb_runner_->PostTask(FROM_HERE, base::Bind(callback, PP_OK));
  } else {
    LOG(WARNING) << "Seek has completed, but a callback for this operation is "
                    "not registered.";
  }
}

void PepperMediaPlayerPrivateTizen::SetPlaybackRate(
    double rate,
    const base::Callback<void(int32_t)>& callback) {
  PepperTizenPlayerDispatcher::DispatchOperation(
      dispatcher_.get(), FROM_HERE,
      base::Bind(&PepperPlayerAdapterInterface::SetPlaybackRate), callback,
      static_cast<float>(rate));
}

void PepperMediaPlayerPrivateTizen::GetDuration(
    const base::Callback<void(int32_t, PP_TimeDelta)>& callback) {
  if (FailIfNotAttached(FROM_HERE, callback, static_cast<PP_TimeDelta>(0)))
    return;

  data_source_->GetDuration(callback);
}

void PepperMediaPlayerPrivateTizen::GetCurrentTime(
    const base::Callback<void(int32_t, PP_TimeTicks)>& callback) {
  PepperTizenPlayerDispatcher::DispatchOperationWithResult(
      dispatcher_.get(), FROM_HERE,
      base::Bind(&PepperPlayerAdapterInterface::GetCurrentTime), callback);
}

void PepperMediaPlayerPrivateTizen::GetPlayerState(
    const base::Callback<void(int32_t, PP_MediaPlayerState)>& callback) {
  PepperTizenPlayerDispatcher::DispatchOperationWithResult(
      dispatcher_.get(), FROM_HERE,
      base::Bind(&PepperPlayerAdapterInterface::GetPlayerState), callback);
}

void PepperMediaPlayerPrivateTizen::GetVideoTracksList(
    const base::Callback<void(int32_t, const std::vector<PP_VideoTrackInfo>&)>&
        callback) {
  PepperTizenPlayerDispatcher::DispatchOperationWithResult(
      dispatcher_.get(), FROM_HERE,
      // base::Bind needed to properly select overload
      base::Bind(&GetVideoTrackListHelper), callback);
}

void PepperMediaPlayerPrivateTizen::GetCurrentVideoTrackInfo(
    const base::Callback<void(int32_t, const PP_VideoTrackInfo&)>& callback) {
  PepperTizenPlayerDispatcher::DispatchOperationWithResult(
      dispatcher_.get(), FROM_HERE,
      // base::Bind needed to properly select overload
      base::Bind(&GetCurrentTrackInfoOnPlayerThread<PP_VideoTrackInfo>),
      callback);
}

void PepperMediaPlayerPrivateTizen::GetAudioTracksList(
    const base::Callback<void(int32_t, const std::vector<PP_AudioTrackInfo>&)>&
        callback) {
  PepperTizenPlayerDispatcher::DispatchOperationWithResult(
      dispatcher_.get(), FROM_HERE,
      // base::Bind needed to properly select overload
      base::Bind(&GetAudioTrackListHelper), callback);
}

void PepperMediaPlayerPrivateTizen::GetCurrentAudioTrackInfo(
    const base::Callback<void(int32_t, const PP_AudioTrackInfo&)>& callback) {
  PepperTizenPlayerDispatcher::DispatchOperationWithResult(
      dispatcher_.get(), FROM_HERE,
      // base::Bind needed to properly select overload
      base::Bind(&GetCurrentTrackInfoOnPlayerThread<PP_AudioTrackInfo>),
      callback);
}

void PepperMediaPlayerPrivateTizen::GetTextTracksList(
    const base::Callback<void(int32_t, const std::vector<PP_TextTrackInfo>&)>&
        callback) {
  PepperTizenPlayerDispatcher::DispatchOperationWithResult(
      dispatcher_.get(), FROM_HERE,
      // base::Bind needed to properly select overload
      base::Bind(&GetTextTrackListHelper), callback);
}

void PepperMediaPlayerPrivateTizen::GetCurrentTextTrackInfo(
    const base::Callback<void(int32_t, const PP_TextTrackInfo&)>& callback) {
  PepperTizenPlayerDispatcher::DispatchOperationWithResult(
      dispatcher_.get(), FROM_HERE,
      // base::Bind needed to properly select overload
      base::Bind(&GetCurrentTrackInfoOnPlayerThread<PP_TextTrackInfo>),
      callback);
}

void PepperMediaPlayerPrivateTizen::SelectTrack(
    PP_ElementaryStream_Type_Samsung pp_type,
    uint32_t track_index,
    const base::Callback<void(int32_t)>& callback) {
  PepperTizenPlayerDispatcher::DispatchOperation(
      dispatcher_.get(), FROM_HERE,
      base::Bind(&PepperPlayerAdapterInterface::SelectTrack), callback, pp_type,
      track_index);
}

void PepperMediaPlayerPrivateTizen::AddExternalSubtitles(
    const std::string& file_path,
    const std::string& encoding,
    const base::Callback<void(int32_t, const PP_TextTrackInfo&)>& callback) {
  PepperTizenPlayerDispatcher::DispatchOperationWithResult(
      dispatcher_.get(), FROM_HERE,
      base::Bind(
          &PepperMediaPlayerPrivateTizen::SetExternalSubtitlePathOnPlayerThread,
          base::Unretained(this), file_path, encoding),
      callback);
}

void PepperMediaPlayerPrivateTizen::SetSubtitlesDelay(
    PP_TimeDelta delay,
    const base::Callback<void(int32_t)>& callback) {
  PepperTizenPlayerDispatcher::DispatchOperation(
      dispatcher_.get(), FROM_HERE,
      base::Bind(&PepperPlayerAdapterInterface::SetSubtitlesDelay), callback,
      delay);
}

void PepperMediaPlayerPrivateTizen::SetDisplayRect(
    const PP_Rect& display_rect,
    const PP_FloatRect& crop_ratio_rect,
    const base::Callback<void(int32_t)>& callback) {
  display_rect_ = display_rect;
  crop_ratio_rect_ = crop_ratio_rect;
  if (data_source_) {
    UpdateDisplayRect(callback);
  } else if (!callback.is_null()) {
    base::MessageLoop::current()->task_runner()->PostTask(
        FROM_HERE, base::Bind(callback, static_cast<int32_t>(PP_OK)));
  }
}

void PepperMediaPlayerPrivateTizen::SetDisplayMode(
    PP_MediaPlayerDisplayMode display_mode,
    const base::Callback<void(int32_t)>& callback) {
  display_mode_ = display_mode;
  if (data_source_) {
    PepperTizenPlayerDispatcher::DispatchOperation(
        dispatcher_.get(), FROM_HERE,
        base::Bind(&PepperPlayerAdapterInterface::SetDisplayMode), callback,
        display_mode);
  } else if (!callback.is_null()) {
    auto is_display_mode_supported =
        dispatcher_->player()->IsDisplayModeSupported(display_mode);
    auto result = (is_display_mode_supported
                       ? static_cast<int32_t>(PP_OK)
                       : static_cast<int32_t>(PP_ERROR_NOTSUPPORTED));
    base::MessageLoop::current()->task_runner()->PostTask(
        FROM_HERE, base::Bind(callback, result));
  }
}

void PepperMediaPlayerPrivateTizen::SetVr360Mode(
    PP_MediaPlayerVr360Mode vr360_mode,
    const base::Callback<void(int32_t)>& callback) {
  if (!data_source_) {
    PepperTizenPlayerDispatcher::DispatchOperation(
        dispatcher_.get(), FROM_HERE,
        base::Bind(&PepperPlayerAdapterInterface::SetVr360Mode), callback,
        vr360_mode);
  } else if (!callback.is_null()) {
    base::MessageLoop::current()->task_runner()->PostTask(
        FROM_HERE, base::Bind(callback, static_cast<int32_t>(PP_ERROR_FAILED)));
  }
}

void PepperMediaPlayerPrivateTizen::SetVr360Rotation(
    float horizontal_angle,
    float vertical_angle,
    const base::Callback<void(int32_t)>& callback) {
  if (data_source_) {
    PepperTizenPlayerDispatcher::DispatchOperation(
        dispatcher_.get(), FROM_HERE,
        base::Bind(&PepperPlayerAdapterInterface::SetVr360Rotation), callback,
        horizontal_angle, vertical_angle);
  } else if (!callback.is_null()) {
    base::MessageLoop::current()->task_runner()->PostTask(
        FROM_HERE, base::Bind(callback, static_cast<int32_t>(PP_ERROR_FAILED)));
  }
}

void PepperMediaPlayerPrivateTizen::SetVr360ZoomLevel(
    uint32_t zoom_level,
    const base::Callback<void(int32_t)>& callback) {
  if (data_source_) {
    PepperTizenPlayerDispatcher::DispatchOperation(
        dispatcher_.get(), FROM_HERE,
        base::Bind(&PepperPlayerAdapterInterface::SetVr360ZoomLevel), callback,
        zoom_level);
  } else if (!callback.is_null()) {
    base::MessageLoop::current()->task_runner()->PostTask(
        FROM_HERE, base::Bind(callback, static_cast<int32_t>(PP_ERROR_FAILED)));
  }
}

void PepperMediaPlayerPrivateTizen::UpdateDisplayRect(
    const base::Callback<void(int32_t)>& callback) {
  if (callback) {
    PepperTizenPlayerDispatcher::DispatchOperation(
        dispatcher_.get(), FROM_HERE,
        base::Bind(&SetDisplayRectOnPlayerThread, display_rect_,
                   crop_ratio_rect_, video_window_),
        callback);
  } else {
    PepperTizenPlayerDispatcher::DispatchOperation(
        dispatcher_.get(), FROM_HERE,
        base::Bind(base::IgnoreResult(&SetDisplayRectOnPlayerThread),
                   display_rect_, crop_ratio_rect_, video_window_));
  }
}

void PepperMediaPlayerPrivateTizen::UpdateDisplayRect() {
  PepperTizenPlayerDispatcher::DispatchOperation(
      dispatcher_.get(), FROM_HERE,
      base::Bind(base::IgnoreResult(&SetDisplayRectOnPlayerThread),
                 display_rect_, crop_ratio_rect_, video_window_));
}

void PepperMediaPlayerPrivateTizen::SetDRMSpecificData(
    PP_MediaPlayerDRMType pp_drm_type,
    PP_MediaPlayerDRMOperation pp_drm_operation,
    const void* raw_data,
    size_t data_size,
    const base::Callback<void(int32_t)>& callback) {
  int32_t result = PP_ERROR_FAILED;
  switch (pp_drm_operation) {
    case PP_MEDIAPLAYERDRMOPERATION_INSTALLLICENSE: {
      auto data = std::string{static_cast<const char*>(raw_data), data_size};
      result = drm_manager_.InstallLicense(data) ? PP_OK : PP_ERROR_FAILED;
      break;
    }
    // TODO(p.balut): Add support for other DRM operations.
    case PP_MEDIAPLAYERDRMOPERATION_SETPROPERTIES:
    case PP_MEDIAPLAYERDRMOPERATION_GENCHALLENGE:
    case PP_MEDIAPLAYERDRMOPERATION_DELETELICENSE:
    case PP_MEDIAPLAYERDRMOPERATION_PROCESSINITIATOR:
    case PP_MEDIAPLAYERDRMOPERATION_GETVERSION:
      result = PP_ERROR_NOTSUPPORTED;
      break;
    default:
      result = PP_ERROR_BADARGUMENT;
  }

  base::MessageLoop::current()->task_runner()->PostTask(
      FROM_HERE, base::Bind(callback, result));
}

void PepperMediaPlayerPrivateTizen::OnLicenseRequest(
    const std::vector<uint8_t>& request) {
  if (drm_listener_)
    drm_listener_->OnLicenseRequest(request);
}

void PepperMediaPlayerPrivateTizen::InitializeDisplayOnPlayerThread(
    PepperPlayerAdapterInterface* player) {
  int error_code = PP_OK;
  std::string tizen_app_id =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kTizenAppId);
  if (!tizen_app_id.empty()) {
    error_code = player->SetApplicationID(tizen_app_id);
    if (error_code != PP_OK)
      LOG(WARNING) << "Failed to set app_id error_code: " << error_code;
  }
#if defined(TIZEN_VIDEO_HOLE)
  if (media::MediaPlayerEfl::is_video_window()) {
    evas_object_resize(video_window_, 1, 1);
    evas_object_move(video_window_, 0, 0);

    evas_object_show(video_window_);
    elm_win_raise(static_cast<Elm_Win*>(video_window_));

    auto parent_window = media::MediaPlayerEfl::main_window_handle();
    if (parent_window)
      elm_win_raise(static_cast<Elm_Win*>(parent_window));

    player->SetDisplay(static_cast<void*>(video_window_), false);
  } else {
    player->SetDisplay(
        static_cast<void*>(media::MediaPlayerEfl::main_window_handle()), true);
  }
#endif
}

int32_t PepperMediaPlayerPrivateTizen::SetExternalSubtitlePathOnPlayerThread(
    const std::string& path,
    const std::string& encoding,
    PepperPlayerAdapterInterface* player,
    PP_TextTrackInfo* result_info) {
  *result_info = PP_TextTrackInfo{};

  int32_t player_error = player->SetExternalSubtitlesPath(path, encoding);
  if (player_error != PP_OK) {
    LOG(ERROR) << "Setting external subtitles failed with error: "
               << player_error;
    return player_error;
  }

  // Language information cannot be obtained from external subtitles in srt
  // format.
  // Language is set to "un" - unknown, it's consistent with the platform
  // player naming.
  *result_info = PP_TextTrackInfo{0, PP_TRUE, "un"};
  return player_error;
}

void PepperMediaPlayerPrivateTizen::OnErrorEvent(
    PP_MediaPlayerError error_code) {
  main_cb_runner_->PostTask(
      FROM_HERE, base::Bind(&PepperMediaPlayerPrivateTizen::AbortCallbacks,
                            weak_ptr_factory_.GetWeakPtr(),
                            error_code == PP_MEDIAPLAYERERROR_RESOURCE
                                ? PP_ERROR_RESOURCE_FAILED
                                : PP_ERROR_ABORTED));
  if (error_code == PP_MEDIAPLAYERERROR_RESOURCE) {
    main_cb_runner_->PostTask(
        FROM_HERE, base::Bind(&PepperMediaPlayerPrivateTizen::InvalidatePlayer,
                              weak_ptr_factory_.GetWeakPtr()));
  }
}

void PepperMediaPlayerPrivateTizen::OnTimeNeedsUpdate() {
  PepperTizenPlayerDispatcher::DispatchOperation(
      dispatcher_.get(), FROM_HERE,
      base::Bind(&OnTimeNeedsUpdateOnPlayerThread));
}

// NOLINTNEXTLINE(runtime/int)
void PepperMediaPlayerPrivateTizen::OnDRMEvent(int /* event_type */,
                                               void* /* msg_data */) {
  // TODO(p.balut): fixme:
  // - Poor documentation.
  // - Need to figure out what are DRM event types and message data.
}

}  // namespace content
