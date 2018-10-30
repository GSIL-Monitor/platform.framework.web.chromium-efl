// Copyright (c) 2016 Samsung Electronics. All rights reserved.

// From samsung/ppb_media_player_samsung.idl modified Mon Aug  1 15:04:43 2016.

#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/samsung/ppb_media_player_samsung.h"
#include "ppapi/shared_impl/resource_tracker.h"
#include "ppapi/shared_impl/tracked_callback.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppapi_thunk_export.h"
#include "ppapi/thunk/ppb_media_player_api.h"

namespace ppapi {
namespace thunk {

namespace {

PP_Resource CreateWithOptions(PP_Instance instance,
                              PP_MediaPlayerMode player_mode,
                              PP_BindToInstanceMode bind_mode) {
  VLOG(4) << "PPB_MediaPlayer_Samsung::CreateWithOptions()";
  EnterResourceCreation enter(instance);
  if (enter.failed())
    return 0;
  return enter.functions()->CreateMediaPlayer(instance, player_mode, bind_mode);
}

PP_Resource Create(PP_Instance instance) {
  VLOG(4) << "PPB_MediaPlayer_Samsung::Create()";
  return CreateWithOptions(instance, PP_MEDIAPLAYERMODE_DEFAULT,
                           PP_BINDTOINSTANCEMODE_BIND);
}

PP_Resource CreateWithoutBindingToInstance(PP_Instance instance) {
  VLOG(4) << "PPB_MediaPlayer_Samsung::CreateWithoutBindingToInstance()";
  return CreateWithOptions(instance, PP_MEDIAPLAYERMODE_DEFAULT,
                           PP_BINDTOINSTANCEMODE_DONT_BIND);
}

PP_Bool IsMediaPlayer(PP_Resource controller) {
  VLOG(4) << "PPB_MediaPlayer_Samsung::IsMediaPlayer()";
  EnterResource<PPB_MediaPlayer_API> enter(controller, true);
  return PP_FromBool(enter.succeeded());
}

PP_Bool SetMediaEventsListener(
    PP_Resource player,
    const struct PPP_MediaEventsListener_Samsung_1_0* listener,
    void* user_data) {
  VLOG(4) << "PPB_MediaPlayer_Samsung::SetMediaEventsListener()";
  EnterResource<PPB_MediaPlayer_API> enter(player, true);
  if (enter.failed())
    return PP_FALSE;
  return PP_FromBool(
      enter.object()->SetMediaEventsListener(listener, user_data));
}

PP_Bool SetSubtitleListener(
    PP_Resource player,
    const struct PPP_SubtitleListener_Samsung_1_0* listener,
    void* user_data) {
  VLOG(4) << "PPB_MediaPlayer_Samsung::SetSubtitleListener()";
  EnterResource<PPB_MediaPlayer_API> enter(player, true);
  if (enter.failed())
    return PP_FALSE;
  return PP_FromBool(enter.object()->SetSubtitleListener(listener, user_data));
}

PP_Bool SetBufferingListener(
    PP_Resource data_source,
    const struct PPP_BufferingListener_Samsung_1_0* listener,
    void* user_data) {
  VLOG(4) << "PPB_MediaPlayer_Samsung::SetBufferingListener()";
  EnterResource<PPB_MediaPlayer_API> enter(data_source, true);
  if (enter.failed())
    return PP_FALSE;
  return PP_FromBool(enter.object()->SetBufferingListener(listener, user_data));
}

PP_Bool SetDRMListener(PP_Resource data_source,
                       const struct PPP_DRMListener_Samsung_1_0* listener,
                       void* user_data) {
  VLOG(4) << "PPB_MediaPlayer_Samsung::SetDRMListener()";
  EnterResource<PPB_MediaPlayer_API> enter(data_source, true);
  if (enter.failed())
    return PP_FALSE;
  return PP_FromBool(enter.object()->SetDRMListener(listener, user_data));
}

int32_t AttachDataSource(PP_Resource player,
                         PP_Resource data_source,
                         struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_MediaPlayer_Samsung::AttachDataSource()";
  EnterResource<PPB_MediaPlayer_API> enter(player, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(
      enter.object()->AttachDataSource(data_source, enter.callback()));
}

int32_t Play(PP_Resource player, struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_MediaPlayer_Samsung::Play()";
  EnterResource<PPB_MediaPlayer_API> enter(player, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(enter.object()->Play(enter.callback()));
}

int32_t Pause(PP_Resource player, struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_MediaPlayer_Samsung::Pause()";
  EnterResource<PPB_MediaPlayer_API> enter(player, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(enter.object()->Pause(enter.callback()));
}

int32_t Stop(PP_Resource player, struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_MediaPlayer_Samsung::Stop()";
  EnterResource<PPB_MediaPlayer_API> enter(player, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(enter.object()->Stop(enter.callback()));
}

int32_t Seek(PP_Resource player,
             PP_TimeTicks time,
             struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_MediaPlayer_Samsung::Seek()";
  EnterResource<PPB_MediaPlayer_API> enter(player, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(enter.object()->Seek(time, enter.callback()));
}

int32_t SetPlaybackRate(PP_Resource player,
                        double rate,
                        struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_MediaPlayer_Samsung::SetPlaybackRate()";
  EnterResource<PPB_MediaPlayer_API> enter(player, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(
      enter.object()->SetPlaybackRate(rate, enter.callback()));
}

int32_t GetDuration(PP_Resource player,
                    PP_TimeDelta* duration,
                    struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_MediaPlayer_Samsung::GetDuration()";
  EnterResource<PPB_MediaPlayer_API> enter(player, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(
      enter.object()->GetDuration(duration, enter.callback()));
}

int32_t GetCurrentTime(PP_Resource player,
                       PP_TimeTicks* time,
                       struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_MediaPlayer_Samsung::GetCurrentTime()";
  EnterResource<PPB_MediaPlayer_API> enter(player, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(
      enter.object()->GetCurrentTime(time, enter.callback()));
}

int32_t GetPlayerState(PP_Resource player,
                       PP_MediaPlayerState* state,
                       struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_MediaPlayer_Samsung::GetPlayerState()";
  EnterResource<PPB_MediaPlayer_API> enter(player, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(
      enter.object()->GetPlayerState(state, enter.callback()));
}

int32_t GetCurrentVideoTrackInfo(PP_Resource player,
                                 struct PP_VideoTrackInfo* track_info,
                                 struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_MediaPlayer_Samsung::GetCurrentVideoTrackInfo()";
  EnterResource<PPB_MediaPlayer_API> enter(player, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(
      enter.object()->GetCurrentVideoTrackInfo(track_info, enter.callback()));
}

int32_t GetVideoTracksList(PP_Resource player,
                           struct PP_ArrayOutput output,
                           struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_MediaPlayer_Samsung::GetVideoTracksList()";
  EnterResource<PPB_MediaPlayer_API> enter(player, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(
      enter.object()->GetVideoTracksList(output, enter.callback()));
}

int32_t GetCurrentAudioTrackInfo(PP_Resource player,
                                 struct PP_AudioTrackInfo* track_info,
                                 struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_MediaPlayer_Samsung::GetCurrentAudioTrackInfo()";
  EnterResource<PPB_MediaPlayer_API> enter(player, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(
      enter.object()->GetCurrentAudioTrackInfo(track_info, enter.callback()));
}

int32_t GetAudioTracksList(PP_Resource player,
                           struct PP_ArrayOutput output,
                           struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_MediaPlayer_Samsung::GetAudioTracksList()";
  EnterResource<PPB_MediaPlayer_API> enter(player, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(
      enter.object()->GetAudioTracksList(output, enter.callback()));
}

int32_t GetCurrentTextTrackInfo(PP_Resource player,
                                struct PP_TextTrackInfo* track_info,
                                struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_MediaPlayer_Samsung::GetCurrentTextTrackInfo()";
  EnterResource<PPB_MediaPlayer_API> enter(player, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(
      enter.object()->GetCurrentTextTrackInfo(track_info, enter.callback()));
}

int32_t GetTextTracksList(PP_Resource player,
                          struct PP_ArrayOutput output,
                          struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_MediaPlayer_Samsung::GetTextTracksList()";
  EnterResource<PPB_MediaPlayer_API> enter(player, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(
      enter.object()->GetTextTracksList(output, enter.callback()));
}

int32_t SelectTrack(PP_Resource player,
                    PP_ElementaryStream_Type_Samsung track_type,
                    uint32_t track_index,
                    struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_MediaPlayer_Samsung::SelectTrack()";
  EnterResource<PPB_MediaPlayer_API> enter(player, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(
      enter.object()->SelectTrack(track_type, track_index, enter.callback()));
}

int32_t AddExternalSubtitles(PP_Resource player,
                             const char* file_path,
                             const char* encoding,
                             struct PP_TextTrackInfo* subtitles,
                             struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_MediaPlayer_Samsung::AddExternalSubtitles()";
  EnterResource<PPB_MediaPlayer_API> enter(player, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(enter.object()->AddExternalSubtitles(
      file_path, encoding, subtitles, enter.callback()));
}

int32_t SetSubtitlesDelay(PP_Resource player,
                          PP_TimeDelta delay,
                          struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_MediaPlayer_Samsung::SetSubtitlesDelay()";
  EnterResource<PPB_MediaPlayer_API> enter(player, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(
      enter.object()->SetSubtitlesDelay(delay, enter.callback()));
}

int32_t SetDisplayRect(PP_Resource player,
                       const struct PP_Rect* rect,
                       struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_MediaPlayer_Samsung::SetDisplayRect()";
  EnterResource<PPB_MediaPlayer_API> enter(player, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(
      enter.object()->SetDisplayRect(rect, enter.callback()));
}

int32_t SetDisplayMode(PP_Resource player,
                       PP_MediaPlayerDisplayMode display_mode,
                       struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_MediaPlayer_Samsung::SetDisplayMode()";
  EnterResource<PPB_MediaPlayer_API> enter(player, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(
      enter.object()->SetDisplayMode(display_mode, enter.callback()));
}

int32_t SetDRMSpecificData(PP_Resource player,
                           PP_MediaPlayerDRMType drm_type,
                           PP_MediaPlayerDRMOperation drm_operation,
                           uint32_t drm_data_size,
                           const void* drm_data,
                           struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_MediaPlayer_Samsung::SetDRMSpecificData()";
  EnterResource<PPB_MediaPlayer_API> enter(player, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(enter.object()->SetDRMSpecificData(
      drm_type, drm_operation, drm_data_size, drm_data, enter.callback()));
}

int32_t SetVr360Mode(PP_Resource player,
                     PP_MediaPlayerVr360Mode vr360_mode,
                     struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_MediaPlayer_Samsung::SetVr360Mode()";
  EnterResource<PPB_MediaPlayer_API> enter(player, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(
      enter.object()->SetVr360Mode(vr360_mode, enter.callback()));
}

int32_t SetVr360Rotation(PP_Resource player,
                         float horizontal_angle,
                         float vertical_angle,
                         struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_MediaPlayer_Samsung::SetVr360Rotation()";
  EnterResource<PPB_MediaPlayer_API> enter(player, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(enter.object()->SetVr360Rotation(
      horizontal_angle, vertical_angle, enter.callback()));
}

int32_t SetVr360ZoomLevel(PP_Resource player,
                          uint32_t zoom_level,
                          struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_MediaPlayer_Samsung::SetVr360ZoomLevel()";
  EnterResource<PPB_MediaPlayer_API> enter(player, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(
      enter.object()->SetVr360ZoomLevel(zoom_level, enter.callback()));
}

const PPB_MediaPlayer_Samsung_1_0 g_ppb_mediaplayer_samsung_thunk_1_0 = {
    &Create,
    &IsMediaPlayer,
    &SetMediaEventsListener,
    &SetSubtitleListener,
    &SetBufferingListener,
    &SetDRMListener,
    &AttachDataSource,
    &Play,
    &Pause,
    &Stop,
    &Seek,
    &SetPlaybackRate,
    &GetDuration,
    &GetCurrentTime,
    &GetPlayerState,
    &GetCurrentVideoTrackInfo,
    &GetVideoTracksList,
    &GetCurrentAudioTrackInfo,
    &GetAudioTracksList,
    &GetCurrentTextTrackInfo,
    &GetTextTracksList,
    &SelectTrack,
    &AddExternalSubtitles,
    &SetSubtitlesDelay,
    &SetDisplayRect,
    &SetDRMSpecificData};

const PPB_MediaPlayer_Samsung_1_1 g_ppb_mediaplayer_samsung_thunk_1_1 = {
    &Create,
    &CreateWithoutBindingToInstance,
    &IsMediaPlayer,
    &SetMediaEventsListener,
    &SetSubtitleListener,
    &SetBufferingListener,
    &SetDRMListener,
    &AttachDataSource,
    &Play,
    &Pause,
    &Stop,
    &Seek,
    &SetPlaybackRate,
    &GetDuration,
    &GetCurrentTime,
    &GetPlayerState,
    &GetCurrentVideoTrackInfo,
    &GetVideoTracksList,
    &GetCurrentAudioTrackInfo,
    &GetAudioTracksList,
    &GetCurrentTextTrackInfo,
    &GetTextTracksList,
    &SelectTrack,
    &AddExternalSubtitles,
    &SetSubtitlesDelay,
    &SetDisplayRect,
    &SetDRMSpecificData};

const PPB_MediaPlayer_Samsung_1_2 g_ppb_mediaplayer_samsung_thunk_1_2 = {
    &Create,
    &CreateWithOptions,
    &IsMediaPlayer,
    &SetMediaEventsListener,
    &SetSubtitleListener,
    &SetBufferingListener,
    &SetDRMListener,
    &AttachDataSource,
    &Play,
    &Pause,
    &Stop,
    &Seek,
    &SetPlaybackRate,
    &GetDuration,
    &GetCurrentTime,
    &GetPlayerState,
    &GetCurrentVideoTrackInfo,
    &GetVideoTracksList,
    &GetCurrentAudioTrackInfo,
    &GetAudioTracksList,
    &GetCurrentTextTrackInfo,
    &GetTextTracksList,
    &SelectTrack,
    &AddExternalSubtitles,
    &SetSubtitlesDelay,
    &SetDisplayRect,
    &SetDRMSpecificData};

const PPB_MediaPlayer_Samsung_1_3 g_ppb_mediaplayer_samsung_thunk_1_3 = {
    &Create,
    &CreateWithOptions,
    &IsMediaPlayer,
    &SetMediaEventsListener,
    &SetSubtitleListener,
    &SetBufferingListener,
    &SetDRMListener,
    &AttachDataSource,
    &Play,
    &Pause,
    &Stop,
    &Seek,
    &SetPlaybackRate,
    &GetDuration,
    &GetCurrentTime,
    &GetPlayerState,
    &GetCurrentVideoTrackInfo,
    &GetVideoTracksList,
    &GetCurrentAudioTrackInfo,
    &GetAudioTracksList,
    &GetCurrentTextTrackInfo,
    &GetTextTracksList,
    &SelectTrack,
    &AddExternalSubtitles,
    &SetSubtitlesDelay,
    &SetDisplayRect,
    &SetDisplayMode,
    &SetDRMSpecificData};

const PPB_MediaPlayer_Samsung_1_4 g_ppb_mediaplayer_samsung_thunk_1_4 = {
    &Create,
    &CreateWithOptions,
    &IsMediaPlayer,
    &SetMediaEventsListener,
    &SetSubtitleListener,
    &SetBufferingListener,
    &SetDRMListener,
    &AttachDataSource,
    &Play,
    &Pause,
    &Stop,
    &Seek,
    &SetPlaybackRate,
    &GetDuration,
    &GetCurrentTime,
    &GetPlayerState,
    &GetCurrentVideoTrackInfo,
    &GetVideoTracksList,
    &GetCurrentAudioTrackInfo,
    &GetAudioTracksList,
    &GetCurrentTextTrackInfo,
    &GetTextTracksList,
    &SelectTrack,
    &AddExternalSubtitles,
    &SetSubtitlesDelay,
    &SetDisplayRect,
    &SetDisplayMode,
    &SetDRMSpecificData,
    &SetVr360Mode,
    &SetVr360Rotation,
    &SetVr360ZoomLevel};

}  // namespace

PPAPI_THUNK_EXPORT const PPB_MediaPlayer_Samsung_1_0*
GetPPB_MediaPlayer_Samsung_1_0_Thunk() {
  return &g_ppb_mediaplayer_samsung_thunk_1_0;
}

PPAPI_THUNK_EXPORT const PPB_MediaPlayer_Samsung_1_1*
GetPPB_MediaPlayer_Samsung_1_1_Thunk() {
  return &g_ppb_mediaplayer_samsung_thunk_1_1;
}

PPAPI_THUNK_EXPORT const PPB_MediaPlayer_Samsung_1_2*
GetPPB_MediaPlayer_Samsung_1_2_Thunk() {
  return &g_ppb_mediaplayer_samsung_thunk_1_2;
}

PPAPI_THUNK_EXPORT const PPB_MediaPlayer_Samsung_1_3*
GetPPB_MediaPlayer_Samsung_1_3_Thunk() {
  return &g_ppb_mediaplayer_samsung_thunk_1_3;
}

PPAPI_THUNK_EXPORT const PPB_MediaPlayer_Samsung_1_4*
GetPPB_MediaPlayer_Samsung_1_4_Thunk() {
  return &g_ppb_mediaplayer_samsung_thunk_1_4;
}

}  // namespace thunk
}  // namespace ppapi
