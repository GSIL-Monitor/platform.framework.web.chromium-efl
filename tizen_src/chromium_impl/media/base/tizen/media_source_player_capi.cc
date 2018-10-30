// Copyright 2015 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "media/base/tizen/media_source_player_capi.h"

#include <player_internal.h>
#include <map>

#include "base/memory/ptr_util.h"
#include "base/process/process.h"
#include "base/threading/thread_task_runner_handle.h"
#include "media/base/tizen/demuxer_efl.h"
#include "media/base/tizen/media_player_manager_efl.h"
#include "media/base/tizen/media_player_util_efl.h"
#include "third_party/libyuv/include/libyuv.h"

#if defined(OS_TIZEN_TV_PRODUCT)
#include <capi-system-info/system_info.h>
#include "base/command_line.h"
#include "ewk/efl_integration/common/content_switches_efl.h"
#include "malloc.h"
#endif

#if defined(TIZEN_TBM_SUPPORT) && defined(OS_TIZEN_TV_PRODUCT)
#include <Elementary.h>
#include <Evas.h>
#include "ui/gl/egl_util.h"
#endif

#if defined(TIZEN_SOUND_FOCUS)
#include "media/base/tizen/sound_focus_manager.h"
#endif

#if defined(TIZEN_VIDEO_HOLE)
#include "media/base/tizen/video_plane_manager_tizen.h"
#endif

namespace {
// Update interval for current time.
const base::TimeDelta kCurrentTimeUpdateInterval =
    base::TimeDelta::FromMilliseconds(100);

// Update interval for state change confirmation.
const base::TimeDelta kStateChangeConfirmationInterval =
    base::TimeDelta::FromMilliseconds(10);

// Initial buffering time.
const base::TimeDelta kBufferingTime = base::TimeDelta::FromSeconds(5);

// Buffering level in percent.
const int kUnderrunBufferThreshold = 1;
const int kMinBufferThreshold = 30;
const int kMaxBufferThreshold = 80;
const int kOverflowBufferThreshold = 95;
const int kMediaStreamBufferMinThreshold = 100;

#if defined(OS_TIZEN_TV_PRODUCT)
const int kVideoFramerateDen = 100;
const int kVideoFramerateNum = 2997;
const int kFHDVideoMaxWidth = 1920;
const int kFHDVideoMaxHeight = 1080;
const int k4KVideoMaxWidth = 3840;
const int k4KVideoMaxHeight = 2160;
const int kMaxFramerate = 60;
#endif

#define ENUM_CASE(x) \
  case x:            \
    return #x;       \
    break

const char* GetString(media::MediaSourcePlayerCapi::BufferStatus status) {
  switch (status) {
    ENUM_CASE(media::MediaSourcePlayerCapi::BUFFER_NONE);
    ENUM_CASE(media::MediaSourcePlayerCapi::BUFFER_UNDERRUN);
    ENUM_CASE(media::MediaSourcePlayerCapi::BUFFER_MIN_THRESHOLD);
    ENUM_CASE(media::MediaSourcePlayerCapi::BUFFER_NORMAL);
    ENUM_CASE(media::MediaSourcePlayerCapi::BUFFER_MAX_THRESHOLD);
    ENUM_CASE(media::MediaSourcePlayerCapi::BUFFER_OVERFLOW);
    ENUM_CASE(media::MediaSourcePlayerCapi::BUFFER_EOS);
  };
  NOTREACHED() << "Invalid MediaSourcePlayerCapi::BufferStatus!"
               << " (code=" << status << ")";
  return "";
}
#undef ENUM_CASE

size_t DemuxStreamToChannelIndex(media::DemuxerStream::Type type) {
  switch (type) {
    case media::DemuxerStream::AUDIO:
      return 0u;
    case media::DemuxerStream::VIDEO:
      return 1u;
    default:
      NOTREACHED() << "Stream type "
                   << media::DemuxerStream::ConvertTypeToString(type)
                   << " is not supported";
      return -1u;
  }
}

}  // namespace

using base::Callback;

namespace std {
std::ostream& operator<<(std::ostream& out, const media::MediaPacket& packet) {
  return out << packet.ToString();
}
}  // namespace std

namespace media {

class BufferObserverImpl : public BufferObserver {
 public:
  typedef std::multimap<int, base::Closure, std::greater<int>> BufferingTasks;

  BufferObserverImpl()
      : task_runner_(base::ThreadTaskRunnerHandle::Get()),
        weak_factory_(this) {}
  void SetBufferSize(DemuxerStream::Type type, player_buffer_size_t size) {
    GetBuffer(type).buffer_size = size;
  }

  // |handler| will be invoked from capi-player's thread.
  void SetBufferingCallback(const BufferingCallback& handler) override {
    buffering_callback_ = handler;
  }

  void EOSReached(DemuxerStream::Type type) override {
    if (GetBuffer(type).last_buffer_status !=
        MediaSourcePlayerCapi::BUFFER_EOS) {
      CallbackIfNeed(type, MediaSourcePlayerCapi::BUFFER_EOS);
    }
  }

  void UpdateBufferFill(DemuxerStream::Type type,
                        player_buffer_size_t bytes) override {
    auto& buffer = GetBuffer(type);
    DCHECK(buffer.buffer_size != 0);

    if (buffer.last_buffer_status == MediaSourcePlayerCapi::BUFFER_EOS ||
        !buffer.buffer_size)
      return;

#ifndef NDEBUG
    const auto old_percent = buffer.buffer_percent;
#endif
    // return 1% as long as there is something in the buffer
    buffer.buffer_percent = (bytes * 100 / buffer.buffer_size) ?: (bytes > 0);

#ifndef NDEBUG
    DLOG_IF(INFO, buffer.buffer_percent != old_percent)
        << "Current Buffer Level: VIDEO: "
        << GetBuffer(DemuxerStream::VIDEO).buffer_percent << "%"
        << "\tAUDIO: " << GetBuffer(DemuxerStream::AUDIO).buffer_percent << "%";
#endif

    MediaSourcePlayerCapi::BufferStatus buffer_status =
        GetBufferStatus(buffer.buffer_percent);
    if (buffer.last_buffer_status != buffer_status) {
      CallbackIfNeed(type, buffer_status);
    }
  }

  static void OnVideoBufferedSizeChanged(player_media_stream_buffer_status_e,
                                         player_buffer_size_t bytes,
                                         void* user_data) {
    static_cast<BufferObserver*>(user_data)->UpdateBufferFill(
        DemuxerStream::VIDEO, bytes);
  }

  static void OnAudioBufferedSizeChanged(player_media_stream_buffer_status_e,
                                         player_buffer_size_t bytes,
                                         void* user_data) {
    static_cast<BufferObserver*>(user_data)->UpdateBufferFill(
        DemuxerStream::AUDIO, bytes);
  }

  void ResetBufferStatus() override {
    for (auto& buffer : buffers_) {
      buffer.last_buffer_status = MediaSourcePlayerCapi::BUFFER_NONE;
    }
  }

  int SetMediaStreamStatusAudioCallback(player_h player) override {
    auto ret = player_set_media_stream_buffer_min_threshold(
        player, PLAYER_STREAM_TYPE_AUDIO, kMediaStreamBufferMinThreshold);

    if (ret != PLAYER_ERROR_NONE) {
      LOG(WARNING) << "player_set_media_stream_buffer_min_threshold: Error: "
                   << ret;
    }
    return player_set_media_stream_buffer_status_cb_ex(
        player, PLAYER_STREAM_TYPE_AUDIO,
        BufferObserverImpl::OnAudioBufferedSizeChanged, this);
  }

  int SetMediaStreamStatusVideoCallback(player_h player) override {
    auto ret = player_set_media_stream_buffer_min_threshold(
        player, PLAYER_STREAM_TYPE_VIDEO, kMediaStreamBufferMinThreshold);

    if (ret != PLAYER_ERROR_NONE) {
      LOG(WARNING)
          << "player_set_media_stream_buffer_min_threshold:video Error: "
          << ret;
    }

    return player_set_media_stream_buffer_status_cb_ex(
        player, PLAYER_STREAM_TYPE_VIDEO,
        BufferObserverImpl::OnVideoBufferedSizeChanged, this);
  }

  MediaSourcePlayerCapi::BufferStatus AudioStatus() const override {
    return GetBuffer(DemuxerStream::AUDIO).last_buffer_status;
  }
  MediaSourcePlayerCapi::BufferStatus VideoStatus() const override {
    return GetBuffer(DemuxerStream::VIDEO).last_buffer_status;
  }

 private:
  void CallbackIfNeed(DemuxerStream::Type type,
                      MediaSourcePlayerCapi::BufferStatus status) {
    if (buffering_callback_.is_null()) {
      LOG(WARNING) << "buffering_callback_ is null, return";
      return;
    }

    // If invoked from other threads, relay to current task_runner.
    if (!task_runner_->BelongsToCurrentThread()) {
      task_runner_->PostTask(
          FROM_HERE, base::Bind(&BufferObserverImpl::CallbackIfNeed,
                                weak_factory_.GetWeakPtr(), type, status));
      return;
    }

    auto& buffer = GetBuffer(type);
    if (buffer.last_buffer_status != status) {
      buffer.last_buffer_status = status;
      buffering_callback_.Run(type, buffer.last_buffer_status);
    }
  }

  static MediaSourcePlayerCapi::BufferStatus GetBufferStatus(
      int buffered_percent) {
    if (buffered_percent < kUnderrunBufferThreshold) {
      return MediaSourcePlayerCapi::BUFFER_UNDERRUN;
    } else if (buffered_percent < kMinBufferThreshold) {
      return MediaSourcePlayerCapi::BUFFER_MIN_THRESHOLD;
    } else if (buffered_percent > kOverflowBufferThreshold) {
      return MediaSourcePlayerCapi::BUFFER_OVERFLOW;
    } else if (buffered_percent > kMaxBufferThreshold) {
      return MediaSourcePlayerCapi::BUFFER_MAX_THRESHOLD;
    }
    return MediaSourcePlayerCapi::BUFFER_NORMAL;
  }

  struct BufferDescriptor {
    BufferDescriptor()
        : buffer_size(0),
          buffer_percent(0),
          last_buffer_status(MediaSourcePlayerCapi::BUFFER_NONE) {}
    player_buffer_size_t buffer_size;
    int buffer_percent;
    MediaSourcePlayerCapi::BufferStatus last_buffer_status;
  };

  BufferDescriptor& GetBuffer(DemuxerStream::Type type) {
    return buffers_[DemuxStreamToChannelIndex(type)];
  }

  const BufferDescriptor& GetBuffer(DemuxerStream::Type type) const {
    return buffers_[DemuxStreamToChannelIndex(type)];
  }

