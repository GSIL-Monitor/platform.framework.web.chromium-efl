// Copyright (c) 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/cpp/samsung/media_player_samsung.h"

#include "ppapi/c/pp_errors.h"
#include "ppapi/c/ppb_instance.h"
#include "ppapi/cpp/instance_handle.h"
#include "ppapi/cpp/module_impl.h"
#include "ppapi/cpp/samsung/media_data_source_samsung.h"

#define CALL_MEDIA_PLAYER(Fn, cb, ...)                                      \
  do {                                                                      \
    if (has_interface<PPB_MediaPlayer_Samsung_1_3>()) {                     \
      return get_interface<PPB_MediaPlayer_Samsung_1_3>()->Fn(__VA_ARGS__); \
    } else if (has_interface<PPB_MediaPlayer_Samsung_1_2>()) {              \
      return get_interface<PPB_MediaPlayer_Samsung_1_2>()->Fn(__VA_ARGS__); \
    } else if (has_interface<PPB_MediaPlayer_Samsung_1_1>()) {              \
      return get_interface<PPB_MediaPlayer_Samsung_1_1>()->Fn(__VA_ARGS__); \
    } else if (has_interface<PPB_MediaPlayer_Samsung_1_0>()) {              \
      return get_interface<PPB_MediaPlayer_Samsung_1_0>()->Fn(__VA_ARGS__); \
    }                                                                       \
    return cb.MayForce(PP_ERROR_NOINTERFACE);                               \
  } while (0)

