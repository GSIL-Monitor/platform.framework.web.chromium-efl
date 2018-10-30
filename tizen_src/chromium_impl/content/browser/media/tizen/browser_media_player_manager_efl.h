// Copyright 2014 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_TIZEN_BROWSER_MEDIA_PLAYER_MANAGER_EFL_H_
#define CONTENT_BROWSER_MEDIA_TIZEN_BROWSER_MEDIA_PLAYER_MANAGER_EFL_H_

#include <map>
#include <memory>
#include <vector>

#include "content/common/media/media_player_init_config.h"
#include "content/common/media/media_player_messages_enums_efl.h"
#include "content/public/browser/browser_message_filter.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents_observer.h"
#include "media/base/tizen/longest_conflict_strategy.h"
#include "media/base/tizen/media_capability_manager.h"
#include "media/base/tizen/media_player_interface_efl.h"
#include "media/base/tizen/media_player_manager_efl.h"
#include "third_party/WebKit/public/platform/WebMediaPlayer.h"

#if defined(OS_TIZEN_TV_PRODUCT)
#include <emeCDM/IEME.h>
#endif

#if defined(TIZEN_VIDEO_HOLE)
namespace gfx {
class Rect;
class RectF;
}  // namespace gfx
#endif

namespace content {

// This class manages all the MediaPlayerEfl objects. It receives
// control operations from the the render process, and forwards
// them to corresponding MediaPlayerEfl object. Callbacks from
// MediaPlayerEfl objects are converted to IPCs and then sent to the
// render process.
class CONTENT_EXPORT BrowserMediaPlayerManagerEfl
    : public media::MediaPlayerManager {
 public:
  // Returns a new instance using the registered factory if available.
  static BrowserMediaPlayerManagerEfl* GetInstance();
  ~BrowserMediaPlayerManagerEfl() override;

  // media::MediaPlayerManager implementation.
  media::MediaPlayerInterfaceEfl* GetPlayer(int player_id) override;
  void OnTimeChanged(int player_id) override;
  void OnTimeUpdate(int player_id, base::TimeDelta current_time) override;
  void OnPauseStateChange(int player_id, bool state) override;
  void OnSeekComplete(int player_id) override;
  void OnRequestSeek(int player_id, base::TimeDelta seek_time) override;
  void OnBufferUpdate(int player_id, int percentage) override;
  void OnDurationChange(int player_id, base::TimeDelta duration) override;
  void OnReadyStateChange(int player_id,
                          blink::WebMediaPlayer::ReadyState state) override;
  void OnNetworkStateChange(int player_id,
                            blink::WebMediaPlayer::NetworkState state) override;
  void OnMediaDataChange(int player_id,
                         int width,
                         int height,
                         int media) override;
  void OnNewFrameAvailable(int player_id,
                           base::SharedMemoryHandle foreign_memory_handle,
                           uint32_t length,
                           base::TimeDelta timestamp) override;
  void OnPlayerDestroyed(int player_id) override;

#if defined(TIZEN_TBM_SUPPORT)
#if defined(OS_TIZEN_TV_PRODUCT)
  void OnControlMediaPacket(int player_id, int mode);
#endif
  void OnNewTbmBufferAvailable(int player_id,
                               gfx::TbmBufferHandle tbm_handle,
                               base::TimeDelta timestamp) override;
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
  void OnNotifySubtitlePlay(int player_id,
                            int id,
                            const std::string& url,
                            const std::string& lang);
  void GetPlayTrackInfo(int player_id, int id);
  void OnDrmError(int player_id) override;
  void OnPlayerResourceConflict(int player_id) override;
  void OnSeekableTimeChange(int player_id,
                            base::TimeDelta min_time,
                            base::TimeDelta max_time) override;
  void OnRegisterTimeline(int player_id,
                          const std::string& timeline_selector,
                          bool* ret);
  void OnUnRegisterTimeline(int player_id,
                            const std::string& timeline_selector,
                            bool* ret);
  void OnGetTimelinePositions(int player_id,
                              const std::string& timeline_selector,
                              uint32_t* units_per_tick,
                              uint32_t* units_per_second,
                              int64_t* content_time,
                              bool* paused);
  void OnGetSpeed(int player_id, double* speed);
  void OnGetMrsUrl(int player_id, std::string* url);
  void OnGetContentId(int player_id, std::string* id);
  void OnSyncTimeline(int player_id,
                      const std::string& timeline_selector,
                      int64_t timeline_pos,
                      int64_t wallclock_pos,
                      int tolerance,
                      bool* ret);
  void OnSetWallClock(int player_id,
                      const std::string& wallclock_url,
                      bool* ret);
  void OnRegisterTimelineCbInfo(
      int player_id,
      const blink::WebMediaPlayer::register_timeline_cb_info_s& info) override;
  void OnSyncTimelineCbInfo(int player_id,
                            const std::string& timeline_selector,
                            int sync) override;
  void OnMrsUrlChange(int player_id, const std::string& url) override;
  void OnContentIdChange(int player_id, const std::string& id) override;
  void OnAddAudioTrack(
      int player_id,
      const blink::WebMediaPlayer::audio_video_track_info_s& info) override;
  void OnAddVideoTrack(
      int player_id,
      const blink::WebMediaPlayer::audio_video_track_info_s& info) override;
  void OnAddTextTrack(int player_id,
                      const std::string& label,
                      const std::string& language,
                      const std::string& id) override;
  void OnSetActiveTextTrack(int player_id, int id, bool is_in_band);
  void OnSetActiveAudioTrack(int player_id, int index);
  void OnSetActiveVideoTrack(int player_id, int index);
  void OnInbandEventTextTrack(int player_id,
                              const std::string& info,
                              int action) override;
  void OnInbandEventTextCue(int player_id,
                            const std::string& info,
                            unsigned int id,
                            long long int start_time,
                            long long int end_time) override;
  void NotifyPlayingVideoRect(int player_id, const gfx::RectF& rect);
  void OnGetDroppedFrameCount(int player_id, unsigned* dropped_frame_count);
  void OnGetDecodedFrameCount(int player_id, unsigned* decoded_frame_count);
  void OnElementRemove(int player_id);
  void OnSetParentalRatingResult(int player_id, bool is_pass);
  void OnPlayerStarted(int player_id, bool play_started) override;
  void OnCheckHLS(const std::string& url, bool* support);
  void OnSetDecryptorHandle(int player_id, eme::eme_decryptor_t decryptor);
  void OnInitData(int player_id, const std::vector<unsigned char>& init_data);
  void OnWaitingForKey(int player_id) override;
  void OnDestroySync(int player_id);
#endif

  // Helper function to handle IPC from RenderMediaPlayerMangaerEfl.
  void OnPlay(RenderFrameHost* render_frame_host, int player_id);
  void OnPause(RenderFrameHost* render_frame_host,
               int player_id,
               bool is_media_related_action);
  void OnSetVolume(RenderFrameHost* render_frame_host,
                   int player_id,
                   double volume);
  void OnSetRate(RenderFrameHost* render_frame_host,
                 int player_id,
                 double rate);
  void OnInitialize(RenderFrameHost* render_frame_host,
                    int player_id,
                    const MediaPlayerInitConfig& config);
  void OnInitializeSync(RenderFrameHost* render_frame_host,
                        int player_id,
                        const MediaPlayerInitConfig& config,
                        bool* success);
  void OnDestroy(RenderFrameHost* render_frame_host, int player_id);
  void OnSeek(RenderFrameHost* render_frame_host,
              int player_id,
              base::TimeDelta time);

  void OnSuspend(RenderFrameHost* render_frame_host, int player_id);
  void OnResume(RenderFrameHost* render_frame_host, int player_id);
  void OnActivate(RenderFrameHost* render_frame_host, int player_id);
  void OnDeactivate(RenderFrameHost* render_frame_host, int player_id);

  void OnEnteredFullscreen(int player_id);
  void OnExitedFullscreen(int player_id);

  void CleanUpBelongToRenderFrameHost(RenderFrameHost* render_frame_host);
  WebContents* GetWebContents(int player_id) override;

  void OnGetStartDate(int player_id, double* start_date);

#if defined(TIZEN_VIDEO_HOLE)
  virtual gfx::Rect GetViewportRect(int player_id) const;
  void OnSetGeometry(int player_id, const gfx::RectF& rect);
  void OnWebViewMoved();
#endif

 protected:
  // Clients must use Create() or subclass constructor.
  explicit BrowserMediaPlayerManagerEfl() = default;

  void AddPlayer(int player_id, media::MediaPlayerInterfaceEfl* player);
  void RemovePlayer(int player_id);

  // To continue initializing of other players in queue.
  void OnInitComplete(int player_id, bool success) override;

  // To manage resumed/suspended player id.
  void OnResumeComplete(int player_id) override;
  void OnSuspendComplete(int player_id) override;

  // Helper function to send messages to RenderFrameObserver.
  bool Send(int player_id, IPC::Message* msg);

  void OnPlayerSuspendRequest(int player_id) override;

  RenderFrameHost* GetRenderFrameHostFromPlayerID(int player_id) const;
  int GetRoutingID(int player_id) const;

 private:
  friend struct base::DefaultSingletonTraits<BrowserMediaPlayerManagerEfl>;

  // MediaCapabilityManager graph traversing API
  bool ActivatePlayer(media::MediaPlayerInterfaceEfl* player);
  void Activate(media::MediaPlayerInterfaceEfl* player);
  void Deactivate(media::MediaPlayerInterfaceEfl* player);
  void Show(media::MediaPlayerInterfaceEfl* player);
  void Hide(media::MediaPlayerInterfaceEfl* player);
  void SuspendPlayer(media::MediaPlayerInterfaceEfl* player, bool preempted);
  std::function<bool(media::MediaPlayerInterfaceEfl*)> PreemptPlayerCb();

  bool Initialize(RenderFrameHost* render_frame_host,
                  int player_id,
                  const MediaPlayerInitConfig& config);

  void OnVideoSlotsAvailableChanged(unsigned slots_available);

  // An map of managed players.
  typedef std::unordered_map<int,
                             std::unique_ptr<media::MediaPlayerInterfaceEfl>>
      PlayerMap;
  PlayerMap players_;
  std::unordered_map<int, bool> encrypted_listeners_;

  typedef std::unordered_map<int, RenderFrameHost*> RenderFrameHostMap;
  RenderFrameHostMap render_frame_host_map_;

  media::MediaCapabilityManager<media::LongestConflictStrategy>
      media_capability_manager_;

#if defined(TIZEN_TBM_SUPPORT) && defined(OS_TIZEN_TV_PRODUCT)
  int media_packet_mode_;
#endif
  class PlayersCountHelper;

  DISALLOW_COPY_AND_ASSIGN(BrowserMediaPlayerManagerEfl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_TIZEN_BROWSER_MEDIA_PLAYER_MANAGER_EFL_H_