  std::array<BufferDescriptor, kCapiPlayerChannelCount> buffers_;
  BufferingCallback buffering_callback_;
  const scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  base::WeakPtrFactory<BufferObserverImpl> weak_factory_;
};

void MediaSourcePlayerCapi::HandlePrepareComplete() {
  const auto state = GetPlayerState();
  if (state != PLAYER_STATE_READY) {
    LOG(WARNING) << "During prepare there was a state change, current is "
                 << GetString(state) << " returning...";
    return;
  }
  LOG(INFO) << "Preroll completed, Signal \"ready to play\"";
  player_prepared_ = true;
#if defined(TIZEN_VIDEO_HOLE)
  OnWebViewMoved();
#endif

  UncheckedUpdateReadyState();
  manager()->OnNetworkStateChange(GetPlayerId(),
                                  blink::WebMediaPlayer::kNetworkStateLoaded);
  OnCurrentTimeUpdateTimerFired();
  if (pending_seek_ == true) {
    pending_seek_ = false;

#if defined(OS_TIZEN_TV_PRODUCT)
    auto DoSeek = [&]() {
      const auto current_time = GetCurrentTime();
      const auto duration = 2 * CalculateLastFrameDuration();
      if (seek_state_ != MEDIA_SEEK_DEMUXER_DONE)
        return false;
      if (((current_time - duration) <= pending_seek_offset_) &&
          ((current_time + duration) >= pending_seek_offset_))
        return false;
      if (video_key_frame_ != media::kNoTimestamp &&
          video_key_frame_ == current_time)
        return false;
      return true;
    };

    if (seek_state_ != MEDIA_SEEK_PLAYER) {
      if (DoSeek()) {
        seek_offset_ = pending_seek_offset_;
        SeekInternal();
      } else
        SeekDone();
    }
#else
    manager()->OnRequestSeek(GetPlayerId(), pending_seek_offset_);
#endif
  }

  RunCompleteCB(true, FROM_HERE);
}

void MediaSourcePlayerCapi::RunCompleteCB(bool success,
                                          const base::Location& from) {
  if (!success)
    LOG(ERROR) << " is called from : " << from.function_name();

  if (!complete_callback_.is_null()) {
    complete_callback_.Run(success);
    complete_callback_.Reset();
  }
}

MediaPlayerEfl* MediaPlayerEfl::CreatePlayer(
    int player_id,
    std::unique_ptr<DemuxerEfl> demuxer,
    MediaPlayerManager* manager,
    bool video_hole) {
  LOG(INFO) << "MediaSourceElement is using |CAPI| to play media";
  return new MediaSourcePlayerCapi(player_id, std::move(demuxer), manager,
                                   video_hole);
}

MediaSourcePlayerCapi::MediaSourcePlayerCapi(
    int player_id,
    std::unique_ptr<DemuxerEfl> demuxer,
    MediaPlayerManager* manager,
    bool video_hole)
    : MediaSourcePlayerCapi(player_id,
                            std::move(demuxer),
                            manager,
                            new BufferObserverImpl(),
#if defined(OS_TIZEN_TV_PRODUCT)
                            PLAYER_DRM_TYPE_EME,
#endif
                            video_hole) {
}

MediaSourcePlayerCapi::MediaSourcePlayerCapi(
    int player_id,
    std::unique_ptr<DemuxerEfl> demuxer,
    MediaPlayerManager* manager,
    BufferObserver* buffer_observer,
#if defined(OS_TIZEN_TV_PRODUCT)
    player_drm_type_e drm_type,
#endif
    bool video_hole)
    : MediaPlayerEfl(player_id, manager, video_hole),
      demuxer_(std::move(demuxer)),
      buffer_observer_(buffer_observer),
      max_audio_buffer_size_(0),
      max_video_buffer_size_(0),
      task_runner_(base::ThreadTaskRunnerHandle::Get()),
      paused_by_buffering_logic_(false),
      pending_seek_(false),
      pending_seek_on_player_(false),
      seeking_video_pushed_(true),
      pending_demuxer_seek_position_(media::kNoTimestamp),
      player_new_state_(PLAYER_STATE_NONE),
      delayed_player_state_(PLAYER_STATE_NONE),
      seek_state_(MEDIA_SEEK_NONE),
#if defined(OS_TIZEN_TV_PRODUCT)
      decoded_frame_count_(0),
      cached_current_time_(base::TimeDelta()),
      drm_type_(drm_type),
#endif
      is_demuxer_state_reported_(false),
      reported_ready_state_(blink::WebMediaPlayer::kReadyStateHaveNothing),
      last_frames_(std::make_pair(media::kNoTimestamp, media::kNoTimestamp),
                   std::make_pair(media::kNoTimestamp, media::kNoTimestamp)),
      weak_factory_(this),
      video_key_frame_(media::kNoTimestamp) {
  DCHECK(buffer_observer_);
#if defined(TIZEN_VIDEO_HOLE)
  if (is_video_hole_) {
    if (is_video_window())
      video_plane_manager_.reset(VideoPlaneManagerTizen::Create(
          MediaPlayerEfl::main_window_handle(), this));
    else  // video windowless mode(ROI)
      video_plane_manager_.reset(VideoPlaneManagerTizen::Create(
          MediaPlayerEfl::main_window_handle(), this, WINDOWLESS_MODE_NORMAL));
  }
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
  memset(&audio_extra_info_, 0, sizeof(player_media_stream_audio_extra_info_s));
  memset(&video_extra_info_, 0, sizeof(player_media_stream_video_extra_info_s));
#endif

  // Initialize player
  if (!ChangePlayerState(PLAYER_STATE_IDLE)) {
#if defined(TIZEN_VIDEO_HOLE)
    video_plane_manager_.reset();
#endif
    return;
  }

  demuxer_->Initialize(this);

  manager->OnReadyStateChange(GetPlayerId(),
                              blink::WebMediaPlayer::kReadyStateHaveNothing);
  manager->OnNetworkStateChange(GetPlayerId(),
                                blink::WebMediaPlayer::kNetworkStateLoading);

  LOG(INFO) << "## MSE Player initialized(ID:" << player_id << ")";
}

bool MediaSourcePlayerCapi::Prepare(CompleteCB cb) {
  LOG(INFO) << __FUNCTION__;
  complete_callback_ = cb;

  SetVolume(volume_);

  int ret =
      player_set_error_cb(player_, MediaSourcePlayerCapi::OnPlayerError, this);
  if (ret != PLAYER_ERROR_NONE) {
    HandlePlayerError(ret, FROM_HERE);
    return false;
  }

#if defined(OS_TIZEN_TV_PRODUCT)
#if defined(TIZEN_TBM_SUPPORT)
  if (!is_video_hole_) {
    ui::SetSecureModeGPU(ui::TzGPUmode::SECURE_MODE);

    // Enhance 360 performance by no window effect.
    if (!elm_win_aux_hint_val_set(
            static_cast<Evas_Object*>(MediaPlayerEfl::main_window_handle()),
            web360_status_hint_id_, "1"))
      LOG(WARNING) << "[elm hint] Set fail : " << kWeb360PlayStateElmHint;
  }
#endif

  NotifyPlaybackState(blink::WebMediaPlayer::kPlaybackLoad);
  position_before_conflict_ = 0;

  std::string tizen_app_id =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kTizenAppId);

  if (!tizen_app_id.empty()) {
    LOG(INFO) << "appid : " << tizen_app_id;

    ret = player_set_app_id(player_, tizen_app_id.c_str());
    if (ret != PLAYER_ERROR_NONE) {
      HandlePlayerError(ret, FROM_HERE);
      return false;
    }
  }
#endif

  ret = player_set_completed_cb(
      player_, MediaSourcePlayerCapi::OnPlaybackComplete, this);
  if (ret != PLAYER_ERROR_NONE) {
    HandlePlayerError(ret, FROM_HERE);
    return false;
  }

  player_set_interrupted_cb(player_, MediaSourcePlayerCapi::OnInterrupted,
                            this);

#if defined(OS_TIZEN_TV_PRODUCT)
  ret = player_display_video_at_paused_state(player_, true);
  if (ret != PLAYER_ERROR_NONE)
    LOG(ERROR) << "player_display_video_at_paused_state() failed";
#endif

#if defined(TIZEN_VIDEO_HOLE)
  if (is_video_hole_) {
    ret = player_set_display(
        player_, PLAYER_DISPLAY_TYPE_OVERLAY,
        GET_DISPLAY(video_plane_manager_->GetVideoPlaneHandle()));
    if (ret != PLAYER_ERROR_NONE) {
      HandlePlayerError(ret, FROM_HERE);
      return false;
    }
  }
#endif

#if defined(TIZEN_VIDEO_HOLE) && defined(TIZEN_VD_LFD_ROTATE)
  if (is_video_hole_) {
    video_plane_manager_->SetDisplayRotation();
  }
#endif

  // Note: In case of Public API, MSE doesn't need uri.
  // Once |player_set_media_stream_info| is called, it worked as MSE.
  buffer_observer_->SetBufferingCallback(
      base::Bind(&MediaSourcePlayerCapi::HandleBufferingStatusChanged,
                 weak_factory_.GetWeakPtr()));

  return true;
}

MediaSourcePlayerCapi::~MediaSourcePlayerCapi() {
  weak_factory_.InvalidateWeakPtrs();
  Release();
  player_destroy(player_);

#if defined(OS_TIZEN_TV_PRODUCT)
  malloc_trim(0);
#endif

#if defined(TIZEN_TBM_SUPPORT) && defined(OS_TIZEN_TV_PRODUCT)
  if (!is_video_hole_)
    ui::SetSecureModeGPU(ui::TzGPUmode::NONSECURE_MODE);
#endif
}

void MediaSourcePlayerCapi::UpdateSeekState(MediaSeekState state) {
  seek_state_ = state;
}

void MediaSourcePlayerCapi::SetRate(double rate) {
  if (DOUBLE_EQUAL(playback_rate_, rate))
    return;

#if defined(OS_TIZEN_TV_PRODUCT)
  if (GetPlayerState() != PLAYER_STATE_PLAYING &&
      !DOUBLE_EQUAL(playback_rate_, 0.0)) {
    pending_playback_rate_ = rate;
    return;
  }
#else
  if (GetPlayerState() < PLAYER_STATE_READY) {
    pending_playback_rate_ = rate;
    return;
  }
#endif

  bool need_play = DOUBLE_EQUAL(playback_rate_, 0.0);
  playback_rate_ = rate;

  // No operation is performed, if rate is 0
  if (DOUBLE_EQUAL(rate, 0.0)) {
    Pause(true);
    return;
  }

  int err =
      player_set_streaming_playback_rate(player_, static_cast<float>(rate));
  if (err != PLAYER_ERROR_NONE) {
    HandlePlayerError(err, FROM_HERE);
    return;
  }

  if (need_play)
    Play();
#if defined(OS_TIZEN_TV_PRODUCT)
  else
    Seek(cached_current_time_);
#endif
}

void MediaSourcePlayerCapi::Seek(base::TimeDelta time) {
  LOG(INFO) << " time : " << time;
#if !defined(OS_TIZEN_TV_PRODUCT)
  if (GetPlayerState() == PLAYER_STATE_PLAYING)
    Pause(true);
#endif
  official_playback_position_ = time;

  ClearBufferQueue(DemuxerStream::AUDIO);
  ClearBufferQueue(DemuxerStream::VIDEO);

  last_frames_.get<DemuxerStream::AUDIO>().first = media::kNoTimestamp;
  last_frames_.get<DemuxerStream::VIDEO>().first = media::kNoTimestamp;
  UncheckedUpdateReadyState();

  UpdateSeekState(MEDIA_SEEK_DEMUXER);
  demuxer_->RequestDemuxerSeek(time);
}

player_state_e MediaSourcePlayerCapi::GetPlayerState() {
  player_state_e state = PLAYER_STATE_NONE;
  player_get_state(player_, &state);
  return state;
}

void MediaSourcePlayerCapi::PlayInternal() {
  if (!ChangePlayerState(PLAYER_STATE_PLAYING))
    return;

  is_paused_ = false;
  paused_by_buffering_logic_ = false;
  should_seek_after_resume_ = true;

  if (GetMediaType() & MediaType::Video)
    WakeUpDisplayAndAcquireDisplayLock();

  StartCurrentTimeUpdateTimer();
}

