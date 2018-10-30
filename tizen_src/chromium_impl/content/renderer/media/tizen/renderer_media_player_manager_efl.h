// Copyright 2014 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_TIZEN_RENDERER_MEDIA_PLAYER_MANAGER_EFL_H_
#define CONTENT_RENDERER_MEDIA_TIZEN_RENDERER_MEDIA_PLAYER_MANAGER_EFL_H_

#include <map>

#include "base/memory/shared_memory.h"
#include "content/common/media/media_player_messages_enums_efl.h"
#include "content/public/renderer/render_frame_observer.h"
#include "media/base/tizen/media_player_efl.h"
#include "third_party/WebKit/public/platform/WebMediaPlayer.h"
#include "url/gurl.h"
#if defined(TIZEN_TBM_SUPPORT)
#include "ui/gfx/tbm_buffer_handle.h"
#endif

#if defined(TIZEN_VIDEO_HOLE)
#include "ui/gfx/geometry/rect_f.h"
#endif

namespace content {
class WebMediaPlayerEfl;

class RendererMediaPlayerManager : public RenderFrameObserver {
 public:
  // Constructs a RendererMediaPlayerManager object for the |render_frame|.
  explicit RendererMediaPlayerManager(RenderFrame* render_frame);
  ~RendererMediaPlayerManager() override;

  // Initializes a MediaPlayerEfl object in browser process.
  void Initialize(int player_id,
                  MediaPlayerHostMsg_Initialize_Type type,
                  const GURL& url,
                  const std::string& mime_type,
                  int demuxer_client_id);

  // Initializes a MediaPlayerEfl object in browser process.
  bool InitializeSync(int player_id,
                      MediaPlayerHostMsg_Initialize_Type type,
                      const GURL& url,
                      const std::string& mime_type,
                      int demuxer_client_id);

  // Starts the player.
  void Play(int player_id);

  // Pauses the player.
  // is_media_related_action should be true if this pause is coming from an
  // an action that explicitly pauses the video (user pressing pause, JS, etc.)
  // Otherwise it should be false if Pause is being called due to other reasons
  // (cleanup, freeing resources, etc.)
  void Pause(int player_id, bool is_media_related_action);

  // Performs seek on the player.
  void Seek(int player_id, base::TimeDelta time);

  // Sets the player volume.
  void SetVolume(int player_id, double volume);

  // Sets the playback rate.
  void SetRate(int player_id, double rate);

  // Destroys the player in the browser process
  void DestroyPlayer(int player_id);

  // Registers and unregisters a WebMediaPlayerEfl object.
  int RegisterMediaPlayer(blink::WebMediaPlayer* player);
  void UnregisterMediaPlayer(int player_id);

  // RenderFrameObserver overrides.
  void OnDestruct() override;
  bool OnMessageReceived(const IPC::Message& message) override;
  void WasHidden() override;
  void WasShown() override;
  void OnStop() override;

  // Pause the playing media players when tab/webpage goes to background
  void PausePlayingPlayers();
  void Suspend(int player_id);
  void Resume(int player_id);
  void Activate(int player_id);
  void Deactivate(int player_id);

  // Get start date of current media
  double GetStartDate(int player_id);

#if defined(OS_TIZEN_TV_PRODUCT)
  void NotifySubtitlePlay(int player_id,
                          int id,
                          const std::string& url,
                          const std::string& lang);
  void OnGetPlayTrackInfo(int player_id, int id);
  bool RegisterTimeline(int player_id, const std::string& timeline_selector);
  bool UnRegisterTimeline(int player_id, const std::string& timeline_selector);
  void GetTimelinePositions(int player_id,
                            const std::string& timeline_selector,
                            uint32_t* units_per_tick,
                            uint32_t* units_per_second,
                            int64_t* content_time,
                            bool* paused);
  double GetSpeed(int player_id_);
  std::string GetMrsUrl(int player_id);
  std::string GetContentId(int player_id);
  bool SyncTimeline(int player_id,
                    const std::string& timeline_selector,
                    int64_t timeline_pos,
                    int64_t wallclock_pos,
                    int tolerance);
  bool SetWallClock(int player_id, const std::string& wallclock_url);
  unsigned GetDecodedFrameCount(int player_id);
  unsigned GetDroppedFrameCount(int player_id);
  void NotifyElementRemove(int player_id);
  void SetParentalRatingResult(int player_id, bool is_pass);
  void SetDecryptorHandle(int player_id, eme::eme_decryptor_t decryptor);
  void OnInitData(int player_id, const std::vector<unsigned char>& init_data);
  void OnWaitingForKey(int player_id);
  // Synchronously destroys the player in the browser process
  void DestroyPlayerSync(int player_id);
#endif

