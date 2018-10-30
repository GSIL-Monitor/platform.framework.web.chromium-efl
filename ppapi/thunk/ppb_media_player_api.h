// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_THUNK_PPB_MEDIA_PLAYER_API_H_
#define PPAPI_THUNK_PPB_MEDIA_PLAYER_API_H_

#include "base/memory/ref_counted.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/samsung/ppb_media_player_samsung.h"
#include "ppapi/thunk/ppapi_thunk_export.h"

namespace ppapi {

class TrackedCallback;

namespace thunk {

// File defining "virtual" interfaces for Media Player PPAPI,
// see ppapi/api/samsung/ppb_media_player_samsung.idl
// for full description of class and its methods.

class PPAPI_THUNK_EXPORT PPB_MediaPlayer_API {
 public:
  virtual ~PPB_MediaPlayer_API() {}

  virtual bool SetMediaEventsListener(const PPP_MediaEventsListener_Samsung*,
                                      void* user_data) = 0;
  virtual bool SetSubtitleListener(const PPP_SubtitleListener_Samsung*,
                                   void* user_data) = 0;
  virtual bool SetBufferingListener(const PPP_BufferingListener_Samsung*,
                                    void* user_data) = 0;
  virtual bool SetDRMListener(const PPP_DRMListener_Samsung*,
                              void* user_data) = 0;

  virtual int32_t AttachDataSource(PP_Resource,
                                   scoped_refptr<TrackedCallback>) = 0;

  virtual int32_t Play(scoped_refptr<TrackedCallback>) = 0;
  virtual int32_t Pause(scoped_refptr<TrackedCallback>) = 0;
  virtual int32_t Stop(scoped_refptr<TrackedCallback>) = 0;
  virtual int32_t Seek(PP_TimeTicks, scoped_refptr<TrackedCallback>) = 0;

  virtual int32_t SetPlaybackRate(double rate,
                                  scoped_refptr<TrackedCallback>) = 0;
  virtual int32_t GetDuration(PP_TimeDelta*,
                              scoped_refptr<TrackedCallback>) = 0;
  virtual int32_t GetCurrentTime(PP_TimeTicks*,
                                 scoped_refptr<TrackedCallback>) = 0;

  virtual int32_t GetPlayerState(PP_MediaPlayerState*,
                                 scoped_refptr<TrackedCallback>) = 0;

  virtual int32_t GetCurrentVideoTrackInfo(PP_VideoTrackInfo*,
                                           scoped_refptr<TrackedCallback>) = 0;
  virtual int32_t GetVideoTracksList(PP_ArrayOutput,
                                     scoped_refptr<TrackedCallback>) = 0;
  virtual int32_t GetCurrentAudioTrackInfo(PP_AudioTrackInfo*,
                                           scoped_refptr<TrackedCallback>) = 0;
  virtual int32_t GetAudioTracksList(PP_ArrayOutput,
                                     scoped_refptr<TrackedCallback>) = 0;
  virtual int32_t GetCurrentTextTrackInfo(PP_TextTrackInfo*,
                                          scoped_refptr<TrackedCallback>) = 0;
  virtual int32_t GetTextTracksList(PP_ArrayOutput,
                                    scoped_refptr<TrackedCallback>) = 0;
  virtual int32_t SelectTrack(PP_ElementaryStream_Type_Samsung,
                              uint32_t track_index,
                              scoped_refptr<TrackedCallback>) = 0;

  virtual int32_t SetSubtitlesVisible(PP_Bool visible,
                                      scoped_refptr<TrackedCallback>) = 0;
  virtual int32_t AddExternalSubtitles(const char* file_path,
                                       const char* encoding,
                                       PP_TextTrackInfo*,
                                       scoped_refptr<TrackedCallback>) = 0;
  virtual int32_t SetSubtitlesDelay(PP_TimeDelta delay,
                                    scoped_refptr<TrackedCallback>) = 0;

  virtual int32_t SetDisplayRect(const PP_Rect*,
                                 scoped_refptr<TrackedCallback>) = 0;
  virtual int32_t SetDisplayMode(PP_MediaPlayerDisplayMode,
                                 scoped_refptr<TrackedCallback>) = 0;

  virtual int32_t SetVr360Mode(PP_MediaPlayerVr360Mode,
                               scoped_refptr<TrackedCallback>) = 0;

  virtual int32_t SetVr360Rotation(float,
                                   float,
                                   scoped_refptr<TrackedCallback>) = 0;

  virtual int32_t SetVr360ZoomLevel(uint32_t,
                                    scoped_refptr<TrackedCallback>) = 0;

  virtual int32_t SetDRMSpecificData(PP_MediaPlayerDRMType,
                                     PP_MediaPlayerDRMOperation,
                                     uint32_t size,
                                     const void*,
                                     scoped_refptr<TrackedCallback>) = 0;
};

}  // namespace thunk
}  // namespace ppapi

#endif  // PPAPI_THUNK_PPB_MEDIA_PLAYER_API_H_