void MediaSourcePlayerCapi::SeekInternal() {
  LOG(INFO) << "seek time : " << seek_offset_
            << " state : " << GetString(GetPlayerState());
  if (GetPlayerState() < PLAYER_STATE_READY || !player_prepared_) {
    // Will be handled once prepare is complete.
    pending_seek_ = true;
    pending_seek_offset_ = seek_offset_;

    if ((GetMediaType() & MEDIA_VIDEO_MASK) &&
        (GetMediaType() & MEDIA_AUDIO_MASK))
      seeking_video_pushed_ = false;

    // The below read is required to handle TCs 37 and 47 at
    // yt-dash-mse-test.commondatastorage.googleapis.com/unit-tests/tip.html
    // It is assumed that at-least one |OnxxxxxBufferedSizeChanged| callback is
    // triggered by CAPI for each media stream after calling
    // |player_prepare_async|
    ReadDemuxedDataIfNeed(DemuxerStream::AUDIO);
    ReadDemuxedDataIfNeed(DemuxerStream::VIDEO);
    return;
  }

#if defined(OS_TIZEN_TV_PRODUCT)
  cached_current_time_ = seek_offset_;
#endif
  pending_seek_on_player_ = true;
  const int ret = PlayerSetPlayPosition(
      player_, seek_offset_, true, MediaSourcePlayerCapi::OnSeekComplete, this);

  seek_offset_ = base::TimeDelta();

  MediaSeekState state = MEDIA_SEEK_PLAYER;
  if (ret != PLAYER_ERROR_NONE) {
    LOG(ERROR) << " Error : " << ret << "Seek to " << seek_offset_.InSeconds()
               << " failed. Current time : " << GetCurrentTime().InSecondsF();
    HandlePlayerError(ret, FROM_HERE);
    manager()->OnSeekComplete(GetPlayerId());
    state = MEDIA_SEEK_NONE;
    pending_seek_on_player_ = false;
  }
  UpdateSeekState(state);
}

void MediaSourcePlayerCapi::OnDemuxerDurationChanged(base::TimeDelta duration) {
  LOG(INFO) << "Duration changed: " << duration;
  duration_ = duration;
#if defined(OS_TIZEN_TV_PRODUCT)
  UpdateReadyState();
#endif
}

void MediaSourcePlayerCapi::OnDemuxerSeekDone(
    const base::TimeDelta& actual_browser_seek_time,
    const base::TimeDelta& video_key_frame) {
  if (pending_seek_on_player_ == true) {
    pending_demuxer_seek_position_ = actual_browser_seek_time;
    ReadDemuxedDataIfNeed(DemuxerStream::AUDIO);
    ReadDemuxedDataIfNeed(DemuxerStream::VIDEO);
    return;
  }

  video_key_frame_ = video_key_frame;
  seek_offset_ = actual_browser_seek_time;
  official_playback_position_ = actual_browser_seek_time;
  UpdateSeekState(MEDIA_SEEK_DEMUXER_DONE);

  ClearBufferQueue(DemuxerStream::AUDIO);
  ClearBufferQueue(DemuxerStream::VIDEO);
  buffer_observer_->ResetBufferStatus();
  SetEos(DemuxerStream::AUDIO, false);
  SetEos(DemuxerStream::VIDEO, false);
  SeekInternal();
}

void MediaSourcePlayerCapi::OnMediaPacketUpdated(media_packet_h packet,
                                                 void* data) {
  MediaSourcePlayerCapi* player = static_cast<MediaSourcePlayerCapi*>(data);
#if defined(USE_DIRECT_CALL_MEDIA_PACKET_THREAD)
  player->DeliverMediaPacket(ScopedMediaPacket(packet));
#else
  ScopedMediaPacket packet_proxy(packet);
  player->task_runner_->PostTask(
      FROM_HERE, base::Bind(&MediaSourcePlayerCapi::DeliverMediaPacket,
                            player->weak_factory_.GetWeakPtr(),
                            base::Passed(&packet_proxy)));
#endif
}

void MediaSourcePlayerCapi::SetVolume(double volume) {
  if (GetPlayerState() == PLAYER_STATE_NONE)
    return;

  if (player_set_volume(player_, volume, volume) != PLAYER_ERROR_NONE)
    LOG(ERROR) << "|player_set_volume| failed";

  volume_ = volume;
}

base::TimeDelta MediaSourcePlayerCapi::GetCurrentTime() {
  base::TimeDelta position;
  int err = PlayerGetPlayPosition(player_, &position);
  if (err != PLAYER_ERROR_NONE) {
    LOG(ERROR) << "player_get_play_position failed. Error #" << err;
    return base::TimeDelta();
  }

#if defined(OS_TIZEN_TV_PRODUCT)
  cached_current_time_ = position;
#endif
  official_playback_position_ = position;
  return position;
}

void MediaSourcePlayerCapi::OnCurrentTimeUpdateTimerFired() {
  const auto current_time = GetCurrentTime();
#if defined(OS_TIZEN_TV_PRODUCT)
  UpdateCurrentTime(current_time);
#endif
  UpdateReadyState();
  manager()->OnTimeUpdate(GetPlayerId(), current_time);
}

void MediaSourcePlayerCapi::StartCurrentTimeUpdateTimer() {
  if (current_time_update_timer_.IsRunning())
    return;

  current_time_update_timer_.Start(
      FROM_HERE, kCurrentTimeUpdateInterval, this,
      &MediaSourcePlayerCapi::OnCurrentTimeUpdateTimerFired);
}

void MediaSourcePlayerCapi::StopCurrentTimeUpdateTimer() {
  if (current_time_update_timer_.IsRunning())
    current_time_update_timer_.Stop();
}

MediaSourcePlayerCapi::ChannelData& MediaSourcePlayerCapi::GetChannelData(
    DemuxerStream::Type type) {
  return channel_data_[DemuxStreamToChannelIndex(type)];
}

const MediaSourcePlayerCapi::ChannelData& MediaSourcePlayerCapi::GetChannelData(
    DemuxerStream::Type type) const {
  return channel_data_[DemuxStreamToChannelIndex(type)];
}

bool MediaSourcePlayerCapi::IsEos(DemuxerStream::Type type) const {
  return GetChannelData(type).is_eos_;
}

void MediaSourcePlayerCapi::SetEos(DemuxerStream::Type type, bool value) {
  GetChannelData(type).is_eos_ = value;
}

bool MediaSourcePlayerCapi::ShouldFeed(DemuxerStream::Type type) const {
  return GetChannelData(type).should_feed_;
}

void MediaSourcePlayerCapi::SetShouldFeed(DemuxerStream::Type type,
                                          bool value) {
  GetChannelData(type).should_feed_ = value;
}

media_format_s* MediaSourcePlayerCapi::GetMediaFormat(
    DemuxerStream::Type type) const {
  return GetChannelData(type).media_format_.get();
}

void MediaSourcePlayerCapi::SetMediaFormat(media_format_s* format,
                                           DemuxerStream::Type type) {
  GetChannelData(type).media_format_.reset(format);
}

MediaSourcePlayerCapi::BufferQueue& MediaSourcePlayerCapi::GetBufferQueue(
    DemuxerStream::Type type) {
  return GetChannelData(type).buffer_queue_;
}

void MediaSourcePlayerCapi::ClearBufferQueue(DemuxerStream::Type type) {
  BufferQueue().swap(GetBufferQueue(type));
}

void MediaSourcePlayerCapi::ReadFromQueueIfNeeded(DemuxerStream::Type type) {
  while (!GetBufferQueue(type).empty() && ShouldFeed(type)) {
    SetShouldFeed(type, ReadFromQueue(type));
  }
}

bool MediaSourcePlayerCapi::ReadFromQueue(DemuxerStream::Type type) {
  auto& buffer_queue = GetBufferQueue(type);
  DCHECK(!buffer_queue.empty());

  auto media_packet = CreateMediaPacket(buffer_queue.front(), type);
  auto ret = PushMediaPacket(*media_packet);

  switch (ret) {
    case SendMediaPacketResult::SUCCESS:
#if defined(OS_TIZEN_TV_PRODUCT)
      // Tz handle ownership is passed to MM player
      buffer_queue.front()->pass_tz_handle();
#endif
      buffer_queue.pop();
      return true;
    case SendMediaPacketResult::FAILURE:
#if !defined(OS_TIZEN_TV_PRODUCT)
      return true;
#endif
    case SendMediaPacketResult::BUFFER_SPACE:
      return false;
  }
  NOTREACHED();
  return false;
}

std::unique_ptr<MediaPacket> MediaSourcePlayerCapi::CreateEosMediaPacket(
    DemuxerStream::Type type) {
  return base::MakeUnique<EosMediaPacket>(GetMediaFormat(type), type);
}

std::unique_ptr<MediaPacket> MediaSourcePlayerCapi::CreateMediaPacket(
    scoped_refptr<DecoderBuffer> decoder_buffer,
    DemuxerStream::Type type) {
  std::unique_ptr<MediaPacket> media_packet;
#if defined(OS_TIZEN_TV_PRODUCT)
  if (decoder_buffer->is_tz_handle_valid()) {
    auto& tz_handle = decoder_buffer->get_tz_handle();
    media_packet = base::MakeUnique<EncryptedMediaPacket>(
        tz_handle.handle(), tz_handle.size(), decoder_buffer->timestamp(),
        decoder_buffer->duration(), GetMediaFormat(type), type);
  } else
#endif
  {
    media_packet = base::MakeUnique<CleanMediaPacket>(
        static_cast<void*>(decoder_buffer.get()->writable_data()),
        decoder_buffer.get()->data_size(), decoder_buffer->timestamp(),
        decoder_buffer->duration(), GetMediaFormat(type), type);
  }

  return media_packet;
}

std::unique_ptr<MediaPacket> MediaSourcePlayerCapi::CreateMediaPacket(
    base::SharedMemoryHandle foreign_memory_handle,
    const media::DemuxedBufferMetaData& meta_data) {
  std::unique_ptr<MediaPacket> media_packet;
#if defined(OS_TIZEN_TV_PRODUCT)
  if (meta_data.tz_handle) {
    media_packet = base::MakeUnique<EncryptedMediaPacket>(
        meta_data.tz_handle, meta_data.size, meta_data.timestamp,
        meta_data.time_duration, GetMediaFormat(meta_data.type),
        meta_data.type);
  } else
#endif
  {
    media_packet = base::MakeUnique<ShmCleanMediaPacket>(
        foreign_memory_handle, meta_data.size, meta_data.timestamp,
        meta_data.time_duration, GetMediaFormat(meta_data.type),
        meta_data.type);
  }

  return media_packet;
}

MediaSourcePlayerCapi::SendMediaPacketResult
MediaSourcePlayerCapi::PushMediaPacket(MediaPacket& media_packet) {
#if defined(OS_TIZEN_TV_PRODUCT)
  LOG(INFO) << "Pushing media packet: " << media_packet;
#endif

  auto packet = media_packet.PrepareAndGet();
  if (!packet) {
    LOG(ERROR) << "Failed to create media packet";
    return SendMediaPacketResult::FAILURE;
  }

  int ret = player_push_media_stream(player_, packet);
  if (ret == PLAYER_ERROR_BUFFER_SPACE) {
    LOG(ERROR) << "player_push_media_stream failed: PLAYER_ERROR_BUFFER_SPACE.";
    return SendMediaPacketResult::BUFFER_SPACE;
  } else if (ret != PLAYER_ERROR_NONE) {
    LOG(ERROR) << "player_push_media_stream failed : " << ret;
    return SendMediaPacketResult::FAILURE;
  }

#if defined(OS_TIZEN_TV_PRODUCT)
  if (media_packet.GetType() == media::DemuxerStream::VIDEO) {
    if (video_extra_info_.is_hdr_changed) {
      video_extra_info_.is_hdr_changed = false;
      LOG(INFO) << "set video_extra_info.is_hdr_changed to false";
    }
    decoded_frame_count_++;
  }
#endif

  if (!IsEos(media_packet.GetType())) {
    last_frames_[media_packet.GetType()].first = media_packet.GetTimeStamp();
    last_frames_[media_packet.GetType()].second =
        media_packet.GetTimeDuration();
  } else {  // EOS means we have all the data
    last_frames_[media_packet.GetType()].first = base::TimeDelta::Max();
  }
  return SendMediaPacketResult::SUCCESS;
}