  // Requests the player to enter/exit fullscreen.
  void EnteredFullscreen(int player_id);
  void ExitedFullscreen(int player_id);

#if defined(TIZEN_VIDEO_HOLE)
  void SetMediaGeometry(int player_id, const gfx::RectF& rect);
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
  void SetActiveTextTrack(int player_id, int id, bool is_in_band);
  void SetActiveAudioTrack(int player_id, int index);
  void SetActiveVideoTrack(int player_id, int index);
  void CheckHLSSupport(const blink::WebString& url, bool& support);
#endif

#if defined(TIZEN_TBM_SUPPORT) && defined(OS_TIZEN_TV_PRODUCT)
  void ControlMediaPacket(int player_id, int mode);
#endif

 private:
  void OnNewFrameAvailable(int player_id,
                           base::SharedMemoryHandle foreign_memory_handle,
                           uint32_t length,
                           base::TimeDelta timestamp);
  void OnPlayerDestroyed(int player_id);

#if defined(TIZEN_TBM_SUPPORT)
  void OnNewTbmBufferAvailable(int player_id,
                               gfx::TbmBufferHandle tbm_handle,
#if defined(OS_TIZEN_TV_PRODUCT)
                               int scaler_physical_id,
#endif
                               base::TimeDelta timestamp);
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
  void OnDrmError(int player_id);
  void OnSeekableTimeChange(int player_id,
                            base::TimeDelta min_time,
                            base::TimeDelta max_time);
  void OnSyncTimeline(int player_id,
                      const std::string& timeline_selector,
                      int sync);
  void OnRegisterTimeline(
      int player_id,
      const blink::WebMediaPlayer::register_timeline_cb_info_s& info);
  void OnMrsUrlChange(int player_id, const std::string& url);
  void OnContentIdChange(int player_id, const std::string& id);
  void OnAddAudioTrack(
      int player_id,
      const blink::WebMediaPlayer::audio_video_track_info_s& info);
  void OnAddVideoTrack(
      int player_id,
      const blink::WebMediaPlayer::audio_video_track_info_s& info);
  void OnAddTextTrack(int player_id,
                      const std::string& label,
                      const std::string& language,
                      const std::string& id);
  void OnInbandEventTextTrack(int player_id,
                              const std::string& info,
                              int action);
  void OnInbandEventTextCue(int player_id,
                            const std::string& info,
                            unsigned int id,
                            long long int start_time,
                            long long int end_time);
  void OnPlayerStarted(int player_id, bool player_started);
  void OnVideoSlotsAvailableChange(unsigned slots_available);
#endif

  blink::WebMediaPlayer* GetMediaPlayer(int player_id);
  void OnMediaDataChange(int player_id, int width, int height, int media);
  void OnDurationChange(int player_id, base::TimeDelta duration);
  void OnTimeUpdate(int player_id, base::TimeDelta current_time);
  void OnBufferUpdate(int player_id, int percentage);
  void OnTimeChanged(int player_id);
  void OnPauseStateChange(int player_id, bool state);
  void OnSeekComplete(int player_id);
  void OnRequestSeek(int player_id, base::TimeDelta seek_time);
  void OnPlayerSuspend(int player_id, bool is_preempted);
  void OnPlayerResumed(int player_id, bool is_preempted);
  void OnReadyStateChange(int player_id,
                          blink::WebMediaPlayer::ReadyState state);
  void OnNetworkStateChange(int player_id,
                            blink::WebMediaPlayer::NetworkState state);

 private:
  std::map<int, blink::WebMediaPlayer*> media_players_;

  DISALLOW_COPY_AND_ASSIGN(RendererMediaPlayerManager);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_TIZEN_RENDERER_MEDIA_PLAYER_MANAGER_EFL_H_
