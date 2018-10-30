// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/media_player/media_player_resource.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "base/logging.h"
#include "base/run_loop.h"
#include "base/synchronization/lock.h"
#include "ppapi/proxy/dispatch_reply_message.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/proxy/resource_callback_helpers.h"
#include "ppapi/proxy/resource_message_params.h"
#include "ppapi/shared_impl/array_writer.h"
#include "ppapi/shared_impl/ppapi_globals.h"
#include "ppapi/shared_impl/proxy_lock.h"
#include "ppapi/shared_impl/resource_tracker.h"

#include "ppapi/proxy/plugin_globals.h"
#include "ppapi/thunk/ppb_instance_api.h"

namespace ppapi {
namespace proxy {

namespace {

bool IsValid(const PP_Rect* rect) {
  return (rect->size.width > 0) && (rect->size.height > 0);
}

template <typename T>
class CallbackRunner {
 public:
  CallbackRunner() = delete;
  CallbackRunner(const T* listener, void* user_data_);
  virtual ~CallbackRunner();

  void SetListener(const T*, void* user_data);

  virtual void OnHandleReply(const proxy::ResourceMessageReplyParams& params,
                             const IPC::Message& msg) = 0;

 protected:
  void DispatchMessageOnTargetThread(
      const base::Location& from_here,
      const proxy::ResourceMessageReplyParams& params,
      const IPC::Message& msg);

  const T* listener_;
  void* user_data_;
  base::WeakPtrFactory<CallbackRunner> weak_factory_;
  base::WeakPtr<CallbackRunner> weak_ptr_;