#if defined(OS_TIZEN_TV_PRODUCT)
unsigned MediaSourcePlayerCapi::GetDecodedFrameCount() const {
  return decoded_frame_count_;
}

unsigned MediaSourcePlayerCapi::GetDroppedFrameCount() {
  uint64_t dropped_frame_count = 0;
  int ret = player_get_adaptive_streaming_info(
      player_, &dropped_frame_count, PLAYER_ADAPTIVE_INFO_VIDEO_FRAMES_DROPPED);
  if (ret != PLAYER_ERROR_NONE) {
    LOG(ERROR) << "GetDroppedFrameCount fail,ret:" << ret;
    return 0;
  }
  return static_cast<unsigned>(dropped_frame_count);
}
#endif

void MediaSourcePlayerCapi::SaveDecoderBuffer(
    base::SharedMemoryHandle foreign_memory_handle,
    const media::DemuxedBufferMetaData& meta_data) {
  scoped_refptr<DecoderBuffer> buffer;

#if defined(OS_TIZEN_TV_PRODUCT)
  if (meta_data.tz_handle) {
    buffer = DecoderBuffer::CreateTzHandleBuffer(meta_data.tz_handle,
                                                 meta_data.size);
  } else
#endif
  {
    base::SharedMemory shared_memory(foreign_memory_handle, false);
    if (!shared_memory.Map(meta_data.size)) {
      LOG(ERROR) << "Failed to map shared memory of size " << meta_data.size;
      return;
    }

    buffer = DecoderBuffer::CopyFrom(
        static_cast<const uint8*>(shared_memory.memory()), meta_data.size);

    if (!buffer.get()) {
      LOG(ERROR) << "DecoderBuffer::CopyFrom failed";
      return;
    }
  }

  buffer->set_timestamp(meta_data.timestamp);
  buffer->set_duration(meta_data.time_duration);
  GetBufferQueue(meta_data.type).push(buffer);
}

player_buffer_size_t MediaSourcePlayerCapi::GetMaxBufferSize(
    media::DemuxerStream::Type type,
    const DemuxerConfigs& configs) {
  int fps = 30;
  base::TimeDelta buffering_time = duration_.is_zero()
                                       ? kBufferingTime
                                       : std::min(duration_, kBufferingTime);
  switch (type) {
    case DemuxerStream::VIDEO:
      if (configs.duration_ms > 0)
        fps = 1000 / configs.duration_ms;
      // Reference of buffer size estimation:
      // http://stackoverflow.com/questions/5024114/suggested-compression-ratio-with-h-264
      return static_cast<player_buffer_size_t>(configs.video_size.width()) *
             configs.video_size.height() * fps * 2 * 7 / 100 / 8 *
             buffering_time.InSeconds();
    case DemuxerStream::AUDIO:
      // Assume MPEG-1 Audio Layer III 320kbit/s audio.
      return 320 * 1000 / 8 *
             static_cast<player_buffer_size_t>(buffering_time.InSeconds());
    default:
      LOG(ERROR) << "Unexpected Type: " << type;
  }

  NOTREACHED();
  return 0;
}

void MediaSourcePlayerCapi::UpdateVideoBufferSize(
    player_buffer_size_t buffer_size_in_bytes) {
  int ret = player_set_media_stream_buffer_max_size(
      player_, PLAYER_STREAM_TYPE_VIDEO, buffer_size_in_bytes);

  if (ret != PLAYER_ERROR_NONE) {
    LOG(WARNING) << "Fail to update player buffer size to "
                 << buffer_size_in_bytes / 1024 << "KB";
    return;
  }
  buffer_observer_->SetBufferSize(DemuxerStream::VIDEO, buffer_size_in_bytes);
  LOG(INFO) << "New VIDEO Buffer Size: " << buffer_size_in_bytes / 1024 << "KB";
}

inline void MediaSourcePlayerCapi::SetMediaType(const DemuxerConfigs& configs) {
  // Check if the video stream exists and is valid.
  if ((configs.video_codec == kCodecH264
#if defined(TIZEN_MULTIMEDIA_WEBM_SUPPORT)
       || configs.video_codec == kCodecVP9 || configs.video_codec == kCodecVP8
#endif
#if BUILDFLAG(ENABLE_HEVC_DEMUXING)
       || configs.video_codec == kCodecHEVC
#endif
       ) &&
      configs.video_size.width() > 0 && configs.video_size.height() > 0) {
    MediaPlayerEfl::SetMediaType(MediaType::Video);
  }

  // Check if the audio stream exists and is valid.
  if ((configs.audio_codec == kCodecAAC || configs.audio_codec == kCodecMP3 ||
       configs.audio_codec == kCodecOpus
#if defined(TIZEN_MULTIMEDIA_WEBM_SUPPORT)
       || configs.audio_codec == kCodecVorbis
#endif
#if BUILDFLAG(ENABLE_AC3_EAC3_AUDIO_DEMUXING)
       || configs.audio_codec == kCodecEAC3
       || configs.audio_codec == kCodecAC3
#endif
       ) &&
      (configs.audio_sampling_rate > 0)) {
    MediaPlayerEfl::SetMediaType(MediaType::Audio);
  }
}

void MediaSourcePlayerCapi::Play() {
  LOG(INFO) << __FUNCTION__;

  if (DOUBLE_EQUAL(playback_rate_, 0.0)) {
    is_paused_ = false;
    return;
  }

  if (GetMediaType() & MediaType::Video)
    WakeUpDisplayAndAcquireDisplayLock();

  is_paused_ = false;
  if (paused_by_buffering_logic_) {
    LOG(INFO) << "paused_by_buffering_logic_ true";
    return;
  }

  paused_by_buffering_logic_ = true;
  UncheckedUpdateReadyState();
}

void MediaSourcePlayerCapi::Pause(bool is_media_related_action) {
  LOG(INFO) << __FUNCTION__
            << ", is_media_related_action : " << is_media_related_action;

  paused_by_buffering_logic_ = is_media_related_action;
  if (!is_media_related_action) {
    is_paused_ = true;
  }

  if (!ChangePlayerState(PLAYER_STATE_PAUSED))
    return;

  StopCurrentTimeUpdateTimer();
  if (!is_media_related_action && (GetMediaType() & MediaType::Video)) {
    ReleaseDisplayLock();
  }
}

void MediaSourcePlayerCapi::HandlePlaybackComplete() {
  StopCurrentTimeUpdateTimer();
  SetEos(DemuxerStream::AUDIO, false);
  SetEos(DemuxerStream::VIDEO, false);
  manager()->OnTimeUpdate(GetPlayerId(), duration_);
  manager()->OnTimeChanged(GetPlayerId());

#if defined(OS_TIZEN_TV_PRODUCT)
  UpdateCurrentTime(duration_);
  NotifyPlaybackState(blink::WebMediaPlayer::kPlaybackFinish);
#endif
}

// TODO(sam) : It's worked as bypass now. Need Suspend/Resume/Initialize
// implementation for MSE multiple instance.
void MediaSourcePlayerCapi::Initialize() {
#if defined(TIZEN_VIDEO_HOLE)
  if (is_video_hole_ && !video_plane_manager_)
    return;
#endif

  if (!Prepare(base::Bind(&MediaSourcePlayerCapi::OnInitComplete,
                          weak_factory_.GetWeakPtr())))
    RunCompleteCB(false, FROM_HERE);
}

void MediaSourcePlayerCapi::Release() {
  LOG(INFO) << "Release,this:" << (void*)this;
#if !defined(OS_TIZEN_TV_PRODUCT)
  is_paused_ = true;
#endif
  SetShouldFeed(DemuxerStream::AUDIO, false);
  SetShouldFeed(DemuxerStream::VIDEO, false);
  StopCurrentTimeUpdateTimer();

#if defined(OS_TIZEN_TV_PRODUCT)
  if (position_before_conflict_ > 0) {
    manager()->OnTimeUpdate(GetPlayerId(), base::TimeDelta::FromMilliseconds(
                                               position_before_conflict_));
    UpdateCurrentTime(
        base::TimeDelta::FromMilliseconds(position_before_conflict_));
  }
#endif

  if (player_) {
    if ((GetMediaType() & MediaType::Audio))
      player_unset_media_stream_buffer_status_cb_ex(player_,
                                                    PLAYER_STREAM_TYPE_AUDIO);
    if ((GetMediaType() & MediaType::Video))
      player_unset_media_stream_buffer_status_cb_ex(player_,
                                                    PLAYER_STREAM_TYPE_VIDEO);
    player_unset_completed_cb(player_);
    player_unset_interrupted_cb(player_);

#if defined(OS_TIZEN_TV_PRODUCT)
    if (!is_video_hole_)
#endif
      player_unset_media_packet_video_frame_decoded_cb(player_);

    if (GetPlayerState() > PLAYER_STATE_IDLE ||
        player_new_state_ > PLAYER_STATE_IDLE) {
      if (GetPlayerState() != PLAYER_STATE_IDLE ||
          player_new_state_ != PLAYER_STATE_READY) {
        player_new_state_ = PLAYER_STATE_NONE;
      }
      ChangePlayerState(PLAYER_STATE_IDLE);
    } else {
      player_new_state_ = PLAYER_STATE_NONE;
      delayed_player_state_ = PLAYER_STATE_NONE;
      LOG(INFO) << "Not changing states because the player is in "
                << GetString(GetPlayerState());
    }
  }
  if (buffer_observer_)
    buffer_observer_->ResetBufferStatus();
  pending_seek_on_player_ = false;

#if defined(OS_TIZEN_TV_PRODUCT)
#if defined(TIZEN_TBM_SUPPORT)
  if (!is_video_hole_ &&
      !elm_win_aux_hint_val_set(
          static_cast<Evas_Object*>(MediaPlayerEfl::main_window_handle()),
          web360_status_hint_id_, "0"))
    LOG(WARNING) << "[elm hint] Unset fail : " << kWeb360PlayStateElmHint;
#endif
  NotifyPlaybackState(blink::WebMediaPlayer::kPlaybackStop);
#endif

  if (manager())
    manager()->OnSuspendComplete(GetPlayerId());
}

#if defined(TIZEN_VIDEO_HOLE)
void MediaSourcePlayerCapi::SetGeometry(const gfx::RectF& rect) {
  if (!is_video_hole_)
    return;
  int ret = video_plane_manager_->SetGeometry(rect);
  if (ret != PLAYER_ERROR_NONE) {
    HandlePlayerError(ret, FROM_HERE);
    return;
  }
}

void MediaSourcePlayerCapi::OnWebViewMoved() {
  if (!is_video_hole_)
    return;
  video_plane_manager_->ResetGeometry();
}
#endif  // defined(TIZEN_VIDEO_HOLE)

