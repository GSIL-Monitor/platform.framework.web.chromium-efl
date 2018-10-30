// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_MEDIA_PLAYER_PRIVATE_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_MEDIA_PLAYER_PRIVATE_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_rect.h"
#include "ppapi/c/pp_size.h"
#include "ppapi/c/pp_time.h"
#include "ppapi/c/samsung/pp_media_player_samsung.h"

namespace content {

class BufferingListenerPrivate;
class DRMListenerPrivate;
class MediaEventsListenerPrivate;
class PepperMediaDataSourcePrivate;
class PepperMediaPlayerPrivateFactory;
class SubtitleListenerPrivate;

// Platform depended part of Media Player PPAPI implementation,
// see ppapi/api/samsung/ppb_media_player_samsung.idl.

class PepperMediaPlayerPrivate
    : public base::RefCountedThreadSafe<PepperMediaPlayerPrivate> {
 public:
  virtual void SetBufferingListener(
      std::unique_ptr<BufferingListenerPrivate>) = 0;
  virtual void SetDRMListener(std::unique_ptr<DRMListenerPrivate>) = 0;
  virtual void SetMediaEventsListener(
      std::unique_ptr<MediaEventsListenerPrivate>) = 0;
  virtual void SetSubtitleListener(
      std::unique_ptr<SubtitleListenerPrivate>) = 0;

  virtual void AttachDataSource(
      const scoped_refptr<PepperMediaDataSourcePrivate>&,
      const base::Callback<void(int32_t)>& callback) = 0;

  virtual void Play(const base::Callback<void(int32_t)>& callback) = 0;
  virtual void Pause(const base::Callback<void(int32_t)>& callback) = 0;
  virtual void Stop(const base::Callback<void(int32_t)>& callback) = 0;
  virtual void Seek(PP_TimeTicks,
                    const base::Callback<void(int32_t)>& callback) = 0;

  virtual void SetPlaybackRate(
      double rate,
      const base::Callback<void(int32_t)>& callback) = 0;
  virtual void GetDuration(
      const base::Callback<void(int32_t, PP_TimeDelta)>& callback) = 0;
  virtual void GetCurrentTime(
      const base::Callback<void(int32_t, PP_TimeTicks)>& callback) = 0;

  virtual void GetPlayerState(
      const base::Callback<void(int32_t, PP_MediaPlayerState)>& callback) = 0;

  virtual void GetCurrentVideoTrackInfo(
      const base::Callback<void(int32_t, const PP_VideoTrackInfo&)>&
          callback) = 0;
  virtual void GetVideoTracksList(
      const base::Callback<
          void(int32_t, const std::vector<PP_VideoTrackInfo>&)>& callback) = 0;
  virtual void GetCurrentAudioTrackInfo(
      const base::Callback<void(int32_t, const PP_AudioTrackInfo&)>&
          callback) = 0;
  virtual void GetAudioTracksList(
      const base::Callback<
          void(int32_t, const std::vector<PP_AudioTrackInfo>&)>& callback) = 0;
  virtual void GetCurrentTextTrackInfo(
      const base::Callback<void(int32_t, const PP_TextTrackInfo&)>&
          callback) = 0;
  virtual void GetTextTracksList(
      const base::Callback<void(int32_t, const std::vector<PP_TextTrackInfo>&)>&
          callback) = 0;
  virtual void SelectTrack(PP_ElementaryStream_Type_Samsung,
                           uint32_t track_index,
                           const base::Callback<void(int32_t)>& callback) = 0;

  virtual void AddExternalSubtitles(
      const std::string& file_path,
      const std::string& encoding,
      const base::Callback<void(int32_t, const PP_TextTrackInfo&)>&
          callback) = 0;
  virtual void SetSubtitlesDelay(
      PP_TimeDelta delay,
      const base::Callback<void(int32_t)>& callback) = 0;

  virtual void SetDisplayRect(
      const PP_Rect& display_rect,
      const PP_FloatRect& crop_ratio_rect,
      const base::Callback<void(int32_t)>& callback) = 0;
  virtual void SetDisplayMode(
      PP_MediaPlayerDisplayMode display_mode,
      const base::Callback<void(int32_t)>& callback) = 0;
  virtual void SetVr360Mode(PP_MediaPlayerVr360Mode vr360_mode,
                            const base::Callback<void(int32_t)>& callback) = 0;
  virtual void SetVr360Rotation(
      float,
      float,
      const base::Callback<void(int32_t)>& callback) = 0;
  virtual void SetVr360ZoomLevel(
      uint32_t,
      const base::Callback<void(int32_t)>& callback) = 0;
  virtual void SetDRMSpecificData(
      PP_MediaPlayerDRMType,
      PP_MediaPlayerDRMOperation,
      const void*,
      size_t,
      const base::Callback<void(int32_t)>& callback) = 0;

 protected:
  friend class base::RefCountedThreadSafe<PepperMediaPlayerPrivate>;
  virtual ~PepperMediaPlayerPrivate() {}
  PepperMediaPlayerPrivate() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(PepperMediaPlayerPrivate);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_MEDIA_PLAYER_PRIVATE_H_