 private:
  scoped_refptr<MessageLoopShared> target_loop_;
  DISALLOW_COPY_AND_ASSIGN(CallbackRunner);
};

template <typename T>
CallbackRunner<T>::CallbackRunner(const T* listener, void* user_data)
    : listener_(listener),
      user_data_(user_data),
      weak_factory_(this),
      weak_ptr_(weak_factory_.GetWeakPtr()),
      target_loop_(PpapiGlobals::Get()->GetCurrentMessageLoop()) {}

template <typename T>
CallbackRunner<T>::~CallbackRunner() = default;

template <typename T>
void CallbackRunner<T>::SetListener(const T* listener, void* user_data) {
  listener_ = listener;
  user_data_ = user_data;
  weak_factory_.InvalidateWeakPtrs();
  weak_ptr_ = weak_factory_.GetWeakPtr();
  target_loop_ = PpapiGlobals::Get()->GetCurrentMessageLoop();
}

template <typename T>
void CallbackRunner<T>::DispatchMessageOnTargetThread(
    const base::Location& from_here,
    const proxy::ResourceMessageReplyParams& params,
    const IPC::Message& msg) {
  if (target_loop_)
    target_loop_->PostClosure(
        from_here,
        base::Bind(&CallbackRunner::OnHandleReply, weak_ptr_, params, msg), 0);
  else
    PpapiGlobals::Get()->GetMainThreadMessageLoop()->PostTask(
        from_here,
        base::Bind(&CallbackRunner::OnHandleReply, weak_ptr_, params, msg));
}

}  // namespace

class MediaPlayerResource::MediaEventsListener
    : public CallbackRunner<PPP_MediaEventsListener_Samsung> {
 public:
  typedef PpapiHostMsg_MediaPlayer_SetMediaEventsListener SetListenerMessage;

  MediaEventsListener(const PPP_MediaEventsListener_Samsung*, void* user_data);
  ~MediaEventsListener();

  void OnHandleReply(const proxy::ResourceMessageReplyParams& params,
                     const IPC::Message& msg) override;

  bool HandleReply(const proxy::ResourceMessageReplyParams& params,
                   const IPC::Message& msg);

 private:
  void OnTimeUpdate(const ResourceMessageReplyParams&, PP_TimeTicks);
  void OnEnded(const ResourceMessageReplyParams&);
  void OnError(const ResourceMessageReplyParams&, PP_MediaPlayerError);

  DISALLOW_COPY_AND_ASSIGN(MediaEventsListener);
};

class MediaPlayerResource::SubtitleListener
    : public CallbackRunner<PPP_SubtitleListener_Samsung> {
 public:
  typedef PpapiHostMsg_MediaPlayer_SetSubtitleListener SetListenerMessage;

  SubtitleListener(const PPP_SubtitleListener_Samsung*, void* user_data);
  ~SubtitleListener();

  void OnHandleReply(const proxy::ResourceMessageReplyParams& params,
                     const IPC::Message& msg) override;

  bool HandleReply(const proxy::ResourceMessageReplyParams& params,
                   const IPC::Message& msg);

 private:
  void OnShowSubtitle(const ResourceMessageReplyParams&,
                      PP_TimeDelta duration,
                      const std::string& text);

  DISALLOW_COPY_AND_ASSIGN(SubtitleListener);
};

class MediaPlayerResource::BufferingListener
    : public CallbackRunner<PPP_BufferingListener_Samsung> {
 public:
  typedef PpapiHostMsg_MediaPlayer_SetBufferingListener SetListenerMessage;

  BufferingListener(const PPP_BufferingListener_Samsung*, void* user_data);
  ~BufferingListener();

  void OnHandleReply(const proxy::ResourceMessageReplyParams& params,
                     const IPC::Message& msg) override;

  bool HandleReply(const proxy::ResourceMessageReplyParams& params,
                   const IPC::Message& msg);

 private:
  void OnBufferingStart(const ResourceMessageReplyParams&);
  void OnBufferingProgress(const ResourceMessageReplyParams&, uint32_t);
  void OnBufferingComplete(const ResourceMessageReplyParams&);

  DISALLOW_COPY_AND_ASSIGN(BufferingListener);
};

class MediaPlayerResource::DRMListener
    : public CallbackRunner<PPP_DRMListener_Samsung> {
 public:
  typedef PpapiHostMsg_MediaPlayer_SetDRMListener SetListenerMessage;

  DRMListener(const PPP_DRMListener_Samsung*, void* user_data);
  ~DRMListener();

  void OnHandleReply(const proxy::ResourceMessageReplyParams& params,
                     const IPC::Message& msg) override;

  bool HandleReply(const proxy::ResourceMessageReplyParams& params,
                   const IPC::Message& msg);

 private:
  void OnInitDataLoaded(const ResourceMessageReplyParams&,
                        PP_MediaPlayerDRMType,
                        const std::vector<uint8_t>& initData);
  void OnLicenseRequest(const ResourceMessageReplyParams&,
                        const std::vector<uint8_t>& request);

  DISALLOW_COPY_AND_ASSIGN(DRMListener);
};

MediaPlayerResource::MediaEventsListener::MediaEventsListener(
    const PPP_MediaEventsListener_Samsung* listener,
    void* user_data)
    : CallbackRunner<PPP_MediaEventsListener_Samsung>(listener, user_data) {}

MediaPlayerResource::MediaEventsListener::~MediaEventsListener() {}

bool MediaPlayerResource::MediaEventsListener::HandleReply(
    const proxy::ResourceMessageReplyParams& params,
    const IPC::Message& msg) {
  switch (msg.type()) {
    case PpapiPluginMsg_MediaEventsListener_OnTimeUpdate::ID:
    case PpapiPluginMsg_MediaEventsListener_OnEnded::ID:
    case PpapiPluginMsg_MediaEventsListener_OnError::ID:
      DispatchMessageOnTargetThread(FROM_HERE, params, msg);
      return true;
    default:
      return false;
  }
}

void MediaPlayerResource::MediaEventsListener::OnHandleReply(
    const proxy::ResourceMessageReplyParams& params,
    const IPC::Message& msg) {
  PPAPI_BEGIN_MESSAGE_MAP(MediaEventsListener, msg)
  PPAPI_DISPATCH_PLUGIN_RESOURCE_CALL(
      PpapiPluginMsg_MediaEventsListener_OnTimeUpdate, OnTimeUpdate)
  PPAPI_DISPATCH_PLUGIN_RESOURCE_CALL_0(
      PpapiPluginMsg_MediaEventsListener_OnEnded, OnEnded)
  PPAPI_DISPATCH_PLUGIN_RESOURCE_CALL(
      PpapiPluginMsg_MediaEventsListener_OnError, OnError)
  PPAPI_END_MESSAGE_MAP()
}

void MediaPlayerResource::MediaEventsListener::OnTimeUpdate(
    const ResourceMessageReplyParams& params,
    PP_TimeTicks time) {
  listener_->OnTimeUpdate(time, user_data_);
}

void MediaPlayerResource::MediaEventsListener::OnEnded(
    const ResourceMessageReplyParams& params) {
  listener_->OnEnded(user_data_);
}

void MediaPlayerResource::MediaEventsListener::OnError(
    const ResourceMessageReplyParams& params,
    PP_MediaPlayerError error) {
  listener_->OnError(error, user_data_);
}

MediaPlayerResource::SubtitleListener::SubtitleListener(
    const PPP_SubtitleListener_Samsung* listener,
    void* user_data)
    : CallbackRunner<PPP_SubtitleListener_Samsung>(listener, user_data) {}

MediaPlayerResource::SubtitleListener::~SubtitleListener() {}

bool MediaPlayerResource::SubtitleListener::HandleReply(
    const proxy::ResourceMessageReplyParams& params,
    const IPC::Message& msg) {
  switch (msg.type()) {
    case PpapiPluginMsg_SubtitleListener_OnShowSubtitle::ID:
      DispatchMessageOnTargetThread(FROM_HERE, params, msg);
      return true;
    default:
      return false;
  }
}

void MediaPlayerResource::SubtitleListener::OnHandleReply(
    const proxy::ResourceMessageReplyParams& params,
    const IPC::Message& msg) {
  PPAPI_BEGIN_MESSAGE_MAP(SubtitleListener, msg)
  PPAPI_DISPATCH_PLUGIN_RESOURCE_CALL(
      PpapiPluginMsg_SubtitleListener_OnShowSubtitle, OnShowSubtitle)
  PPAPI_END_MESSAGE_MAP()
}

void MediaPlayerResource::SubtitleListener::OnShowSubtitle(
    const ResourceMessageReplyParams& params,
    PP_TimeDelta duration,
    const std::string& text) {
  listener_->OnShowSubtitle(duration, text.data(), user_data_);
}

MediaPlayerResource::BufferingListener::BufferingListener(
    const PPP_BufferingListener_Samsung* listener,
    void* user_data)
    : CallbackRunner<PPP_BufferingListener_Samsung>(listener, user_data) {}

MediaPlayerResource::BufferingListener::~BufferingListener() {}

bool MediaPlayerResource::BufferingListener::HandleReply(
    const proxy::ResourceMessageReplyParams& params,
    const IPC::Message& msg) {
  switch (msg.type()) {
    case PpapiPluginMsg_BufferingListener_OnBufferingStart::ID:
    case PpapiPluginMsg_BufferingListener_OnBufferingProgress::ID:
    case PpapiPluginMsg_BufferingListener_OnBufferingComplete::ID:
      DispatchMessageOnTargetThread(FROM_HERE, params, msg);
      return true;
    default:
      return false;
  }
}

void MediaPlayerResource::BufferingListener::OnHandleReply(
    const proxy::ResourceMessageReplyParams& params,
    const IPC::Message& msg) {
  PPAPI_BEGIN_MESSAGE_MAP(BufferingListener, msg)
  PPAPI_DISPATCH_PLUGIN_RESOURCE_CALL_0(
      PpapiPluginMsg_BufferingListener_OnBufferingStart, OnBufferingStart)
  PPAPI_DISPATCH_PLUGIN_RESOURCE_CALL(
      PpapiPluginMsg_BufferingListener_OnBufferingProgress, OnBufferingProgress)
  PPAPI_DISPATCH_PLUGIN_RESOURCE_CALL_0(
      PpapiPluginMsg_BufferingListener_OnBufferingComplete, OnBufferingComplete)
  PPAPI_END_MESSAGE_MAP()
}

void MediaPlayerResource::BufferingListener::OnBufferingStart(
    const ResourceMessageReplyParams& params) {
  listener_->OnBufferingStart(user_data_);
}

void MediaPlayerResource::BufferingListener::OnBufferingProgress(
    const ResourceMessageReplyParams& params,
    uint32_t percent) {
  listener_->OnBufferingProgress(percent, user_data_);
}

void MediaPlayerResource::BufferingListener::OnBufferingComplete(
    const ResourceMessageReplyParams& params) {
  listener_->OnBufferingComplete(user_data_);
}

MediaPlayerResource::DRMListener::DRMListener(
    const PPP_DRMListener_Samsung* listener,
    void* user_data)
    : CallbackRunner<PPP_DRMListener_Samsung>(listener, user_data) {}

MediaPlayerResource::DRMListener::~DRMListener() {}

bool MediaPlayerResource::DRMListener::HandleReply(
    const proxy::ResourceMessageReplyParams& params,
    const IPC::Message& msg) {
  switch (msg.type()) {
    case PpapiPluginMsg_DRMListener_OnInitDataLoaded::ID:
    case PpapiPluginMsg_DRMListener_OnLicenseRequest::ID:
      DispatchMessageOnTargetThread(FROM_HERE, params, msg);
      return true;
    default:
      return false;
  }
}

void MediaPlayerResource::DRMListener::OnHandleReply(
    const proxy::ResourceMessageReplyParams& params,
    const IPC::Message& msg) {
  PPAPI_BEGIN_MESSAGE_MAP(DRMListener, msg)
  PPAPI_DISPATCH_PLUGIN_RESOURCE_CALL(
      PpapiPluginMsg_DRMListener_OnInitDataLoaded, OnInitDataLoaded)
  PPAPI_DISPATCH_PLUGIN_RESOURCE_CALL(
      PpapiPluginMsg_DRMListener_OnLicenseRequest, OnLicenseRequest)
  PPAPI_END_MESSAGE_MAP()
}

void MediaPlayerResource::DRMListener::OnInitDataLoaded(
    const ResourceMessageReplyParams& params,
    PP_MediaPlayerDRMType type,
    const std::vector<uint8_t>& initData) {
  listener_->OnInitdataLoaded(type, initData.size(), initData.data(),
                              user_data_);
}

void MediaPlayerResource::DRMListener::OnLicenseRequest(
    const ResourceMessageReplyParams& params,
    const std::vector<uint8_t>& request) {
  listener_->OnLicenseRequest(request.size(), request.data(), user_data_);
}

// static
PP_Resource MediaPlayerResource::Create(Connection connection,
                                        PP_Instance instance,
                                        PP_MediaPlayerMode player_mode,
                                        PP_BindToInstanceMode bind_mode) {
  scoped_refptr<MediaPlayerResource> player(
      new MediaPlayerResource(connection, instance, player_mode));
  if (bind_mode == PP_BINDTOINSTANCEMODE_BIND) {
    if (!player->BindToInstance())
      return 0;
  }
  return player->GetReference();
}

MediaPlayerResource::MediaPlayerResource(Connection connection,
                                         PP_Instance instance,
                                         PP_MediaPlayerMode player_mode)
    : PluginResource(connection, instance), data_source_(0) {
  SendCreate(BROWSER, PpapiHostMsg_MediaPlayer_Create(player_mode));
  SendCreate(RENDERER, PpapiHostMsg_MediaPlayer_Create(player_mode));
}

MediaPlayerResource::~MediaPlayerResource() {
  PostAbortIfNecessary(attach_data_source_callback_);
  PostAbortIfNecessary(play_callback_);
  PostAbortIfNecessary(pause_callback_);
  PostAbortIfNecessary(stop_callback_);
  PostAbortIfNecessary(seek_callback_);
  PostAbortIfNecessary(set_playback_rate_callback_);
  PostAbortIfNecessary(get_duration_callback_);
  PostAbortIfNecessary(get_current_time_callback_);
  PostAbortIfNecessary(get_player_state_callback_);
  PostAbortIfNecessary(get_current_video_track_info_callback_);
  PostAbortIfNecessary(get_video_tracks_list_callback_);
  PostAbortIfNecessary(get_current_audio_track_info_callback_);
  PostAbortIfNecessary(get_audio_tracks_list_callback_);
  PostAbortIfNecessary(get_current_text_track_info_callback_);
  PostAbortIfNecessary(get_text_tracks_list_callback_);
  PostAbortIfNecessary(select_track_callback_);
  PostAbortIfNecessary(set_subtitles_visible_callback_);
  PostAbortIfNecessary(add_external_subtitles_callback_);
  PostAbortIfNecessary(set_subtitles_delay_callback_);
  PostAbortIfNecessary(set_display_rect_callback_);
  PostAbortIfNecessary(set_drm_specific_data_callback_);
}

thunk::PPB_MediaPlayer_API* MediaPlayerResource::AsPPB_MediaPlayer_API() {
  return this;
}

void MediaPlayerResource::OnReplyReceived(
    const proxy::ResourceMessageReplyParams& params,
    const IPC::Message& msg) {
  if (events_listener_ && events_listener_->HandleReply(params, msg))
    return;

  if (subtitle_listener_ && subtitle_listener_->HandleReply(params, msg))
    return;

  if (buffering_listener_ && buffering_listener_->HandleReply(params, msg))
    return;

  if (drm_listener_ && drm_listener_->HandleReply(params, msg))
    return;

  PluginResource::OnReplyReceived(params, msg);
}

bool MediaPlayerResource::SetMediaEventsListener(
    const PPP_MediaEventsListener_Samsung* listener,
    void* user_data) {
  return SetListener(&events_listener_, listener, user_data);
}

bool MediaPlayerResource::SetSubtitleListener(
    const PPP_SubtitleListener_Samsung* listener,
    void* user_data) {
  return SetListener(&subtitle_listener_, listener, user_data);
}

bool MediaPlayerResource::SetBufferingListener(
    const PPP_BufferingListener_Samsung* listener,
    void* user_data) {
  return SetListener(&buffering_listener_, listener, user_data);
}

bool MediaPlayerResource::SetDRMListener(
    const PPP_DRMListener_Samsung* listener,
    void* user_data) {
  return SetListener(&drm_listener_, listener, user_data);
}

int32_t MediaPlayerResource::AttachDataSource(
    PP_Resource data_source,
    scoped_refptr<TrackedCallback> callback) {
  if (TrackedCallback::IsPending(attach_data_source_callback_)) {
    LOG(ERROR) << "AttachDataSource is already in progress";
    return PP_ERROR_INPROGRESS;
  }

  auto data_source_resource =
      PpapiGlobals::Get()->GetResourceTracker()->GetResource(data_source);
  if (!data_source_resource) {
    LOG(ERROR) << "Invalid DataSourceResource";
    return PP_ERROR_BADARGUMENT;
  }

  attach_data_source_callback_ = callback;
  data_source_ = 0;
  Call<PpapiPluginMsg_MediaPlayer_AttachDataSourceReply>(
      BROWSER, PpapiHostMsg_MediaPlayer_AttachDataSource(data_source),
      base::Bind(&MediaPlayerResource::OnAttachDataSourceReply,
                 base::Unretained(this), data_source));
  return PP_OK_COMPLETIONPENDING;
}

int32_t MediaPlayerResource::Play(scoped_refptr<TrackedCallback> callback) {
  auto error = CheckPreconditions(play_callback_);
  if (error != PP_OK)
    return error;

  play_callback_ = callback;
  Call<PpapiPluginMsg_MediaPlayer_PlayReply>(
      BROWSER, PpapiHostMsg_MediaPlayer_Play(),
      base::Bind(&OnReply, play_callback_));
  return PP_OK_COMPLETIONPENDING;
}

int32_t MediaPlayerResource::Pause(scoped_refptr<TrackedCallback> callback) {
  auto error = CheckPreconditions(pause_callback_);
  if (error != PP_OK)
    return error;

  pause_callback_ = callback;
  Call<PpapiPluginMsg_MediaPlayer_PauseReply>(
      BROWSER, PpapiHostMsg_MediaPlayer_Pause(),
      base::Bind(&OnReply, pause_callback_));
  return PP_OK_COMPLETIONPENDING;
}

int32_t MediaPlayerResource::Stop(scoped_refptr<TrackedCallback> callback) {
  auto error = CheckPreconditions(stop_callback_);
  if (error != PP_OK)
    return error;

  stop_callback_ = callback;
  Call<PpapiPluginMsg_MediaPlayer_StopReply>(
      BROWSER, PpapiHostMsg_MediaPlayer_Stop(),
      base::Bind(&OnReply, stop_callback_));
  return PP_OK_COMPLETIONPENDING;
}

int32_t MediaPlayerResource::Seek(PP_TimeTicks time,
                                  scoped_refptr<TrackedCallback> callback) {
  auto error = CheckPreconditions(seek_callback_);
  if (error != PP_OK)
    return error;

  seek_callback_ = callback;
  Call<PpapiPluginMsg_MediaPlayer_SeekReply>(
      BROWSER, PpapiHostMsg_MediaPlayer_Seek(time),
      base::Bind(&OnReply, seek_callback_));
  return PP_OK_COMPLETIONPENDING;
}

int32_t MediaPlayerResource::SetPlaybackRate(
    double rate,
    scoped_refptr<TrackedCallback> callback) {
  auto error = CheckPreconditions(set_playback_rate_callback_);
  if (error != PP_OK)
    return error;

  set_playback_rate_callback_ = callback;
  Call<PpapiPluginMsg_MediaPlayer_SetPlaybackRateReply>(
      BROWSER, PpapiHostMsg_MediaPlayer_SetPlaybackRate(rate),
      base::Bind(&OnReply, set_playback_rate_callback_));
  return PP_OK_COMPLETIONPENDING;
}

int32_t MediaPlayerResource::GetDuration(
    PP_TimeDelta* duration,
    scoped_refptr<TrackedCallback> callback) {
  if (!duration) {
    LOG(ERROR) << "Invalid PP_TimeDelta";
    return PP_ERROR_BADARGUMENT;
  }

  auto error = CheckPreconditions(get_duration_callback_);
  if (error != PP_OK)
    return error;

  get_duration_callback_ = callback;
  Call<PpapiPluginMsg_MediaPlayer_GetDurationReply>(
      BROWSER, PpapiHostMsg_MediaPlayer_GetDuration(),
      base::Bind(&OnReplyWithOutput<PP_TimeDelta>, get_duration_callback_,
                 duration));
  return PP_OK_COMPLETIONPENDING;
}

int32_t MediaPlayerResource::GetCurrentTime(
    PP_TimeTicks* currentTime,
    scoped_refptr<TrackedCallback> callback) {
  if (!currentTime) {
    LOG(ERROR) << "Invalid PP_TimeTicks";
    return PP_ERROR_BADARGUMENT;
  }

  auto error = CheckPreconditions(get_current_time_callback_);
  if (error != PP_OK)
    return error;

  get_current_time_callback_ = callback;
  Call<PpapiPluginMsg_MediaPlayer_GetCurrentTimeReply>(
      BROWSER, PpapiHostMsg_MediaPlayer_GetCurrentTime(),
      base::Bind(&OnReplyWithOutput<PP_TimeTicks>, get_current_time_callback_,
                 currentTime));
  return PP_OK_COMPLETIONPENDING;
}

int32_t MediaPlayerResource::GetPlayerState(
    PP_MediaPlayerState* state,
    scoped_refptr<TrackedCallback> callback) {
  if (!state) {
    LOG(ERROR) << "Invalid PP_MediaPlayerState";
    return PP_ERROR_BADARGUMENT;
  }

  if (TrackedCallback::IsPending(get_player_state_callback_)) {
    LOG(ERROR) << "GetPlayerState is already in progress";
    return PP_ERROR_INPROGRESS;
  }

  get_player_state_callback_ = callback;
  Call<PpapiPluginMsg_MediaPlayer_GetPlayerStateReply>(
      BROWSER, PpapiHostMsg_MediaPlayer_GetPlayerState(),
      base::Bind(&OnReplyWithOutput<PP_MediaPlayerState>,
                 get_player_state_callback_, state));
  return PP_OK_COMPLETIONPENDING;
}

int32_t MediaPlayerResource::GetCurrentVideoTrackInfo(
    PP_VideoTrackInfo* trackInfo,
    scoped_refptr<TrackedCallback> callback) {
  if (!trackInfo) {
    LOG(ERROR) << "Invalid PP_VideoTrackInfo";
    return PP_ERROR_BADARGUMENT;
  }

  auto error = CheckPreconditions(get_current_video_track_info_callback_);
  if (error != PP_OK)
    return error;

  get_current_video_track_info_callback_ = callback;
  Call<PpapiPluginMsg_MediaPlayer_GetCurrentVideoTrackInfoReply>(
      BROWSER, PpapiHostMsg_MediaPlayer_GetCurrentVideoTrackInfo(),
      base::Bind(&OnReplyWithOutput<PP_VideoTrackInfo>,
                 get_current_video_track_info_callback_, trackInfo));
  return PP_OK_COMPLETIONPENDING;
}

int32_t MediaPlayerResource::GetVideoTracksList(
    PP_ArrayOutput output,
    scoped_refptr<TrackedCallback> callback) {
  auto error = CheckPreconditions(get_video_tracks_list_callback_);
  if (error != PP_OK)
    return error;

  get_video_tracks_list_callback_ = callback;
  Call<PpapiPluginMsg_MediaPlayer_GetVideoTracksListReply>(
      BROWSER, PpapiHostMsg_MediaPlayer_GetVideoTracksList(),
      base::Bind(&OnReplyWithArrayOutput<PP_VideoTrackInfo>,
                 get_video_tracks_list_callback_, output));
  return PP_OK_COMPLETIONPENDING;
}

int32_t MediaPlayerResource::GetCurrentAudioTrackInfo(
    PP_AudioTrackInfo* trackInfo,
    scoped_refptr<TrackedCallback> callback) {
  if (!trackInfo) {
    LOG(ERROR) << "Invalid PP_AudioTrackInfo";
    return PP_ERROR_BADARGUMENT;
  }

  auto error = CheckPreconditions(get_current_audio_track_info_callback_);
  if (error != PP_OK)
    return error;

  get_current_audio_track_info_callback_ = callback;
  Call<PpapiPluginMsg_MediaPlayer_GetCurrentAudioTrackInfoReply>(
      BROWSER, PpapiHostMsg_MediaPlayer_GetCurrentAudioTrackInfo(),
      base::Bind(&OnReplyWithOutput<PP_AudioTrackInfo>,
                 get_current_audio_track_info_callback_, trackInfo));
  return PP_OK_COMPLETIONPENDING;
}

int32_t MediaPlayerResource::GetAudioTracksList(
    PP_ArrayOutput output,
    scoped_refptr<TrackedCallback> callback) {
  auto error = CheckPreconditions(get_audio_tracks_list_callback_);
  if (error != PP_OK)
    return error;

  get_audio_tracks_list_callback_ = callback;
  Call<PpapiPluginMsg_MediaPlayer_GetAudioTracksListReply>(
      BROWSER, PpapiHostMsg_MediaPlayer_GetAudioTracksList(),
      base::Bind(&OnReplyWithArrayOutput<PP_AudioTrackInfo>,
                 get_audio_tracks_list_callback_, output));
  return PP_OK_COMPLETIONPENDING;
}

int32_t MediaPlayerResource::GetCurrentTextTrackInfo(
    PP_TextTrackInfo* track_info,
    scoped_refptr<TrackedCallback> callback) {
  if (!track_info) {
    LOG(ERROR) << "Invalid PP_TextTrackInfo";
    return PP_ERROR_BADARGUMENT;
  }

  auto error = CheckPreconditions(get_current_text_track_info_callback_);
  if (error != PP_OK)
    return error;

  get_current_text_track_info_callback_ = callback;
  Call<PpapiPluginMsg_MediaPlayer_GetCurrentTextTrackInfoReply>(
      BROWSER, PpapiHostMsg_MediaPlayer_GetCurrentTextTrackInfo(),
      base::Bind(&OnReplyWithOutput<PP_TextTrackInfo>,
                 get_current_text_track_info_callback_, track_info));
  return PP_OK_COMPLETIONPENDING;
}

int32_t MediaPlayerResource::GetTextTracksList(
    PP_ArrayOutput output,
    scoped_refptr<TrackedCallback> callback) {
  auto error = CheckPreconditions(get_text_tracks_list_callback_);
  if (error != PP_OK)
    return error;

  get_text_tracks_list_callback_ = callback;
  Call<PpapiPluginMsg_MediaPlayer_GetTextTracksListReply>(
      BROWSER, PpapiHostMsg_MediaPlayer_GetTextTracksList(),
      base::Bind(&OnReplyWithArrayOutput<PP_TextTrackInfo>,
                 get_text_tracks_list_callback_, output));
  return PP_OK_COMPLETIONPENDING;
}

int32_t MediaPlayerResource::SelectTrack(
    PP_ElementaryStream_Type_Samsung stream_type,
    uint32_t track_index,
    scoped_refptr<TrackedCallback> callback) {
  auto error = CheckPreconditions(select_track_callback_);
  if (error != PP_OK)
    return error;

  select_track_callback_ = callback;
  Call<PpapiPluginMsg_MediaPlayer_SelectTrackReply>(
      BROWSER, PpapiHostMsg_MediaPlayer_SelectTrack(stream_type, track_index),
      base::Bind(&OnReply, select_track_callback_));
  return PP_OK_COMPLETIONPENDING;
}

int32_t MediaPlayerResource::SetSubtitlesVisible(
    PP_Bool visible,
    scoped_refptr<TrackedCallback> callback) {
  auto error = CheckPreconditions(set_subtitles_visible_callback_);
  if (error != PP_OK)
    return error;

  set_subtitles_visible_callback_ = callback;
  Call<PpapiPluginMsg_MediaPlayer_SetSubtitlesVisibleReply>(
      BROWSER, PpapiHostMsg_MediaPlayer_SetSubtitlesVisible(visible),
      base::Bind(&OnReply, set_subtitles_visible_callback_));
  return PP_OK_COMPLETIONPENDING;
}

int32_t MediaPlayerResource::AddExternalSubtitles(
    const char* file_path,
    const char* encoding,
    PP_TextTrackInfo* text_track_info,
    scoped_refptr<TrackedCallback> callback) {
  if (TrackedCallback::IsPending(add_external_subtitles_callback_)) {
    LOG(ERROR) << "AddExternalSubtitles is already in progress";
    return PP_ERROR_INPROGRESS;
  }

  add_external_subtitles_callback_ = callback;
  Call<PpapiPluginMsg_MediaPlayer_AddExternalSubtitlesReply>(
      BROWSER,
      PpapiHostMsg_MediaPlayer_AddExternalSubtitles(
          std::string(file_path), std::string(encoding ? encoding : "")),
      base::Bind(&OnReplyWithOutput<PP_TextTrackInfo>,
                 add_external_subtitles_callback_, text_track_info));
  return PP_OK_COMPLETIONPENDING;
}

int32_t MediaPlayerResource::SetSubtitlesDelay(
    PP_TimeDelta delay,
    scoped_refptr<TrackedCallback> callback) {
  auto error = CheckPreconditions(set_subtitles_delay_callback_);
  if (error != PP_OK)
    return error;

  set_subtitles_delay_callback_ = callback;
  Call<PpapiPluginMsg_MediaPlayer_SetSubtitlesDelayReply>(
      BROWSER, PpapiHostMsg_MediaPlayer_SetSubtitlesDelay(delay),
      base::Bind(&OnReply, set_subtitles_delay_callback_));
  return PP_OK_COMPLETIONPENDING;
}

int32_t MediaPlayerResource::SetDisplayRect(
    const PP_Rect* rect,
    scoped_refptr<TrackedCallback> callback) {
  if (TrackedCallback::IsPending(set_display_rect_callback_)) {
    LOG(ERROR) << "SetDisplayRect is already in progress";
    return PP_ERROR_INPROGRESS;
  }

  if (!IsValid(rect)) {
    LOG(ERROR) << "Invalid PP_Rect";
    return PP_ERROR_BADARGUMENT;
  }

  set_display_rect_callback_ = callback;
  Call<PpapiPluginMsg_MediaPlayer_SetDisplayRectReply>(
      BROWSER, PpapiHostMsg_MediaPlayer_SetDisplayRect(*rect),
      base::Bind(&OnReply, set_display_rect_callback_));
  return PP_OK_COMPLETIONPENDING;
}

int32_t MediaPlayerResource::SetDisplayMode(
    PP_MediaPlayerDisplayMode display_mode,
    scoped_refptr<TrackedCallback> callback) {
  if (TrackedCallback::IsPending(set_display_mode_callback_)) {
    LOG(ERROR) << "SetDisplayMode is already in progress";
    return PP_ERROR_INPROGRESS;
  }

  set_display_mode_callback_ = callback;
  Call<PpapiPluginMsg_MediaPlayer_SetDisplayModeReply>(
      BROWSER, PpapiHostMsg_MediaPlayer_SetDisplayMode(display_mode),
      base::Bind(&OnReply, set_display_mode_callback_));
  return PP_OK_COMPLETIONPENDING;
}

int32_t MediaPlayerResource::SetVr360Mode(
    PP_MediaPlayerVr360Mode vr360_mode,
    scoped_refptr<TrackedCallback> callback) {
  if (TrackedCallback::IsPending(set_vr360_mode_callback_)) {
    LOG(ERROR) << "SetVr360Mode is already in progress";
    return PP_ERROR_INPROGRESS;
  }

  set_vr360_mode_callback_ = callback;
  Call<PpapiPluginMsg_MediaPlayer_SetVr360ModeReply>(
      BROWSER, PpapiHostMsg_MediaPlayer_SetVr360Mode(vr360_mode),
      base::Bind(&OnReply, set_vr360_mode_callback_));
  return PP_OK_COMPLETIONPENDING;
}

int32_t MediaPlayerResource::SetVr360Rotation(
    float horizontal_angle,
    float vertical_angle,
    scoped_refptr<TrackedCallback> callback) {
  if (TrackedCallback::IsPending(set_vr360_rotation_callback_)) {
    LOG(ERROR) << "SetVr360Rotation is already in progress";
    return PP_ERROR_INPROGRESS;
  }

  set_vr360_rotation_callback_ = callback;
  Call<PpapiPluginMsg_MediaPlayer_SetVr360RotationReply>(
      BROWSER,
      PpapiHostMsg_MediaPlayer_SetVr360Rotation(horizontal_angle,
                                                vertical_angle),
      base::Bind(&OnReply, set_vr360_rotation_callback_));
  return PP_OK_COMPLETIONPENDING;
}

int32_t MediaPlayerResource::SetVr360ZoomLevel(
    uint32_t zoom_level,
    scoped_refptr<TrackedCallback> callback) {
  if (TrackedCallback::IsPending(set_vr360_zoom_level_callback_)) {
    LOG(ERROR) << "SetVr360ZoomLevel is already in progress";
    return PP_ERROR_INPROGRESS;
  }

  set_vr360_zoom_level_callback_ = callback;
  Call<PpapiPluginMsg_MediaPlayer_SetVr360ZoomLevelReply>(
      BROWSER, PpapiHostMsg_MediaPlayer_SetVr360ZoomLevel(zoom_level),
      base::Bind(&OnReply, set_vr360_zoom_level_callback_));
  return PP_OK_COMPLETIONPENDING;
}

int32_t MediaPlayerResource::SetDRMSpecificData(
    PP_MediaPlayerDRMType type,
    PP_MediaPlayerDRMOperation operation,
    uint32_t size,
    const void* data,
    scoped_refptr<TrackedCallback> callback) {
  auto error = CheckPreconditions(set_drm_specific_data_callback_);
  if (error != PP_OK)
    return error;

  set_drm_specific_data_callback_ = callback;
  Call<PpapiPluginMsg_MediaPlayer_SetDRMSpecificDataReply>(
      BROWSER,
      PpapiHostMsg_MediaPlayer_SetDRMSpecificData(
          type, operation, std::string(static_cast<const char*>(data), size)),
      base::Bind(&OnReply, set_drm_specific_data_callback_));
  return PP_OK_COMPLETIONPENDING;
}

int32_t MediaPlayerResource::CheckPreconditions(
    const scoped_refptr<TrackedCallback>& callback) {
  if (!data_source_) {
    LOG(ERROR) << "DataSource is not attached";
    return PP_ERROR_FAILED;
  }

  if (TrackedCallback::IsPending(callback)) {
    LOG(ERROR) << "A call [to checked method] is already in progress";
    return PP_ERROR_INPROGRESS;
  }

  return PP_OK;
}

void MediaPlayerResource::OnAttachDataSourceReply(
    PP_Resource data_source,
    const ResourceMessageReplyParams& params) {
  if (params.result() == PP_OK)
    data_source_ = data_source;

  Run(attach_data_source_callback_, params.result());
}

template <typename Handler, typename Listener>
bool MediaPlayerResource::SetListener(std::unique_ptr<Handler>* handler,
                                      const Listener* listener,
                                      void* user_data) {
  if (!(*handler) && listener) {
    (*handler).reset(new Handler(listener, user_data));
    Post(BROWSER, typename Handler::SetListenerMessage(true));
  } else if (*handler && !listener) {
    (*handler).reset(nullptr);
    Post(BROWSER, typename Handler::SetListenerMessage(false));
  } else if (*handler && listener) {
    (*handler)->SetListener(listener, user_data);
  }
  return true;
}

bool MediaPlayerResource::BindToInstance() {
  ProxyLock::AssertAcquired();
  thunk::PPB_Instance_API* instance =
      PluginGlobals::Get()->GetInstanceAPI(pp_instance());
  if (!instance)
    return false;
  return instance->BindGraphics(pp_instance(), pp_resource());
}

}  // namespace proxy
}  // namespace ppapi