bool MediaSourcePlayerCapi::SetAudioStreamInfo() {
  if (!(GetMediaType() & MediaType::Audio))
    return true;
  if (!GetMediaFormat(DemuxerStream::AUDIO))
    return false;

  int ret = PLAYER_ERROR_NONE;

#if defined(TIZEN_SOUND_FOCUS)
  ret = player_set_sound_stream_info(
      player_, SoundFocusManager::GetInstance()->GetSoundStreamInfo());
  if (ret != PLAYER_ERROR_NONE)
    LOG(ERROR) << "player_set_sound_stream_info failed " << ret;
#endif
  if (buffer_observer_->SetMediaStreamStatusAudioCallback(player_) !=
      PLAYER_ERROR_NONE) {
    HandlePlayerError(ret, FROM_HERE);
    return false;
  }
  ret = player_set_media_stream_info(player_, PLAYER_STREAM_TYPE_AUDIO,
                                     GetMediaFormat(DemuxerStream::AUDIO));
  if (ret != PLAYER_ERROR_NONE) {
    LOG(ERROR) << "player_set_audio_stream_info Error: " << ret;
    return false;
  }

  ret = player_set_media_stream_buffer_max_size(
      player_, PLAYER_STREAM_TYPE_AUDIO, max_audio_buffer_size_);
  if (ret != PLAYER_ERROR_NONE) {
    LOG(WARNING) << "player_set_media_stream_buffer_max_size : Error: " << ret;
  }
  return true;
}

bool MediaSourcePlayerCapi::SetVideoStreamInfo() {
  if (!(GetMediaType() & MediaType::Video))
    return true;
  if (!GetMediaFormat(DemuxerStream::VIDEO))
    return false;

  int ret = PLAYER_ERROR_NONE;
#if defined(OS_TIZEN_TV_PRODUCT)
  if (!is_video_hole_)
#endif
  {
    ret = player_set_media_packet_video_frame_decoded_cb(
        player_, MediaSourcePlayerCapi::OnMediaPacketUpdated, this);
    if (ret != PLAYER_ERROR_NONE) {
      HandlePlayerError(ret, FROM_HERE);
      return false;
    }
  }

#if defined(TIZEN_VIDEO_HOLE) && !defined(OS_TIZEN_TV_PRODUCT)
  if (is_video_hole_) {
    ret = player_enable_media_packet_video_frame_decoded_cb(player_,
                                                            !is_fullscreen_);
    if (ret != PLAYER_ERROR_NONE) {
      HandlePlayerError(ret, FROM_HERE);
      return false;
    }
  }
#endif

  if (buffer_observer_->SetMediaStreamStatusVideoCallback(player_) !=
      PLAYER_ERROR_NONE) {
    HandlePlayerError(ret, FROM_HERE);
    return false;
  }
  ret = player_set_media_stream_info(player_, PLAYER_STREAM_TYPE_VIDEO,
                                     GetMediaFormat(DemuxerStream::VIDEO));
  if (ret != PLAYER_ERROR_NONE) {
    LOG(ERROR) << "player_set_video_stream_info Error: " << ret;
    return false;
  }
  UpdateVideoBufferSize(max_video_buffer_size_);
#if !defined(OS_TIZEN_TV_PRODUCT)
  player_set_media_stream_dynamic_resolution(player_, true);
#endif
  return true;
}

bool MediaSourcePlayerCapi::HasConfigs() const {
  return GetMediaFormat(DemuxerStream::VIDEO) ||
         GetMediaFormat(DemuxerStream::AUDIO);
}

// We want to get larger duration value from last frames, that have been sent
// to mm_player
base::TimeDelta MediaSourcePlayerCapi::CalculateLastFrameDuration() const {
  return std::max(base::TimeDelta(),
                  std::max(last_frames_.get<DemuxerStream::VIDEO>().second,
                           last_frames_.get<DemuxerStream::AUDIO>().second));
}

#if defined(OS_TIZEN_TV_PRODUCT)
void MediaSourcePlayerCapi::OnPlayerResourceConflict() {
  int ret_val = player_get_play_position_ex(
      player_, (int64_t*)&position_before_conflict_);
  if (ret_val != PLAYER_ERROR_NONE)
    LOG(WARNING) << "player_get_play_position_ex failed";

  MediaPlayerEfl::OnPlayerResourceConflict();
}

void MediaSourcePlayerCapi::OnDemuxerStateChanged(
    const DemuxerConfigs& configs) {
  LOG(INFO) << "New width " << configs.video_size.width() << " height "
            << configs.video_size.height();

  SetMediaType(configs);
  int media_type = GetMediaType();

  if (!(media_type & MEDIA_VIDEO_MASK) && !(media_type & MEDIA_AUDIO_MASK)) {
    LOG(ERROR) << "Neither Audio nor Video stream is valid for MediaSource";
    HandlePlayerError(PLAYER_ERROR_NOT_SUPPORTED_FILE, FROM_HERE);
    RunCompleteCB(false, FROM_HERE);
    return;
  }

  if (media_type & MEDIA_VIDEO_MASK) {
    auto video_format = GetMediaFormat(DemuxerStream::VIDEO);
    UpdateResolution(configs.video_size.width(), configs.video_size.height(),
                     media_type);

    if (configs.is_framerate_changed) {
      LOG(INFO) << "Framerate change detected, framerate: "
                << static_cast<double>(video_extra_info_.framerate_num) /
                       static_cast<double>(video_extra_info_.framerate_den)
                << " -> "
                << static_cast<double>(configs.framerate_num) /
                       static_cast<double>(configs.framerate_den);
      video_extra_info_.framerate_num = configs.framerate_num;
      video_extra_info_.framerate_den = configs.framerate_den;
      video_extra_info_.is_framerate_changed = configs.is_framerate_changed;

      if (video_format) {
        media_format_set_extra(video_format, &video_extra_info_);
        int ret =
            player_set_media_stream_info(player_, PLAYER_STREAM_TYPE_VIDEO,
                                         GetMediaFormat(DemuxerStream::VIDEO));
        if (ret != PLAYER_ERROR_NONE)
          LOG(WARNING) << "set video info failed";
      }
      video_extra_info_.is_framerate_changed = false;
    }
  }
}

void MediaSourcePlayerCapi::UpdateResolution(int width,
                                             int height,
                                             int media_type) {
  // Update video info if resolution changes.
  if (width != width_ || height != height_) {
    LOG(INFO) << "Update video info : resolution changes.";
    width_ = width;
    height_ = height;
    auto video_format = GetMediaFormat(DemuxerStream::VIDEO);

    if (video_format) {
      media_format_set_video_width(video_format, width_);
      media_format_set_video_height(video_format, height_);
    }
    manager()->OnMediaDataChange(GetPlayerId(), width_, height_, media_type);
  }
}
#endif

void MediaSourcePlayerCapi::StartWaitingForData() {
  if (GetPlayerState() == PLAYER_STATE_PLAYING)
    Pause(true);
}

void MediaSourcePlayerCapi::FinishWaitingForData() {
  if (paused_by_buffering_logic_ && seek_state_ == MEDIA_SEEK_NONE) {
    DLOG(INFO) << "Resume playback after stall";

    // Not pushed Video or Audio frame to player.
    const auto media_type = GetMediaType();
    if ((!(media_type & MediaType::Audio) &&
         !(media_type & MediaType::Video)) ||
        ((media_type & MediaType::Video) &&
         last_frames_.get<DemuxerStream::VIDEO>().first ==
             media::kNoTimestamp) ||
        ((media_type & MediaType::Audio) &&
         last_frames_.get<DemuxerStream::AUDIO>().first ==
             media::kNoTimestamp)) {
      LOG(INFO) << "video or audio is null";
      return;
    }

    // Get the useful end position of pushed data.
    const auto cur_end =
        std::min((media_type & MediaType::Video)
                     ? last_frames_.get<DemuxerStream::VIDEO>().first +
                           last_frames_.get<DemuxerStream::VIDEO>().second
                     : base::TimeDelta::Max(),
                 (media_type & MediaType::Audio)
                     ? last_frames_.get<DemuxerStream::AUDIO>().first +
                           last_frames_.get<DemuxerStream::AUDIO>().second
                     : base::TimeDelta::Max());

    // useful data >= 500ms, resume play.
    if (cur_end - official_playback_position_ >=
        kCurrentTimeUpdateInterval * 5) {
      PlayInternal();
    }
  }
}

std::ostream& operator<<(std::ostream& out,
                         const media::Ranges<base::TimeDelta>& range) {
  out << "{";
  for (size_t i = 0; i < range.size(); ++i) {
    out << " " << i << ": [" << range.start(i).InMilliseconds() << ", "
        << range.end(i).InMilliseconds() << "]";
    if (i != range.size() - 1)
      out << ",";
  }
  out << " }";
  return out;
}

void MediaSourcePlayerCapi::OnDemuxerBufferedChanged(
    const media::Ranges<base::TimeDelta>& ranges) {
  demuxer_buffered_ranges_ = ranges;

  DLOG(INFO) << demuxer_buffered_ranges_;

  UpdateReadyState();
}

std::pair<base::TimeDelta, base::TimeDelta>
MediaSourcePlayerCapi::FindBufferedRange(base::TimeDelta start,
                                         base::TimeDelta end) const {
  // Time obtained direcly from a player is rounded to milliseconds. Time
  // obtained from demuxer, after seek, is kept in full precision and should
  // be rounded explicitly.
  start = start.RoundedToMilliseconds();
  end = end.RoundedToMilliseconds();
  for (size_t i = 0; i < demuxer_buffered_ranges_.size(); ++i) {
    // Rounding buffered ranges to milliseconds to make it conformant with
    // current time returned by CAPI
    if (end >= demuxer_buffered_ranges_.start(i).RoundedToMilliseconds() &&
        start <= demuxer_buffered_ranges_.end(i).RoundedToMilliseconds())
      return {demuxer_buffered_ranges_.start(i),
              demuxer_buffered_ranges_.end(i)};
  }
  return {media::kNoTimestamp, media::kNoTimestamp};
}

