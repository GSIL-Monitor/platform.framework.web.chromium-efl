// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_MEDIA_PLAYER_MEDIA_PLAYER_RESOURCE_H_
#define PPAPI_PROXY_MEDIA_PLAYER_MEDIA_PLAYER_RESOURCE_H_

#include "ppapi/proxy/plugin_resource.h"
#include "ppapi/proxy/ppapi_proxy_export.h"
#include "ppapi/shared_impl/proxy_lock.h"
#include "ppapi/thunk/ppb_media_player_api.h"

namespace ppapi {
namespace proxy {

class ResourceMessageReplyParams;

/*
 * Implementation of plugin side Resource proxy for Media Player PPAPI,
 * see ppapi/api/samsung/ppb_media_player_samsung.idl
 * for full description of class and its methods.
 */
class MediaPlayerResource : public PluginResource,
                            public thunk::PPB_MediaPlayer_API {
 public:
  static PP_Resource Create(Connection,
                            PP_Instance,
                            PP_MediaPlayerMode,
                            PP_BindToInstanceMode bind_mode);
  virtual ~MediaPlayerResource();

  // PluginResource overrides.
  thunk::PPB_MediaPlayer_API* AsPPB_MediaPlayer_API() override;

  void OnReplyReceived(const proxy::ResourceMessageReplyParams& params,
                       const IPC::Message& msg) override;

  // PPB_MediaPlayer_API implementation
  bool SetMediaEventsListener(const PPP_MediaEventsListener_Samsung*,
                              void* user_data) override;
  bool SetSubtitleListener(const PPP_SubtitleListener_Samsung*,
                           void* user_data) override;
  bool SetBufferingListener(const PPP_BufferingListener_Samsung*,
                            void* user_data) override;
  bool SetDRMListener(const PPP_DRMListener_Samsung*, void* user_data) override;

  int32_t AttachDataSource(PP_Resource,
                           scoped_refptr<TrackedCallback>) override;

  int32_t Play(scoped_refptr<TrackedCallback>) override;
  int32_t Pause(scoped_refptr<TrackedCallback>) override;
  int32_t Stop(scoped_refptr<TrackedCallback>) override;
  int32_t Seek(PP_TimeTicks, scoped_refptr<TrackedCallback>) override;

  int32_t SetPlaybackRate(double rate, scoped_refptr<TrackedCallback>) override;
  int32_t GetDuration(PP_TimeDelta*, scoped_refptr<TrackedCallback>) override;
  int32_t GetCurrentTime(PP_TimeTicks*,
                         scoped_refptr<TrackedCallback>) override;

  int32_t GetPlayerState(PP_MediaPlayerState*,
                         scoped_refptr<TrackedCallback>) override;

  int32_t GetCurrentVideoTrackInfo(PP_VideoTrackInfo*,
                                   scoped_refptr<TrackedCallback>) override;
  int32_t GetVideoTracksList(PP_ArrayOutput,
                             scoped_refptr<TrackedCallback>) override;
  int32_t GetCurrentAudioTrackInfo(PP_AudioTrackInfo*,
                                   scoped_refptr<TrackedCallback>) override;
  int32_t GetAudioTracksList(PP_ArrayOutput,
                             scoped_refptr<TrackedCallback>) override;
  int32_t GetCurrentTextTrackInfo(PP_TextTrackInfo*,
                                  scoped_refptr<TrackedCallback>) override;
  int32_t GetTextTracksList(PP_ArrayOutput,
                            scoped_refptr<TrackedCallback>) override;
  int32_t SelectTrack(PP_ElementaryStream_Type_Samsung,
                      uint32_t track_index,
                      scoped_refptr<TrackedCallback>) override;

  int32_t SetSubtitlesVisible(PP_Bool visible,
                              scoped_refptr<TrackedCallback>) override;
  int32_t AddExternalSubtitles(const char* file_path,
                               const char* encoding,
                               PP_TextTrackInfo*,
                               scoped_refptr<TrackedCallback>) override;
  int32_t SetSubtitlesDelay(PP_TimeDelta delay,
                            scoped_refptr<TrackedCallback>) override;

  int32_t SetDisplayRect(const PP_Rect*,
                         scoped_refptr<TrackedCallback>) override;
  int32_t SetDisplayMode(PP_MediaPlayerDisplayMode,
                         scoped_refptr<TrackedCallback>) override;

  int32_t SetVr360Mode(PP_MediaPlayerVr360Mode,
                       scoped_refptr<TrackedCallback>) override;

  int32_t SetVr360Rotation(float,
                           float,
                           scoped_refptr<TrackedCallback>) override;

  int32_t SetVr360ZoomLevel(uint32_t, scoped_refptr<TrackedCallback>) override;

  int32_t SetDRMSpecificData(PP_MediaPlayerDRMType,
                             PP_MediaPlayerDRMOperation,
                             uint32_t size,
                             const void*,
                             scoped_refptr<TrackedCallback>) override;

 private:
  MediaPlayerResource(Connection, PP_Instance, PP_MediaPlayerMode);
  // need const & due to TrackedCallback::isPending API
  int32_t CheckPreconditions(const scoped_refptr<TrackedCallback>&);

  void OnAttachDataSourceReply(PP_Resource data_source,
                               const ResourceMessageReplyParams&);

  template <typename Handler, typename Listener>
  bool SetListener(std::unique_ptr<Handler>*, const Listener*, void* user_data);

  bool BindToInstance();

  scoped_refptr<TrackedCallback> attach_data_source_callback_;
  scoped_refptr<TrackedCallback> play_callback_;
  scoped_refptr<TrackedCallback> pause_callback_;
  scoped_refptr<TrackedCallback> stop_callback_;
  scoped_refptr<TrackedCallback> seek_callback_;
  scoped_refptr<TrackedCallback> set_playback_rate_callback_;
  scoped_refptr<TrackedCallback> get_duration_callback_;
  scoped_refptr<TrackedCallback> get_current_time_callback_;
  scoped_refptr<TrackedCallback> get_player_state_callback_;
  scoped_refptr<TrackedCallback> get_current_video_track_info_callback_;
  scoped_refptr<TrackedCallback> get_video_tracks_list_callback_;
  scoped_refptr<TrackedCallback> get_current_audio_track_info_callback_;
  scoped_refptr<TrackedCallback> get_audio_tracks_list_callback_;
  scoped_refptr<TrackedCallback> get_current_text_track_info_callback_;
  scoped_refptr<TrackedCallback> get_text_tracks_list_callback_;
  scoped_refptr<TrackedCallback> select_track_callback_;
  scoped_refptr<TrackedCallback> set_subtitles_visible_callback_;
  scoped_refptr<TrackedCallback> add_external_subtitles_callback_;
  scoped_refptr<TrackedCallback> set_subtitles_delay_callback_;
  scoped_refptr<TrackedCallback> set_display_rect_callback_;
  scoped_refptr<TrackedCallback> set_display_mode_callback_;
  scoped_refptr<TrackedCallback> set_vr360_mode_callback_;
  scoped_refptr<TrackedCallback> set_vr360_rotation_callback_;
  scoped_refptr<TrackedCallback> set_vr360_zoom_level_callback_;
  scoped_refptr<TrackedCallback> set_drm_specific_data_callback_;

  PP_Resource data_source_;

  class MediaEventsListener;
  class SubtitleListener;
  class BufferingListener;
  class DRMListener;

  std::unique_ptr<MediaEventsListener> events_listener_;
  std::unique_ptr<SubtitleListener> subtitle_listener_;
  std::unique_ptr<BufferingListener> buffering_listener_;
  std::unique_ptr<DRMListener> drm_listener_;

  DISALLOW_COPY_AND_ASSIGN(MediaPlayerResource);
};

}  // namespace proxy
}  // namespace ppapi

#endif  //  PPAPI_PROXY_MEDIA_PLAYER_MEDIA_PLAYER_RESOURCE_H_
