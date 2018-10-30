// Copyright 2014 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_TIZEN_MEDIA_PLAYER_MANAGER_EFL_H_
#define MEDIA_BASE_TIZEN_MEDIA_PLAYER_MANAGER_EFL_H_

#include <vector>

#include "base/memory/shared_memory.h"
#include "media/base/media_export.h"
#include "third_party/WebKit/public/platform/WebMediaPlayer.h"

#if defined(TIZEN_TBM_SUPPORT)
#include "ui/gfx/tbm_buffer_handle.h"
#endif

namespace content {
class WebContents;
}
#if defined(TIZEN_VIDEO_HOLE)
namespace gfx {
class Rect;
}
#endif

namespace media {

class MediaPlayerInterfaceEfl;

class MEDIA_EXPORT MediaPlayerManager {
 public:
  virtual ~MediaPlayerManager() = default;
  virtual MediaPlayerInterfaceEfl* GetPlayer(int player_id) = 0;

  virtual void OnTimeChanged(int player_id) = 0;
  virtual void OnTimeUpdate(int player_id, base::TimeDelta current_time) = 0;
  virtual void OnRequestSeek(int player_id, base::TimeDelta seek_time) = 0;
  virtual void OnPauseStateChange(int player_id, bool state) = 0;
  virtual void OnSeekComplete(int player_id) = 0;
  virtual void OnBufferUpdate(int player_id, int buffering_percentage) = 0;
  virtual void OnDurationChange(int player_id, base::TimeDelta duration) = 0;
  virtual void OnReadyStateChange(int player_id,
                                  blink::WebMediaPlayer::ReadyState state) = 0;
  virtual void OnNetworkStateChange(
      int player_id,
      blink::WebMediaPlayer::NetworkState state) = 0;
  virtual void OnMediaDataChange(int player_id,
                                 int width,
                                 int height,
                                 int media) = 0;
  virtual void OnNewFrameAvailable(int player_id,
                                   base::SharedMemoryHandle handle,
                                   uint32_t length,
                                   base::TimeDelta timestamp) = 0;
  virtual void OnPlayerDestroyed(int) = 0;
#if defined(TIZEN_TBM_SUPPORT)
  virtual void OnNewTbmBufferAvailable(int player_id,
                                       gfx::TbmBufferHandle tbm_handle,
                                       base::TimeDelta timestamp) = 0;
#endif
  virtual void OnInitComplete(int player_id, bool success) = 0;
  virtual void OnResumeComplete(int player_id) = 0;
  virtual void OnSuspendComplete(int player_id) = 0;
#if defined(TIZEN_VIDEO_HOLE)
  virtual gfx::Rect GetViewportRect(int player_id) const = 0;
#endif
#if defined(OS_TIZEN_TV_PRODUCT)
  virtual void GetPlayTrackInfo(int player_id, int id) = 0;
  virtual void OnSeekableTimeChange(int player_id,
                                    base::TimeDelta min_time,
                                    base::TimeDelta max_time) = 0;
  virtual void OnPlayerResourceConflict(int player_id) = 0;
  virtual void OnDrmError(int player_id) = 0;
  virtual void OnRegisterTimelineCbInfo(
      int player_id,
      const blink::WebMediaPlayer::register_timeline_cb_info_s& info) = 0;
  virtual void OnSyncTimelineCbInfo(int player_id,
                                    const std::string& timeline_selector,
                                    int sync) = 0;
  virtual void OnMrsUrlChange(int player_id, const std::string& url) = 0;
  virtual void OnContentIdChange(int player_id, const std::string& id) = 0;
  virtual void OnAddAudioTrack(
      int player_id,
      const blink::WebMediaPlayer::audio_video_track_info_s& info) = 0;
  virtual void OnAddVideoTrack(
      int player_id,
      const blink::WebMediaPlayer::audio_video_track_info_s& info) = 0;
  virtual void OnAddTextTrack(int player_id,
                              const std::string& label,
                              const std::string& language,
                              const std::string& id) = 0;
  virtual void OnInbandEventTextTrack(int player_id,
                                      const std::string& info,
                                      int action) = 0;
  virtual void OnInbandEventTextCue(int player_id,
                                    const std::string& info,
                                    unsigned int id,
                                    long long int start_time,
                                    long long int end_time) = 0;
  virtual void OnPlayerStarted(int player_id, bool play_started) = 0;
  virtual void OnInitData(int, const std::vector<unsigned char>&) = 0;
  virtual void OnWaitingForKey(int) = 0;
#endif
  virtual content::WebContents* GetWebContents(int player_id) = 0;
  virtual void OnPlayerSuspendRequest(int player_id) = 0;
};

}  // namespace media

#endif  // MEDIA_BASE_TIZEN_MEDIA_PLAYER_MANAGER_EFL_H_