void MediaSourcePlayerCapi::UncheckedUpdateReadyState() {
  const auto ReportStateIfNeeded =
      [this](blink::WebMediaPlayer::ReadyState newState) {
        if (reported_ready_state_ != newState) {
          LOG(INFO) << "Reporting readyState change from "
                    << reported_ready_state_ << " to " << newState;
          manager()->OnReadyStateChange(GetPlayerId(), newState);
          reported_ready_state_ = newState;
        }
      };

  // TODO(m.debski): Use framerate instead
  const auto last_frame_duration = CalculateLastFrameDuration();
  const auto range =
      FindBufferedRange(official_playback_position_,
                        official_playback_position_ + last_frame_duration);

  // 2.4.4 SourceBuffer Monitoring
  // If HTMLMediaElement.buffered does not contain a TimeRange for the current
  // playback position:
  if (range.first == media::kNoTimestamp ||
      range.second == media::kNoTimestamp) {
    // 1. Set the HTMLMediaElement.readyState attribute to HAVE_METADATA.
    ReportStateIfNeeded(blink::WebMediaPlayer::kReadyStateHaveMetadata);
    // 2. Abort these steps.
    return;
  }

  // If HTMLMediaElement.buffered contains a TimeRange that includes the current
  // playback position and enough data to ensure uninterrupted playback:
  // TODO(m.debski): Better conditon than flat 5 seconds of buffered data.
  if (range.second >= (duration_ - last_frame_duration) ||
      range.second - official_playback_position_ > kBufferingTime) {
    // 1. Set the HTMLMediaElement.readyState attribute to HAVE_ENOUGH_DATA.
    ReportStateIfNeeded(blink::WebMediaPlayer::kReadyStateHaveEnoughData);
    // 2. Playback may resume at this point if it was previously suspended by a
    // transition to HAVE_CURRENT_DATA.
    FinishWaitingForData();
    // 3. Abort these steps.
    return;
  }

  // If HTMLMediaElement.buffered contains a TimeRange that includes the current
  // playback position and some time beyond the current playback position, then
  // run the following steps:
  // We have HAVE_FUTURE_DATA if we will be able to update current time one more
  // time.
  if (range.second - official_playback_position_ >
      (kCurrentTimeUpdateInterval + last_frame_duration)) {
    // 1. Set the HTMLMediaElement.readyState attribute to HAVE_FUTURE_DATA.
    ReportStateIfNeeded(blink::WebMediaPlayer::kReadyStateHaveFutureData);
    // 2. Playback may resume at this point if it was previously suspended by a
    // transition to HAVE_CURRENT_DATA.
    FinishWaitingForData();
    // 3. Abort these steps.
    return;
  }

  // If HTMLMediaElement.buffered contains a TimeRange that ends at the current
  // playback position and does not have a range covering the time immediately
  // after the current position:
  //   1. Set the HTMLMediaElement.readyState attribute to HAVE_CURRENT_DATA.
  ReportStateIfNeeded(blink::WebMediaPlayer::kReadyStateHaveCurrentData);
  //   2. Playback is suspended at this point since the media element doesn't
  //   have enough data to advance the media timeline.
  DLOG(INFO) << "Stalling playback";
  StartWaitingForData();
  //   3. Abort these steps.
}

void MediaSourcePlayerCapi::UpdateReadyState() {
  // We don't want to change ready state until we receive configs and we update
  // states only if not stalled because we need to wait until data is pushed to
  // the player
  if (reported_ready_state_ != blink::WebMediaPlayer::kReadyStateHaveNothing &&
      !paused_by_buffering_logic_ && seek_state_ == MEDIA_SEEK_NONE)
    UncheckedUpdateReadyState();
}

void MediaSourcePlayerCapi::OnDemuxerConfigsAvailable(
    const DemuxerConfigs& configs) {
  LOG(INFO) << "New width " << configs.video_size.width() << " height "
            << configs.video_size.height();
  SetMediaType(configs);
  const auto media_type = GetMediaType();
  if (!(media_type & MediaType::Video) && !(media_type & MediaType::Audio)) {
    LOG(ERROR) << "Neither Audio nor Video stream is valid for MediaSource";
    HandlePlayerError(PLAYER_ERROR_NOT_SUPPORTED_FILE, FROM_HERE);
    return;
  }

  auto video_format = GetMediaFormat(DemuxerStream::VIDEO);
  if ((media_type & MediaType::Video) && video_format) {
    media_format_set_video_width(video_format, configs.video_size.width());
    media_format_set_video_height(video_format, configs.video_size.height());

    int max_buffer_size = GetMaxBufferSize(DemuxerStream::VIDEO, configs);
    UpdateVideoBufferSize(max_buffer_size);

    // Read request is returned with config change. So re-trigger demuxer read.
    ReadDemuxedDataIfNeed(DemuxerStream::AUDIO);
    ReadDemuxedDataIfNeed(DemuxerStream::VIDEO);
#if defined(OS_TIZEN_TV_PRODUCT)
    if (hdr_info_.compare(configs.webm_hdr_info) != 0) {
      video_extra_info_.is_hdr_changed = true;
    }
    LOG(INFO) << "video_extra_info_.is_hdr_changed_  :"
              << video_extra_info_.is_hdr_changed;

    hdr_info_ = configs.webm_hdr_info;
    video_extra_info_.hdr_info = hdr_info_.c_str();

    UpdateResolution(configs.video_size.width(), configs.video_size.height(),
                     media_type);
#endif
    return;
  }

  CHECK(!GetMediaFormat(DemuxerStream::AUDIO))
      << "|audio_format_| is already initialized!";
  CHECK(!GetMediaFormat(DemuxerStream::VIDEO))
      << "|video_format_| is already initialized!";

  if (media_type & MediaType::Audio) {
    int samplerate = configs.audio_sampling_rate;
    int channel = configs.audio_channels;
    int bit_rate = configs.audio_bit_rate;

    ScopedMediaFormat audio_format;
    media_format_h temp_format = nullptr;

    int ret = media_format_create(&temp_format);
    if (ret != MEDIA_FORMAT_ERROR_NONE) {
      LOG(ERROR) << "media_format_create : 0x" << ret;
      return;
    }
    audio_format.reset(temp_format);

    media_format_mimetype_e audiomimeType = MEDIA_FORMAT_MP3;
    if (configs.audio_codec == kCodecAAC)
      audiomimeType = MEDIA_FORMAT_AAC;
    else if (configs.audio_codec == kCodecOpus)
      audiomimeType = MEDIA_FORMAT_OPUS;
#if defined(TIZEN_MULTIMEDIA_WEBM_SUPPORT)
    else if (configs.audio_codec == kCodecVorbis)
      audiomimeType = MEDIA_FORMAT_VORBIS;
#endif
#if BUILDFLAG(ENABLE_AC3_EAC3_AUDIO_DEMUXING)
    else if (configs.audio_codec == kCodecEAC3)
      audiomimeType = MEDIA_FORMAT_EAC3;
    else if (configs.audio_codec == kCodecAC3)
      audiomimeType = MEDIA_FORMAT_AC3;
#endif

    media_format_set_audio_mime(audio_format.get(), audiomimeType);
    media_format_set_audio_channel(audio_format.get(), channel);
    media_format_set_audio_samplerate(audio_format.get(), samplerate);
    media_format_set_audio_avg_bps(audio_format.get(), bit_rate);

#if defined(OS_TIZEN_TV_PRODUCT)
    audio_codec_extra_data_ = configs.audio_extra_data;
    audio_extra_info_.extradata_size = audio_codec_extra_data_.size();
    audio_extra_info_.codec_extradata =
        (audio_extra_info_.extradata_size == 0)
            ? nullptr
            : const_cast<uint8_t*>(audio_codec_extra_data_.data());
    // Drm type should be always set to PLAYER_DRM_TYPE_EME even for clean
    // content. It is possible that clean content is followed by encrypted e.i.
    // when a stream starts from an adverisement. This situation cannot be
    // detected when the first config is sent. In that case setting content to
    // encrypted in advance allows to avoid costly player reinitialization while
    // switching between clean and encrypted content.
    if (is_video_hole_) {
      audio_extra_info_.drm_type = drm_type_;
    } else {
      audio_extra_info_.drm_type = configs.is_audio_encrypted
                                       ? PLAYER_DRM_TYPE_EME
                                       : PLAYER_DRM_TYPE_NONE;
    }
    media_format_set_extra(audio_format.get(), &audio_extra_info_);
#endif
    SetMediaFormat(audio_format.release(), DemuxerStream::AUDIO);

    max_audio_buffer_size_ = GetMaxBufferSize(DemuxerStream::AUDIO, configs);
    buffer_observer_->SetBufferSize(DemuxerStream::AUDIO,
                                    max_audio_buffer_size_);
    LOG(INFO) << "AUDIO Buffer Size: " << max_audio_buffer_size_ / 1024 << "KB";
    if (!SetAudioStreamInfo())
      return;
  }

  if (media_type & MediaType::Video) {
    width_ = configs.video_size.width();
    height_ = configs.video_size.height();

    ScopedMediaFormat video_format;
    media_format_h temp_format = nullptr;
    int ret = media_format_create(&temp_format);
    if (ret != MEDIA_FORMAT_ERROR_NONE) {
      LOG(ERROR) << "media_format_create : 0x" << ret;
      return;
    }
    video_format.reset(temp_format);
    media_format_mimetype_e videomimeType = MEDIA_FORMAT_H264_SP;
#if defined(TIZEN_MULTIMEDIA_WEBM_SUPPORT)
    if (configs.video_codec == kCodecVP9) {
      videomimeType = MEDIA_FORMAT_VP9;
    } else if (configs.video_codec == kCodecVP8) {
      videomimeType = MEDIA_FORMAT_VP8;
    }
#endif
#if BUILDFLAG(ENABLE_HEVC_DEMUXING)
    if (configs.video_codec == kCodecHEVC)
      videomimeType = MEDIA_FORMAT_HEVC;
#endif
    media_format_set_video_mime(video_format.get(), videomimeType);
    media_format_set_video_width(video_format.get(), width_);
    media_format_set_video_height(video_format.get(), height_);

#if defined(OS_TIZEN_TV_PRODUCT)
    if (hdr_info_.compare(configs.webm_hdr_info) != 0) {
      video_extra_info_.is_hdr_changed = true;
    }
    LOG(INFO) << "video_extra_info_.is_hdr_changed_  :"
              << video_extra_info_.is_hdr_changed;

    hdr_info_ = configs.webm_hdr_info;
    video_extra_info_.hdr_info = hdr_info_.c_str();

    if (configs.framerate_num > 0 && configs.framerate_den > 0) {
      video_extra_info_.framerate_num = configs.framerate_num;
      video_extra_info_.framerate_den = configs.framerate_den;
    } else {
      video_extra_info_.framerate_den = kVideoFramerateDen;
      video_extra_info_.framerate_num = kVideoFramerateNum;
    }
    // In case of bigger than 60fps (> 60), web media should adjust to 60
    if (video_extra_info_.framerate_num && video_extra_info_.framerate_den &&
        static_cast<double>(video_extra_info_.framerate_num) /
                static_cast<double>(video_extra_info_.framerate_den) >
            kMaxFramerate) {
      video_extra_info_.framerate_num = kMaxFramerate;
      video_extra_info_.framerate_den = 1;
    }
    LOG(INFO) << "Framerate: [" << video_extra_info_.framerate_num << "] ["
              << video_extra_info_.framerate_den << "]"
              << " [" << configs.is_framerate_changed << "]";

    int max_width = kFHDVideoMaxWidth;
    int max_height = kFHDVideoMaxHeight;
    if (videomimeType == MEDIA_FORMAT_VP9 ||
        videomimeType == MEDIA_FORMAT_HEVC) {
      if ((system_info_get_value_int(SYSTEM_INFO_KEY_PANEL_RESOLUTION_WIDTH,
                                     &max_width) != SYSTEM_INFO_ERROR_NONE) ||
          (system_info_get_value_int(SYSTEM_INFO_KEY_PANEL_RESOLUTION_HEIGHT,
                                     &max_height) != SYSTEM_INFO_ERROR_NONE)) {
        LOG(ERROR) << "System info error, Fail to get panel resolution";
        max_width = kFHDVideoMaxWidth;
        max_height = kFHDVideoMaxHeight;
      }

      // For VP9 codec, the decoder only support up to 4K resolution.
      // If the max width is bigger than 4K, it can't find decoder,
      // so we have to transform the resolution info to 4K to make
      // the video playback, and it won't be 8K except H.265
      if (videomimeType == MEDIA_FORMAT_VP9) {
        max_width =
            (max_width <= k4KVideoMaxWidth) ? max_width : k4KVideoMaxWidth;
        max_height =
            (max_height <= k4KVideoMaxHeight) ? max_height : k4KVideoMaxHeight;
      }
    }

    video_extra_info_.max_width = max_width;
    video_extra_info_.max_height = max_height;
    video_extra_info_.hdr_mode = MEDIA_STREAM_HDR_TYPE_MATROSKA;
    video_extra_info_.is_framerate_changed = configs.is_framerate_changed;
    video_codec_extra_data_ = configs.video_extra_data;
    video_extra_info_.extradata_size = video_codec_extra_data_.size();
    video_extra_info_.codec_extradata =
        (video_extra_info_.extradata_size == 0)
            ? nullptr
            : const_cast<uint8_t*>(video_codec_extra_data_.data());
    // Drm type should be always set to PLAYER_DRM_TYPE_EME even for clean
    // content. It is possible that clean content is followed by encrypted e.i.
    // when a stream starts from an adverisement. This situation cannot be
    // detected when the first config is sent. In that case setting content to
    // encrypted in advance allows to avoid costly player reinitialization while
    // switching between clean and encrypted content.
    if (is_video_hole_) {
      video_extra_info_.drm_type = drm_type_;
    } else {
      video_extra_info_.drm_type = configs.is_video_encrypted
                                       ? PLAYER_DRM_TYPE_EME
                                       : PLAYER_DRM_TYPE_NONE;
    }
    media_format_set_extra(video_format.get(), &video_extra_info_);
#else
    if (configs.duration_ms > 0) {
      media_format_set_video_frame_rate(video_format.get(),
                                        (1000 / configs.duration_ms));
    }
#endif

    SetMediaFormat(video_format.release(), DemuxerStream::VIDEO);
    max_video_buffer_size_ = GetMaxBufferSize(DemuxerStream::VIDEO, configs);
    if (!SetVideoStreamInfo())
      return;
  }

  if (!is_demuxer_state_reported_) {
    manager()->OnMediaDataChange(GetPlayerId(), width_, height_, media_type);
    manager()->OnReadyStateChange(
        GetPlayerId(), blink::WebMediaPlayer::kReadyStateHaveMetadata);
    is_demuxer_state_reported_ = true;
    reported_ready_state_ = blink::WebMediaPlayer::kReadyStateHaveMetadata;
  }

  ChangePlayerState(PLAYER_STATE_READY);
}

