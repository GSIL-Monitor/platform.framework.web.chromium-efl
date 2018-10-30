// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_PLAYER_ADAPTER_INTERFACE_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_PLAYER_ADAPTER_INTERFACE_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"

#include "content/browser/renderer_host/pepper/media_player/buffering_listener_private.h"
#include "content/browser/renderer_host/pepper/media_player/media_events_listener_private.h"
#include "content/browser/renderer_host/pepper/media_player/subtitle_listener_private.h"
#include "content/browser/renderer_host/pepper/media_player/tizen/elementary_stream_listener_private_wrapper.h"
#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_tizen_drm_session.h"
#include "ppapi/c/pp_rect.h"
#include "ppapi/c/pp_time.h"
#include "ppapi/c/samsung/pp_media_common_samsung.h"
#include "ppapi/c/samsung/pp_media_player_samsung.h"
#include "ppapi/shared_impl/media_player/es_packet.h"
#include "ppapi/shared_impl/media_player/media_codecs_configs.h"

namespace content {

struct PepperAudioStreamInfo {
  ppapi::AudioCodecConfig config;
  PP_MediaPlayerDRMType drm_type;
  PP_StreamInitializationMode initialization_mode;
};

struct PepperVideoStreamInfo {
  ppapi::VideoCodecConfig config;
  PP_MediaPlayerDRMType drm_type;
  PP_StreamInitializationMode initialization_mode;
};

class PepperPlayerAdapterInterface {
 public:
  virtual ~PepperPlayerAdapterInterface() = default;

  // Player preparing
  virtual int32_t Initialize() = 0;
  virtual int32_t Reset() = 0;
  virtual int32_t Unprepare() = 0;

  // Prepare for ES playback.
  // Preconditions:
  // * streams are configured
  // Postconditions:
  // * After this call player is ready to accept packets
  // * BufferingListenerPrivate::OnBufferingStart() will be called when player
  //   is ready to accept packets.
  // * BufferingListenerPrivate::OnBufferingComplete() will be called when ES
  //   data source buffering is completed and playback may be started.
  //   Depending on the underlying implementation this may happen either
  //   asynchronously after call to PrepareES() completes or during call to
  //   PrepareES().
  virtual int32_t PrepareES() = 0;
  virtual int32_t PrepareURL(const std::string& url) = 0;

  // Player controllers
  virtual int32_t Play() = 0;
  virtual int32_t Pause() = 0;
  virtual int32_t Seek(PP_TimeTicks time,
                       const base::Closure& seek_completed_cb) = 0;
  virtual void SeekOnAppendThread(PP_TimeTicks time) = 0;
  virtual int32_t SelectTrack(PP_ElementaryStream_Type_Samsung,
                              uint32_t track_index) = 0;
  virtual int32_t SetPlaybackRate(float rate) = 0;
  virtual int32_t SubmitEOSPacket(
      PP_ElementaryStream_Type_Samsung track_type) = 0;
  virtual int32_t SubmitPacket(const ppapi::ESPacket* packet,
                               PP_ElementaryStream_Type_Samsung track_type) = 0;
  virtual int32_t SubmitEncryptedPacket(
      const ppapi::ESPacket* packet,
      ScopedHandleAndSize handle,
      PP_ElementaryStream_Type_Samsung track_type) = 0;
  virtual void Stop(const base::Callback<void(int32_t)>&) = 0;

  // Player setters
  virtual int32_t SetApplicationID(const std::string& application_id) = 0;
  virtual int32_t SetAudioStreamInfo(
      const PepperAudioStreamInfo& stream_info) = 0;
  virtual int32_t SetDisplay(void* display, bool is_windowless) = 0;
  virtual int32_t SetDisplayRect(const PP_Rect& display_rect,
                                 const PP_FloatRect& crop_ratio_rect) = 0;
  virtual int32_t SetDisplayMode(PP_MediaPlayerDisplayMode display_mode) = 0;
  virtual int32_t SetVr360Mode(PP_MediaPlayerVr360Mode vr360_mode) = 0;
  virtual int32_t SetVr360Rotation(float horizontal_angle,
                                   float vertical_angle) = 0;
  virtual int32_t SetVr360ZoomLevel(uint32_t zoom_level) = 0;
  // Set duration of a current ES media.
  //
  // Please note this might be called before PrepareES. If underlying player
  // needs to set duration after PrepareES, duration must be cached and applied
  // during PrepareES by the adapter.
  virtual int32_t SetDuration(PP_TimeDelta duration) = 0;
  virtual int32_t SetExternalSubtitlesPath(const std::string& file_path,
                                           const std::string& encoding) = 0;
  virtual int32_t SetStreamingProperty(PP_StreamingProperty property,
                                       const std::string& data) = 0;
  virtual int32_t SetSubtitlesDelay(PP_TimeDelta delay) = 0;
  virtual int32_t SetVideoStreamInfo(
      const PepperVideoStreamInfo& stream_info) = 0;

  // Player getters
  virtual int32_t GetAudioTracksList(
      std::vector<PP_AudioTrackInfo>* track_list) = 0;
  virtual int32_t GetAvailableBitrates(std::string* bitrates) = 0;
  virtual int32_t GetCurrentTime(PP_TimeTicks* time) = 0;
  // Emit OnTimeUpdate event with current time to a registred
  // MediaEventsListener.
  virtual int32_t NotifyCurrentTimeListener() = 0;
  virtual int32_t GetCurrentTrack(PP_ElementaryStream_Type_Samsung stream_type,
                                  int32_t* track_index) = 0;
  virtual int32_t GetDuration(PP_TimeDelta* duration) = 0;
  virtual int32_t GetPlayerState(PP_MediaPlayerState* state) = 0;
  virtual int32_t GetStreamingProperty(PP_StreamingProperty property,
                                       std::string* property_value) = 0;
  virtual int32_t GetTextTracksList(
      std::vector<PP_TextTrackInfo>* track_list) = 0;
  virtual int32_t GetVideoTracksList(
      std::vector<PP_VideoTrackInfo>* track_list) = 0;

  virtual bool IsDisplayModeSupported(
      PP_MediaPlayerDisplayMode display_mode) = 0;

  // Callback registering
  virtual int32_t SetErrorCallback(
      const base::Callback<void(PP_MediaPlayerError)>& callback) = 0;
  virtual int32_t SetDisplayRectUpdateCallback(
      const base::Callback<void()>& callback) = 0;

  // Listeners registering
  virtual void SetListener(
      PP_ElementaryStream_Type_Samsung type,
      base::WeakPtr<ElementaryStreamListenerPrivate> listener) = 0;
  virtual void RemoveListener(PP_ElementaryStream_Type_Samsung type,
                              ElementaryStreamListenerPrivate* listener) = 0;
  virtual void SetMediaEventsListener(MediaEventsListenerPrivate* listener) = 0;
  virtual void SetSubtitleListener(SubtitleListenerPrivate* listener) = 0;
  virtual void SetBufferingListener(BufferingListenerPrivate* listener) = 0;

 protected:
  PepperPlayerAdapterInterface() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(PepperPlayerAdapterInterface);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_PLAYER_ADAPTER_INTERFACE_H_