namespace pp {

namespace {

template <> const char* interface_name<PPB_MediaPlayer_Samsung_1_0>() {
  return PPB_MEDIAPLAYER_SAMSUNG_INTERFACE_1_0;
}

template <>
const char* interface_name<PPB_MediaPlayer_Samsung_1_1>() {
  return PPB_MEDIAPLAYER_SAMSUNG_INTERFACE_1_1;
}

template <>
const char* interface_name<PPB_MediaPlayer_Samsung_1_2>() {
  return PPB_MEDIAPLAYER_SAMSUNG_INTERFACE_1_2;
}

template <>
const char* interface_name<PPB_MediaPlayer_Samsung_1_3>() {
  return PPB_MEDIAPLAYER_SAMSUNG_INTERFACE_1_3;
}

template <>
const char* interface_name<PPB_MediaPlayer_Samsung_1_4>() {
  return PPB_MEDIAPLAYER_SAMSUNG_INTERFACE_1_4;
}

template <>
const char* interface_name<PPB_Instance_1_0>() {
  return PPB_INSTANCE_INTERFACE_1_0;
}

}  // namespace

MediaPlayer_Samsung::MediaPlayer_Samsung() {
}

MediaPlayer_Samsung::MediaPlayer_Samsung(const InstanceHandle& instance) {
  if (has_interface<PPB_MediaPlayer_Samsung_1_4>()) {
    PassRefFromConstructor(get_interface<PPB_MediaPlayer_Samsung_1_4>()->Create(
        instance.pp_instance()));
  } else if (has_interface<PPB_MediaPlayer_Samsung_1_3>()) {
    PassRefFromConstructor(get_interface<PPB_MediaPlayer_Samsung_1_3>()->Create(
        instance.pp_instance()));
  } else if (has_interface<PPB_MediaPlayer_Samsung_1_2>()) {
    PassRefFromConstructor(get_interface<PPB_MediaPlayer_Samsung_1_2>()->Create(
        instance.pp_instance()));
  } else if (has_interface<PPB_MediaPlayer_Samsung_1_1>()) {
    PassRefFromConstructor(get_interface<PPB_MediaPlayer_Samsung_1_1>()->Create(
        instance.pp_instance()));
  } else if (has_interface<PPB_MediaPlayer_Samsung_1_0>()) {
    PassRefFromConstructor(
        get_interface<PPB_MediaPlayer_Samsung_1_0>()->Create(
            instance.pp_instance()));
  }
}

MediaPlayer_Samsung::MediaPlayer_Samsung(
    const InstanceHandle& instance,
    MediaPlayer_Samsung::DoNotBindToInstance) {
  if (has_interface<PPB_MediaPlayer_Samsung_1_4>()) {
    PassRefFromConstructor(
        get_interface<PPB_MediaPlayer_Samsung_1_3>()->CreateWithOptions(
            instance.pp_instance(), PP_MEDIAPLAYERMODE_DEFAULT,
            PP_BINDTOINSTANCEMODE_DONT_BIND));
  } else if (has_interface<PPB_MediaPlayer_Samsung_1_3>()) {
    PassRefFromConstructor(
        get_interface<PPB_MediaPlayer_Samsung_1_3>()->CreateWithOptions(
            instance.pp_instance(), PP_MEDIAPLAYERMODE_DEFAULT,
            PP_BINDTOINSTANCEMODE_DONT_BIND));
  } else if (has_interface<PPB_MediaPlayer_Samsung_1_2>()) {
    PassRefFromConstructor(
        get_interface<PPB_MediaPlayer_Samsung_1_2>()->CreateWithOptions(
            instance.pp_instance(), PP_MEDIAPLAYERMODE_DEFAULT,
            PP_BINDTOINSTANCEMODE_DONT_BIND));
  } else if (has_interface<PPB_MediaPlayer_Samsung_1_1>()) {
    PassRefFromConstructor(
        get_interface<PPB_MediaPlayer_Samsung_1_1>()
            ->CreateWithoutBindingToInstance(instance.pp_instance()));
  } else if (has_interface<PPB_MediaPlayer_Samsung_1_0>()) {
    // If only version 1.0 is supported, then DoNotBind option is irrelevant.
    PassRefFromConstructor(
        get_interface<PPB_MediaPlayer_Samsung_1_0>()->Create(
            instance.pp_instance()));
  }
}

MediaPlayer_Samsung::MediaPlayer_Samsung(const InstanceHandle& instance,
                                         PP_MediaPlayerMode player_mode,
                                         PP_BindToInstanceMode bind_mode) {
  if (has_interface<PPB_MediaPlayer_Samsung_1_4>()) {
    PassRefFromConstructor(
        get_interface<PPB_MediaPlayer_Samsung_1_4>()->CreateWithOptions(
            instance.pp_instance(), player_mode, bind_mode));
  } else if (has_interface<PPB_MediaPlayer_Samsung_1_3>()) {
    PassRefFromConstructor(
        get_interface<PPB_MediaPlayer_Samsung_1_3>()->CreateWithOptions(
            instance.pp_instance(), player_mode, bind_mode));
  } else if (has_interface<PPB_MediaPlayer_Samsung_1_2>()) {
    PassRefFromConstructor(
        get_interface<PPB_MediaPlayer_Samsung_1_2>()->CreateWithOptions(
            instance.pp_instance(), player_mode, bind_mode));
  } else if (player_mode == PP_MEDIAPLAYERMODE_DEFAULT) {
    if (has_interface<PPB_MediaPlayer_Samsung_1_1>()) {
      if (bind_mode == PP_BINDTOINSTANCEMODE_BIND) {
        PassRefFromConstructor(
            get_interface<PPB_MediaPlayer_Samsung_1_1>()->Create(
                instance.pp_instance()));
      } else {
        PassRefFromConstructor(
            get_interface<PPB_MediaPlayer_Samsung_1_1>()
                ->CreateWithoutBindingToInstance(instance.pp_instance()));
      }
    } else if (has_interface<PPB_MediaPlayer_Samsung_1_0>()) {
      PassRefFromConstructor(
          get_interface<PPB_MediaPlayer_Samsung_1_0>()->Create(
              instance.pp_instance()));
    }
  }
}

MediaPlayer_Samsung::MediaPlayer_Samsung(const MediaPlayer_Samsung& other)
    : Resource(other) {
}

MediaPlayer_Samsung& MediaPlayer_Samsung::operator=(
    const MediaPlayer_Samsung& other) {
  Resource::operator=(other);
  return *this;
}

MediaPlayer_Samsung::~MediaPlayer_Samsung() {
}

bool MediaPlayer_Samsung::BindToInstance(const InstanceHandle& instance) {
  if (!has_interface<PPB_Instance_1_0>())
    return false;
  return PP_ToBool(get_interface<PPB_Instance_1_0>()->BindGraphics(
      instance.pp_instance(), pp_resource()));
}

int32_t MediaPlayer_Samsung::AttachDataSource(
    const MediaDataSource_Samsung& data_source,
    const CompletionCallback& callback) {
  CALL_MEDIA_PLAYER(AttachDataSource, callback, pp_resource(),
                    data_source.pp_resource(),
                    callback.pp_completion_callback());
}

/* Playback control */

int32_t MediaPlayer_Samsung::Play(const CompletionCallback& callback) {
  CALL_MEDIA_PLAYER(Play, callback, pp_resource(),
                    callback.pp_completion_callback());
}

int32_t MediaPlayer_Samsung::Pause(const CompletionCallback& callback) {
  CALL_MEDIA_PLAYER(Pause, callback, pp_resource(),
                    callback.pp_completion_callback());
}

int32_t MediaPlayer_Samsung::Stop(const CompletionCallback& callback) {
  CALL_MEDIA_PLAYER(Stop, callback, pp_resource(),
                    callback.pp_completion_callback());
}

int32_t MediaPlayer_Samsung::Seek(PP_TimeTicks time,
                          const CompletionCallback& callback) {
  CALL_MEDIA_PLAYER(Seek, callback, pp_resource(), time,
                    callback.pp_completion_callback());
}

int32_t MediaPlayer_Samsung::SetPlaybackRate(double rate,
                                     const CompletionCallback& callback) {
  CALL_MEDIA_PLAYER(SetPlaybackRate, callback, pp_resource(), rate,
                    callback.pp_completion_callback());
}

/* Playback time info */

int32_t MediaPlayer_Samsung::GetDuration(
    const CompletionCallbackWithOutput<PP_TimeDelta>& callback) {
  CALL_MEDIA_PLAYER(GetDuration, callback, pp_resource(), callback.output(),
                    callback.pp_completion_callback());
}

int32_t MediaPlayer_Samsung::GetCurrentTime(
    const CompletionCallbackWithOutput<PP_TimeTicks>& callback) {
  CALL_MEDIA_PLAYER(GetCurrentTime, callback, pp_resource(), callback.output(),
                    callback.pp_completion_callback());
}

/* Playback state info */

int32_t MediaPlayer_Samsung::GetPlayerState(
    const CompletionCallbackWithOutput<PP_MediaPlayerState>& callback) {
  CALL_MEDIA_PLAYER(GetPlayerState, callback, pp_resource(), callback.output(),
                    callback.pp_completion_callback());
}

/* Tracks info */

int32_t MediaPlayer_Samsung::GetCurrentVideoTrackInfo(
    const CompletionCallbackWithOutput<PP_VideoTrackInfo>& callback) {
  CALL_MEDIA_PLAYER(GetCurrentVideoTrackInfo, callback, pp_resource(),
                    callback.output(), callback.pp_completion_callback());
}

int32_t MediaPlayer_Samsung::GetVideoTracksList(
    const CompletionCallbackWithOutput<MediaPlayer_Samsung::VideoTracksList>&
        callback) {
  CALL_MEDIA_PLAYER(GetVideoTracksList, callback, pp_resource(),
                    callback.output(), callback.pp_completion_callback());
}

int32_t MediaPlayer_Samsung::GetCurrentAudioTrackInfo(
    const CompletionCallbackWithOutput<PP_AudioTrackInfo>& callback) {
  CALL_MEDIA_PLAYER(GetCurrentAudioTrackInfo, callback, pp_resource(),
                    callback.output(), callback.pp_completion_callback());
}

int32_t MediaPlayer_Samsung::GetAudioTracksList(
    const CompletionCallbackWithOutput<MediaPlayer_Samsung::AudioTracksList>&
        callback) {
  CALL_MEDIA_PLAYER(GetAudioTracksList, callback, pp_resource(),
                    callback.output(), callback.pp_completion_callback());
}

int32_t MediaPlayer_Samsung::GetCurrentTextTrackInfo(
    const CompletionCallbackWithOutput<PP_TextTrackInfo>& callback) {
  CALL_MEDIA_PLAYER(GetCurrentTextTrackInfo, callback, pp_resource(),
                    callback.output(), callback.pp_completion_callback());
}

int32_t MediaPlayer_Samsung::GetTextTracksList(
    const CompletionCallbackWithOutput<MediaPlayer_Samsung::TextTracksList>&
        callback) {
  CALL_MEDIA_PLAYER(GetTextTracksList, callback, pp_resource(),
                    callback.output(), callback.pp_completion_callback());
}

int32_t MediaPlayer_Samsung::SelectTrack(
    PP_ElementaryStream_Type_Samsung track_type,
    uint32_t track_index,
    const CompletionCallback& callback) {
  CALL_MEDIA_PLAYER(SelectTrack, callback, pp_resource(), track_type,
                    track_index, callback.pp_completion_callback());
}

/* Subtitles control */

int32_t MediaPlayer_Samsung::AddExternalSubtitles(
    const std::string& file_path,
    const std::string& encoding,
    const CompletionCallbackWithOutput<PP_TextTrackInfo>& callback) {
  CALL_MEDIA_PLAYER(AddExternalSubtitles, callback, pp_resource(),
                    file_path.c_str(), encoding.c_str(), callback.output(),
                    callback.pp_completion_callback());
}

int32_t MediaPlayer_Samsung::SetSubtitlesDelay(
    PP_TimeDelta delay,
    const CompletionCallback& callback) {
  CALL_MEDIA_PLAYER(SetSubtitlesDelay, callback, pp_resource(), delay,
                    callback.pp_completion_callback());
}

/* Other - player control */

int32_t MediaPlayer_Samsung::SetDisplayRect(
    const PP_Rect& rect,
    const CompletionCallback& callback) {
  CALL_MEDIA_PLAYER(SetDisplayRect, callback, pp_resource(), &rect,
                    callback.pp_completion_callback());
}

int32_t MediaPlayer_Samsung::SetDisplayMode(
    PP_MediaPlayerDisplayMode display_mode,
    const CompletionCallback& callback) {
  if (has_interface<PPB_MediaPlayer_Samsung_1_3>()) {
    return get_interface<PPB_MediaPlayer_Samsung_1_3>()->SetDisplayMode(
        pp_resource(), display_mode, callback.pp_completion_callback());
  }
  return PP_ERROR_NOTSUPPORTED;
}

/* DRM Related */

int32_t MediaPlayer_Samsung::SetDRMSpecificData(
    PP_MediaPlayerDRMType drm_type,
    PP_MediaPlayerDRMOperation drm_operation,
    uint32_t drm_data_size,
    const void* drm_data,
    const CompletionCallback& callback) {
  CALL_MEDIA_PLAYER(SetDRMSpecificData, callback, pp_resource(), drm_type,
                    drm_operation, drm_data_size, drm_data,
                    callback.pp_completion_callback());
}

int32_t MediaPlayer_Samsung::SetVr360Mode(PP_MediaPlayerVr360Mode display_mode,
                                          const CompletionCallback& callback) {
  if (has_interface<PPB_MediaPlayer_Samsung_1_4>()) {
    return get_interface<PPB_MediaPlayer_Samsung_1_4>()->SetVr360Mode(
        pp_resource(), display_mode, callback.pp_completion_callback());
  }
  return PP_ERROR_NOTSUPPORTED;
}

int32_t MediaPlayer_Samsung::SetVr360Rotation(
    float horizontal_angle,
    float vertical_angle,
    const CompletionCallback& callback) {
  if (has_interface<PPB_MediaPlayer_Samsung_1_4>()) {
    return get_interface<PPB_MediaPlayer_Samsung_1_4>()->SetVr360Rotation(
        pp_resource(), horizontal_angle, vertical_angle,
        callback.pp_completion_callback());
  }
  return PP_ERROR_NOTSUPPORTED;
}

int32_t MediaPlayer_Samsung::SetVr360ZoomLevel(
    uint32_t zoom_level,
    const CompletionCallback& callback) {
  if (has_interface<PPB_MediaPlayer_Samsung_1_4>()) {
    return get_interface<PPB_MediaPlayer_Samsung_1_4>()->SetVr360ZoomLevel(
        pp_resource(), zoom_level, callback.pp_completion_callback());
  }
  return PP_ERROR_NOTSUPPORTED;
}

}  // namespace pp