// Helper method that prints error occured with CAPI.
void MediaSourcePlayerCapi::OnMediaError(MediaError error_type) {
  StopCurrentTimeUpdateTimer();
  MediaPlayerEfl::OnMediaError(error_type);
}

void MediaSourcePlayerCapi::ReadDemuxedDataIfNeed(
    media::DemuxerStream::Type type) {
  if (!IsEos(type) && ShouldFeed(type)) {
    demuxer_->RequestDemuxerData(type);
  }
}

void MediaSourcePlayerCapi::HandleBufferingStatusChanged(
    media::DemuxerStream::Type type,
    BufferStatus status) {
  DLOG(INFO) << __FUNCTION__ << " "
             << media::DemuxerStream::ConvertTypeToString(type) << ": "
             << ::GetString(status);

  switch (status) {
    case BUFFER_UNDERRUN:
    case BUFFER_MIN_THRESHOLD:
      SetShouldFeed(type, true);
      ReadDemuxedDataIfNeed(type);
      break;
    case BUFFER_MAX_THRESHOLD:
    case BUFFER_OVERFLOW:
    case BUFFER_EOS:
      SetShouldFeed(type, false);
      break;
    case BUFFER_NORMAL:
      break;
    case BUFFER_NONE:
      NOTREACHED();
  }

#if !defined(OS_TIZEN_TV_PRODUCT)
  // On mobile profile, CAPI player may not emit actual usable buffer level.
  // The feeding stops and playback stalls. So, here we try to initiate play
  // when buffer levels are above MAX threshold.
  if (!is_paused_ && paused_by_buffering_logic_ && player_prepared_ &&
      (!(GetMediaType() & MEDIA_AUDIO_MASK) ||
       buffer_observer_->AudioStatus() >= BUFFER_MAX_THRESHOLD) &&
      (!(GetMediaType() & MEDIA_VIDEO_MASK) ||
       buffer_observer_->VideoStatus() >= BUFFER_MAX_THRESHOLD)) {
    LOG(INFO) << "Initiate playback as buffer level is sufficient."
              << "\ttime : " << GetCurrentTime();
    PlayInternal();
  }
#endif
}

void MediaSourcePlayerCapi::OnResumeComplete(bool success) {
  LOG(INFO) << ", success: " << success;
  if (success) {
    manager()->OnResumeComplete(GetPlayerId());
    if (should_seek_after_resume_) {
      const auto current_time = GetCurrentTime();
#if defined(OS_TIZEN_TV_PRODUCT)
      UpdateCurrentTime(current_time);
#endif
      manager()->OnTimeUpdate(GetPlayerId(), current_time);
    } else {
#if defined(OS_TIZEN_TV_PRODUCT)
      UpdateCurrentTime(seek_offset_);
#endif
      manager()->OnTimeUpdate(GetPlayerId(), seek_offset_);
    }
  }
}

void MediaSourcePlayerCapi::OnInitComplete(bool success) {
  LOG(INFO) << "OnInitComplete success: " << success;
  manager()->OnInitComplete(GetPlayerId(), success);
}

void MediaSourcePlayerCapi::OnDemuxerDataAvailable(
    base::SharedMemoryHandle foreign_memory_handle,
    const media::DemuxedBufferMetaData& meta_data) {
  static media::DemuxedBufferMetaData dbg_audio_meta;

  if (meta_data.end_of_stream) {
    ReadFromQueueIfNeeded(meta_data.type);
    buffer_observer_->EOSReached(meta_data.type);
    SendEosToCapi(meta_data);
    return;
  }

  DCHECK(meta_data.status == media::DemuxerStream::kOk);

  if (meta_data.size <= 0) {
    LOG(ERROR) << "ERROR : Size of shared memory is Zero";
    return;
  }

  // make sure we push VIDEO data to CAPI first
  if (!seeking_video_pushed_) {
    if (meta_data.type == DemuxerStream::VIDEO) {
      seeking_video_pushed_ = true;
      SetShouldFeed(DemuxerStream::AUDIO, true);
      ReadDemuxedDataIfNeed(DemuxerStream::AUDIO);
    } else if (meta_data.type == DemuxerStream::AUDIO) {
      SetShouldFeed(DemuxerStream::AUDIO, false);
    }
  }

  if (!ShouldFeed(meta_data.type)) {
    // Why store the DecoderBuffer? we have requested for buffer
    // from demuxer but CAPI player asked to stop. So need to save
    // this buffer and use it on next |ReadDemuxedDataIfNeed| call.
    SaveDecoderBuffer(foreign_memory_handle, meta_data);
    return;
  }

  ReadFromQueueIfNeeded(meta_data.type);

  auto media_packet = CreateMediaPacket(foreign_memory_handle, meta_data);
  auto ret = PushMediaPacket(*media_packet);
  if (ret == SendMediaPacketResult::BUFFER_SPACE) {
    LOG(ERROR) << "|PLAYER_ERROR_BUFFER_SPACE|. Shouldn't feed";
    SetShouldFeed(meta_data.type, false);
    SaveDecoderBuffer(foreign_memory_handle, meta_data);
    return;
  } else if (ret != SendMediaPacketResult::SUCCESS) {
    LOG(ERROR) << "player_push_media_stream failed : " << ret;
    // TODO(sam): Do we need to call |SaveDecoderBuffer| here except
    // |PLAYER_ERROR_BUFFER_SPACE| ?
#if defined(OS_TIZEN_TV_PRODUCT)
    media::ReleaseTzHandle(meta_data.tz_handle, meta_data.size);
#endif
  }

  ReadDemuxedDataIfNeed(meta_data.type);
  if (paused_by_buffering_logic_ && seek_state_ == MEDIA_SEEK_NONE) {
    UncheckedUpdateReadyState();
  }
}

void MediaSourcePlayerCapi::SendEosToCapi(
    const media::DemuxedBufferMetaData& meta_data) {
  const auto type = meta_data.type;
  if (IsEos(type)) {
    LOG(ERROR) << "Already " << DemuxerStream::ConvertTypeToString(type)
               << " EOS reached, but buffer comes: " << meta_data.size
               << "bytes";
    return;
  }
  auto media_packet = CreateEosMediaPacket(type);
  if (!media_packet->PrepareAndGet()) {
    LOG(ERROR) << "Creating " << *media_packet << "failed";
    return;
  }
  SetEos(type, true);
  PushMediaPacket(*media_packet);
}

void MediaSourcePlayerCapi::OnPrepareComplete(void* user_data) {
  LOG(INFO) << "CAPI player is ready. Prerolling...";

  MediaSourcePlayerCapi* player =
      static_cast<MediaSourcePlayerCapi*>(user_data);
  player->task_runner_->PostTask(
      FROM_HERE, base::Bind(&MediaSourcePlayerCapi::HandlePrepareComplete,
                            player->weak_factory_.GetWeakPtr()));
}

void MediaSourcePlayerCapi::OnPlaybackComplete(void* user_data) {
  MediaSourcePlayerCapi* player =
      static_cast<MediaSourcePlayerCapi*>(user_data);

  player->task_runner_->PostTask(
      FROM_HERE, base::Bind(&MediaSourcePlayerCapi::HandlePlaybackComplete,
                            player->weak_factory_.GetWeakPtr()));
}

void MediaSourcePlayerCapi::OnPlayerError(int error_code, void* user_data) {
  MediaSourcePlayerCapi* player =
      static_cast<MediaSourcePlayerCapi*>(user_data);
  player->task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&MediaSourcePlayerCapi::HandlePlayerError,
                 player->weak_factory_.GetWeakPtr(), error_code, FROM_HERE));
}

void MediaSourcePlayerCapi::OnInterrupted(player_interrupted_code_e code,
                                          void* user_data) {
  if (code != PLAYER_INTERRUPTED_BY_RESOURCE_CONFLICT)
    return;

  MediaSourcePlayerCapi* player =
      static_cast<MediaSourcePlayerCapi*>(user_data);

  LOG(INFO) << "Resource Conflict : player[" << player->GetPlayerId() << "]";
#if !defined(OS_TIZEN_TV_PRODUCT)
  player->task_runner_->PostTask(
      FROM_HERE, base::Bind(&MediaSourcePlayerCapi::Suspend,
                            player->weak_factory_.GetWeakPtr()));
#else
  if (player) {
    player->task_runner_->PostTask(
        FROM_HERE, base::Bind(&MediaSourcePlayerCapi::OnPlayerResourceConflict,
                              player->weak_factory_.GetWeakPtr()));
  }
#endif
}

void MediaSourcePlayerCapi::OnSeekComplete(void* user_data) {
  MediaSourcePlayerCapi* player =
      static_cast<MediaSourcePlayerCapi*>(user_data);
  player->task_runner_->PostTask(
      FROM_HERE, base::Bind(&MediaSourcePlayerCapi::HandleSeekComplete,
                            player->weak_factory_.GetWeakPtr()));
}

void MediaSourcePlayerCapi::HandlePlayerError(int player_error_code,
                                              const base::Location& location) {
  LOG(ERROR) << GetString(static_cast<player_error_e>(player_error_code))
             << " from " << location.function_name() << "("
             << location.line_number() << ")";

  SetShouldFeed(DemuxerStream::AUDIO, false);
  SetShouldFeed(DemuxerStream::VIDEO, false);

  OnMediaError(GetMediaError(player_error_code));
  OnPlayerSuspendRequest();
}

void MediaSourcePlayerCapi::SeekDone() {
  UpdateSeekState(MEDIA_SEEK_NONE);
  const auto current_time = GetCurrentTime();
  manager()->OnTimeUpdate(GetPlayerId(), current_time);
  manager()->OnSeekComplete(GetPlayerId());
  UncheckedUpdateReadyState();
#if defined(OS_TIZEN_TV_PRODUCT)
  UpdateCurrentTime(current_time);
#endif
}

void MediaSourcePlayerCapi::HandleSeekComplete() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  pending_seek_on_player_ = false;
  if (GetPlayerState() < PLAYER_STATE_READY) {
    LOG(INFO) << "Player not in ready state " << GetPlayerId();
    return;
  }
  if (pending_demuxer_seek_position_ != media::kNoTimestamp) {
    base::TimeDelta last_demuxer_seek_time{pending_demuxer_seek_position_};
    pending_demuxer_seek_position_ = media::kNoTimestamp;
    UpdateSeekState(MEDIA_SEEK_NONE);
    Seek(last_demuxer_seek_time);
    return;
  }

  SeekDone();
  // Initiate state change.
  ExecuteDelayedPlayerState();
}

// This API to run loop with a delay of 10ms till state
// change or error to confirm the operation.
//
// This is needed as CAPI cannot handle multiple state
// change calls.
void MediaSourcePlayerCapi::ConfirmStateChange() {
  // We might be inbetween OnPrepareComplete and HandlePrepareComplete
  if (GetPlayerState() == player_new_state_ &&
      (player_new_state_ != PLAYER_STATE_READY || player_prepared_)) {
    LOG(INFO) << "State change to " << GetString(player_new_state_)
              << " confirmed!";
    player_new_state_ = PLAYER_STATE_NONE;
    ExecuteDelayedPlayerState();
    return;
  }

  // Async state change may have been aborted
  if (player_new_state_ != PLAYER_STATE_NONE) {
    task_runner_->PostDelayedTask(
        FROM_HERE,
        base::Bind(&MediaSourcePlayerCapi::ConfirmStateChange,
                   weak_factory_.GetWeakPtr()),
        kStateChangeConfirmationInterval);
  }
}

void MediaSourcePlayerCapi::ExecuteDelayedPlayerState() {
  if (GetPlayerState() == delayed_player_state_)
    SetDelayedPlayerState(PLAYER_STATE_NONE);

  if (delayed_player_state_ == PLAYER_STATE_NONE)
    return;

  LOG(INFO) << "Execute delayed state change to "
            << GetString(delayed_player_state_);
  ChangePlayerState(delayed_player_state_);
}

void MediaSourcePlayerCapi::Suspend() {
  LOG(INFO) << "Suspending the player (paused_by_buffering_logic_ : "
            << paused_by_buffering_logic_ << ", playing_: " << !is_paused_
            << "). ID:" << GetPlayerId();

  if (IsPlayerSuspended()) {
    LOG(INFO) << "Player " << GetPlayerId() << "already suspended";
    return;
  }

  pending_playback_rate_ = playback_rate_;
  playback_rate_ = 1.0;

  Release();
  MediaPlayerEfl::Suspend();
}

void MediaSourcePlayerCapi::Resume() {
  LOG(INFO) << "paused_by_buffering_logic_ : " << paused_by_buffering_logic_
            << "HasConfigs:" << HasConfigs();
  MediaPlayerEfl::Resume();
  if (!HasConfigs()) {
    // We don't have configs so we were not in PLAYER_STATE_READY before the
    // suspend.
    Initialize();
    OnResumeComplete(true);
    return;
  }

  if (!Prepare(base::Bind(&MediaSourcePlayerCapi::OnResumeComplete,
                          weak_factory_.GetWeakPtr()))) {
    RunCompleteCB(false, FROM_HERE);
    return;
  }

  if (!SetAudioStreamInfo())
    return;
  if (!SetVideoStreamInfo())
    return;
  if (!ChangePlayerState(PLAYER_STATE_READY))
    return;
#if defined(OS_TIZEN_TV_PRODUCT)
  position_before_conflict_ = 0;
#endif
  should_seek_after_resume_ = false;
}

void MediaSourcePlayerCapi::SetDelayedPlayerState(player_state_e state) {
  if (delayed_player_state_ == state)
    return;

  delayed_player_state_ = state;
}

//  Here's the state diagram that describes CAPI player lifetime.
//
//            [ PLAYER_STATE_NONE ]
//                   ^    |
//        player_destroy player_create
//                   |    v
//      ----->[ PLAYER_STATE_IDLE ]<---------------
//      |            ^    |                       |
//     player_unprepare  player_prepare_async     |
//      |            |    v                       |
//      |     [ PLAYER_STATE_READY ]<---|         |
//      |            ^    |             | player_unprepare
//      |    player_stop player_start   |         |
//      |            |    |             |         |
//      |    ---------    -----------   |         |
//      |    |        player_pause  | player_stop |
//      |    |           <-------   v   |         |
// [ PLAYER_STATE_PAUSED ]     [ PLAYER_STATE_PLAYING ]
//                       ------->
//                     player_start
bool MediaSourcePlayerCapi::ChangePlayerState(player_state_e state) {
  SetDelayedPlayerState(PLAYER_STATE_NONE);

  if (GetPlayerState() == state && player_new_state_ == PLAYER_STATE_NONE) {
    LOG(INFO) << "Ignore state change request as player is already in : "
              << GetString(state);

    // Multiple PLAY/PAUSE requests can be triggered during rapid seek.
    // Reset delayed state handler to avoid moving CAPI player to an
    // invalid state.
    return true;
  }

  if (PLAYER_STATE_READY == player_new_state_ && PLAYER_STATE_IDLE == state) {
    // We need to destroy the player if we want to abort player_prepare
    player_new_state_ = PLAYER_STATE_NONE;
    player_unprepare(player_);
    return true;
  }

  if (player_new_state_ != PLAYER_STATE_NONE ||
      (seek_state_ != MEDIA_SEEK_NONE && state >= PLAYER_STATE_PLAYING)) {
    LOG(INFO) << "Inprogress state change : " << GetString(player_new_state_)
              << ", Requested state : " << GetString(state)
              << ", seek_state : " << GetString(seek_state_);
    SetDelayedPlayerState(state);
    return true;
  }

  int error = 0;

  // Error handling code is duplicated to avoid rework on
  // http://165.213.202.130/gerrit/#/c/100742/
  //
  // TODO: Find better way to inform player API name
  // on error instead of just line number.
  switch (state) {
    case PLAYER_STATE_IDLE:
      if (GetPlayerState() == PLAYER_STATE_NONE) {
        error = player_create(&player_);
        if (error != PLAYER_ERROR_NONE) {
          HandlePlayerError(error, FROM_HERE);
          return false;
        }
#if defined(OS_TIZEN_TV_PRODUCT)
        error = player_set_error_cb(player_,
                                    MediaSourcePlayerCapi::OnPlayerError, this);
        if (error != PLAYER_ERROR_NONE) {
          HandlePlayerError(error, FROM_HERE);
          return false;
        }
#if defined(TIZEN_VIDEO_HOLE)
        // We must reset display options of the player
        if (is_video_hole_ && video_plane_manager_) {
          error = video_plane_manager_->Initialize();
          if (error != PLAYER_ERROR_NONE) {
            HandlePlayerError(error, FROM_HERE);
            return false;
          }
        }
#endif  // initialize video_plane_manager
#endif
      } else if (GetPlayerState() >= PLAYER_STATE_READY) {
        error = player_unprepare(player_);
#if defined(OS_TIZEN_TV_PRODUCT)
        NotifyPlaybackState(blink::WebMediaPlayer::kPlaybackStop);
#endif
        if (error != PLAYER_ERROR_NONE) {
          HandlePlayerError(error, FROM_HERE);
          return false;
        }
      }
      player_prepared_ = false;
      break;
    case PLAYER_STATE_READY:
      if (GetPlayerState() == PLAYER_STATE_IDLE) {
#if defined(OS_TIZEN_TV_PRODUCT)
        NotifyPlaybackState(blink::WebMediaPlayer::kPlaybackReady);
#endif
        if (!SetPlayerIniParam())
          return false;
        error = player_prepare_async(
            player_, MediaSourcePlayerCapi::OnPrepareComplete, this);
        if (error != PLAYER_ERROR_NONE) {
          LOG(ERROR) << "player prepare failed : 0x " << error;
          return false;
        }
      } else if (GetPlayerState() >= PLAYER_STATE_PLAYING) {
        player_stop(player_);
      }
      break;
    case PLAYER_STATE_PLAYING:
      error = player_start(player_);
      if (error != PLAYER_ERROR_NONE) {
        HandlePlayerError(error, FROM_HERE);
        return false;
      }
#if defined(OS_TIZEN_TV_PRODUCT)
      NotifyPlaybackState(blink::WebMediaPlayer::kPlaybackStart);
#endif
      break;
    case PLAYER_STATE_PAUSED:
      if (GetPlayerState() == PLAYER_STATE_PLAYING) {
        error = player_pause(player_);
        if (error != PLAYER_ERROR_NONE) {
          HandlePlayerError(error, FROM_HERE);
          return false;
        }
      } else {
        LOG(ERROR) << "Can't pause CAPI player in state : "
                   << GetString(GetPlayerState());
        return false;
      }
      break;
    case PLAYER_STATE_NONE:
    default:
      NOTIMPLEMENTED();
  }

  if (GetPlayerState() == state) {
    LOG(INFO) << "CAPI Player state changed to : " << GetString(state);
#if defined(OS_TIZEN_TV_PRODUCT)
    if (state == PLAYER_STATE_PLAYING &&
        !DOUBLE_EQUAL(pending_playback_rate_, kInvalidPlayRate)) {
      SetRate(pending_playback_rate_);
      pending_playback_rate_ = kInvalidPlayRate;
    }
#endif
  } else {
    player_new_state_ = state;
    ConfirmStateChange();
  }

  return true;
}

void MediaSourcePlayerCapi::EnteredFullscreen() {
  MediaPlayerEfl::EnteredFullscreen();

#if defined(TIZEN_VIDEO_HOLE) && !defined(OS_TIZEN_TV_PRODUCT)
  if (GetPlayerState() == PLAYER_STATE_NONE || !is_video_hole_)
    return;

  int ret = player_enable_media_packet_video_frame_decoded_cb(player_, false);
  if (ret != PLAYER_ERROR_NONE) {
    HandlePlayerError(ret, FROM_HERE);
    return;
  }
#endif
}

void MediaSourcePlayerCapi::ExitedFullscreen() {
  MediaPlayerEfl::ExitedFullscreen();

#if defined(TIZEN_VIDEO_HOLE) && !defined(OS_TIZEN_TV_PRODUCT)
  if (GetPlayerState() == PLAYER_STATE_NONE || !is_video_hole_)
    return;

  int ret = player_enable_media_packet_video_frame_decoded_cb(player_, true);
  if (ret != PLAYER_ERROR_NONE) {
    HandlePlayerError(ret, FROM_HERE);
    return;
  }
#endif
}

bool MediaSourcePlayerCapi::SetPlayerIniParam() {
  return true;
}

PlayerRoleFlags MediaSourcePlayerCapi::GetRoles() const noexcept {
  return PlayerRole::ElementaryStreamBased;
}

#if defined(OS_TIZEN_TV_PRODUCT)
void MediaSourcePlayerCapi::SetDecryptor(eme::eme_decryptor_t decryptor) {
  if (decryptor == 0) {
    HandlePlayerError(PLAYER_ERROR_DRM_EXPIRED, FROM_HERE);
  }
}
#endif

}  // namespace media
