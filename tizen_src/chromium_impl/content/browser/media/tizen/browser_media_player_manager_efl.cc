// Copyright 2014 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/tizen/browser_media_player_manager_efl.h"

#include "base/memory/shared_memory.h"
#include "content/browser/media/tizen/browser_demuxer_efl.h"
#include "content/browser/renderer_host/render_process_host_impl.h"
#include "content/common/media/media_player_messages_efl.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_client.h"
#include "ewk/efl_integration/browser_context_efl.h"
#include "ipc/ipc_channel_proxy.h"
#include "ipc/ipc_logging.h"
#include "media/base/tizen/placeholder_media_player.h"
#include "media/base/tizen/suspending_media_player.h"
#include "tizen/system_info.h"

#if defined(TIZEN_TBM_SUPPORT)
#include <media_packet.h>
#endif

#if defined(TIZEN_SOUND_FOCUS)
#include "media/base/tizen/sound_focus_manager.h"
#endif

#if defined(TIZEN_VIDEO_HOLE)
#include "content/browser/renderer_host/render_widget_host_view_efl.h"
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
#include "common/application_type.h"
#include "ewk/efl_integration/web_contents_delegate_efl.h"
#include "tizen_src/chromium_impl/media/base/tizen/media_player_bridge_capi.h"
#endif

#include <set>
#include <tuple>

namespace {

#if defined(OS_TIZEN_TV_PRODUCT)
double GetAmplifiedVolume(double volume) {
  static int gain_index_mapping_table[101] = {
      0,  1,  2,  3,  4,  6,  7,  9,  12, 14, 16, 18, 20, 23, 26, 29, 32,
      34, 36, 38, 41, 42, 44, 46, 47, 49, 51, 52, 53, 54, 56, 57, 58, 59,
      61, 62, 63, 63, 64, 66, 67, 67, 68, 69, 69, 71, 71, 72, 73, 73, 74,
      74, 76, 76, 77, 77, 78, 78, 78, 79, 79, 81, 81, 81, 82, 82, 82, 83,
      83, 83, 84, 84, 84, 84, 86, 86, 86, 86, 86, 87, 87, 87, 87, 87, 88,
      88, 88, 88, 88, 88, 88, 88, 89, 89, 89, 89, 89, 89, 89, 89, 89};

  int control_index = volume * 100;
  return (gain_index_mapping_table[control_index] * 0.01);
}
#endif

// Remove |e| from container |c|.
template <typename C, typename E>
void RemoveFromQueue(C& c, const E& e) {
  c.erase(std::remove(c.begin(), c.end(), e), c.end());
}

template <typename C, typename E>
bool IsExistInQueue(C& c, const E& e) {
  return std::find(c.begin(), c.end(), e) != c.end();
}

}  // namespace

namespace content {

class BrowserMediaPlayerManagerEfl::PlayersCountHelper {
 public:
  explicit PlayersCountHelper(BrowserMediaPlayerManagerEfl* owner,
                              const char* function_name)
      : owner_{owner}, function_name_{function_name} {}
  ~PlayersCountHelper() {
    auto current = GetCount();
    LOG(INFO) << "[VSAC][Counter] before:" << count_ << ", current:" << current
              << " (" << function_name_ << ")";
    if (count_ != current)
      owner_->OnVideoSlotsAvailableChanged(current);
  }

  template <typename CapsManager>
  static unsigned GetEmptyCount(const CapsManager& manager) {
    auto range = manager.template GetActive<media::MediaType::Video>();

    // null entries in the range represent the available slots...
    // also, treat players representing remote streams from
    // peer connections as not occupying any slots.
    const auto is_suspended_remote_peer =
        [](const media::MediaPlayerInterfaceEfl* player) {
          return ((player->GetRoles() & media::PlayerRole::RemotePeerStream) ==
                  media::PlayerRole::RemotePeerStream) &&
                 player->IsPlayerSuspended();
        };

    unsigned result = 0;
    for (const auto player : range) {
      if (!player || is_suspended_remote_peer(player))
        ++result;
    }

    return result;
  }

 private:
  BrowserMediaPlayerManagerEfl* owner_;
  const char* function_name_;
  unsigned count_{GetCount()};

  unsigned GetCount() const {
    return GetEmptyCount(owner_->media_capability_manager_);
  }
};

#if defined(TIZEN_TBM_SUPPORT)
// APIs to manage TBM packet.
void AddMediaPacket(void* media_packet_handle,
                    RenderFrameHost* render_frame_host);

void CleanUpMediaPacket(RenderFrameHost* render_frame_host);
#endif

// GetMaximumPlayerCount logic has been (will be)
// moved to the media_capabilites.h, the
// media::platform namespace

// GetContentClient() is defined in content_client.cc, but in content_client.h
// it is hidden by CONTENT_IMPLEMENTATION ifdef. We don't want to define
// CONTENT_IMPLEMENTATION because it may bring a lot of things we don't need.
ContentClient* GetContentClient();

BrowserMediaPlayerManagerEfl* BrowserMediaPlayerManagerEfl::GetInstance() {
  return base::Singleton<BrowserMediaPlayerManagerEfl>::get();
}

BrowserMediaPlayerManagerEfl::~BrowserMediaPlayerManagerEfl() {
  // Players can be destroyed during destruction of BMPM without clear(),
  // but not to make destruction order dependencies with other member variables
  // we explicitly destroy player instances here.
  players_.clear();
}

media::MediaPlayerInterfaceEfl* BrowserMediaPlayerManagerEfl::GetPlayer(
    int player_id) {
  auto player_iter = players_.find(player_id);
  return player_iter != players_.end() ? player_iter->second.get() : nullptr;
}

void BrowserMediaPlayerManagerEfl::OnInitComplete(int player_id, bool success) {
}

void BrowserMediaPlayerManagerEfl::OnResumeComplete(int player_id) {
  // TODO: Send a message to the HTMLMediaElement to restore state
  Send(player_id, new MediaPlayerEflMsg_PlayerResumed(GetRoutingID(player_id),
                                                      player_id, false));
}

void BrowserMediaPlayerManagerEfl::OnSuspendComplete(int player_id) {
#if defined(TIZEN_SOUND_FOCUS)
  media::MediaPlayerInterfaceEfl* player = GetPlayer(player_id);
  if (player)
    media::SoundFocusManager::GetInstance()->DeregisterClient(player);
#endif
}

void BrowserMediaPlayerManagerEfl::OnNewFrameAvailable(
    int player_id,
    base::SharedMemoryHandle foreign_memory_handle,
    uint32_t length,
    base::TimeDelta timestamp) {
  Send(player_id, new MediaPlayerEflMsg_NewFrameAvailable(
                      GetRoutingID(player_id), player_id, foreign_memory_handle,
                      length, timestamp));
}

#if defined(TIZEN_TBM_SUPPORT)
#if defined(OS_TIZEN_TV_PRODUCT)
void BrowserMediaPlayerManagerEfl::OnControlMediaPacket(int player_id,
                                                        int mode) {
  media_packet_mode_ = mode;
  LOG(INFO) << " mode : " << mode;
}
#endif

void BrowserMediaPlayerManagerEfl::OnNewTbmBufferAvailable(
    int player_id,
    gfx::TbmBufferHandle tbm_handle,
    base::TimeDelta timestamp) {
  RenderFrameHost* rfh = GetRenderFrameHostFromPlayerID(player_id);
  if (!rfh)
    return;

#if defined(OS_TIZEN_TV_PRODUCT)
  if (media_packet_mode_ == gfx::MEDIA_PACKET_RING_BUFFER_MODE) {
    media_packet_destroy(static_cast<media_packet_h>(tbm_handle.media_packet));
    LOG(INFO) << "Force media packet skip!";
    return;
  }
#endif

  AddMediaPacket(tbm_handle.media_packet, rfh);

#if defined(OS_TIZEN_TV_PRODUCT)
  void* extra;
  if (media_packet_get_extra(
          static_cast<media_packet_h>(tbm_handle.media_packet), &extra) ==
      MEDIA_PACKET_ERROR_NONE) {
    LOG(INFO) << "extra = " << reinterpret_cast<int>(extra)
              << " mode: " << media_packet_mode_;
  } else {
    LOG(INFO) << "media_packet_get_extra is failed...";
  }

  Send(player_id, new MediaPlayerEflMsg_NewTbmBufferAvailable(
                      rfh->GetRoutingID(), player_id, tbm_handle,
                      reinterpret_cast<int>(extra), timestamp));
#else
  Send(player_id,
       new MediaPlayerEflMsg_NewTbmBufferAvailable(
           GetRoutingID(player_id), player_id, tbm_handle, timestamp));
#endif
}
#endif

void BrowserMediaPlayerManagerEfl::OnTimeChanged(int player_id) {
  Send(player_id,
       new MediaPlayerEflMsg_TimeChanged(GetRoutingID(player_id), player_id));
}

void BrowserMediaPlayerManagerEfl::OnSeekComplete(int player_id) {
  Send(player_id, new MediaPlayerEflMsg_OnSeekComplete(GetRoutingID(player_id),
                                                       player_id));
}

void BrowserMediaPlayerManagerEfl::OnPauseStateChange(int player_id,
                                                      bool state) {
  Send(player_id, new MediaPlayerEflMsg_PauseStateChanged(
                      GetRoutingID(player_id), player_id, state));
}

void BrowserMediaPlayerManagerEfl::OnRequestSeek(int player_id,
                                                 base::TimeDelta seek_time) {
  // To handle internal seek.
  Send(player_id, new MediaPlayerEflMsg_SeekRequest(GetRoutingID(player_id),
                                                    player_id, seek_time));
}

void BrowserMediaPlayerManagerEfl::OnTimeUpdate(int player_id,
                                                base::TimeDelta current_time) {
  Send(player_id, new MediaPlayerEflMsg_TimeUpdate(GetRoutingID(player_id),
                                                   player_id, current_time));
}

void BrowserMediaPlayerManagerEfl::OnBufferUpdate(int player_id,
                                                  int percentage) {
  Send(player_id, new MediaPlayerEflMsg_BufferUpdate(GetRoutingID(player_id),
                                                     player_id, percentage));
}

void BrowserMediaPlayerManagerEfl::OnDurationChange(int player_id,
                                                    base::TimeDelta duration) {
  Send(player_id, new MediaPlayerEflMsg_DurationChanged(GetRoutingID(player_id),
                                                        player_id, duration));
}

void BrowserMediaPlayerManagerEfl::OnReadyStateChange(
    int player_id,
    blink::WebMediaPlayer::ReadyState state) {
  Send(player_id, new MediaPlayerEflMsg_ReadyStateChange(
                      GetRoutingID(player_id), player_id, state));
}

void BrowserMediaPlayerManagerEfl::OnNetworkStateChange(
    int player_id,
    blink::WebMediaPlayer::NetworkState state) {
  Send(player_id, new MediaPlayerEflMsg_NetworkStateChange(
                      GetRoutingID(player_id), player_id, state));
}

void BrowserMediaPlayerManagerEfl::OnMediaDataChange(int player_id,
                                                     int width,
                                                     int height,
                                                     int media) {
  auto player = GetPlayer(player_id);

#if defined(OS_TIZEN_TV_PRODUCT)
  if (media_capability_manager_.GetState(player) ==
          media::PlayerState::RUNNING &&
      !IsHbbTV())
#else
  if (media_capability_manager_.GetState(player) == media::PlayerState::RUNNING)
#endif
    ActivatePlayer(player);

  Send(player_id,
       new MediaPlayerEflMsg_MediaDataChanged(GetRoutingID(player_id),
                                              player_id, width, height, media));
}

RenderFrameHost* BrowserMediaPlayerManagerEfl::GetRenderFrameHostFromPlayerID(
    int player_id) const {
  auto iter = render_frame_host_map_.find(player_id);
  if (iter == render_frame_host_map_.end()) {
    // TODO(sam) : Does it better remove player in here?
    // Logically player should be removed before destroy render frame host.
    // If we find the path for this case, consider remove player here or not.
    LOG(ERROR) << " No RenderFrameHost for id : " << player_id;
    return nullptr;
  }
  return iter->second;
}

int BrowserMediaPlayerManagerEfl::GetRoutingID(int player_id) const {
  RenderFrameHost* rfh = GetRenderFrameHostFromPlayerID(player_id);
  return rfh ? rfh->GetRoutingID() : 0;
}

void BrowserMediaPlayerManagerEfl::OnPlayerSuspendRequest(int player_id) {
  if (auto player = GetPlayer(player_id))
    Deactivate(player);
}

bool BrowserMediaPlayerManagerEfl::Send(int player_id, IPC::Message* msg) {
  RenderFrameHost* rfh = GetRenderFrameHostFromPlayerID(player_id);
  if (!rfh)
    return false;

  return rfh->Send(msg);
}

class AddPlayerHelper {
 public:
  AddPlayerHelper(WebContents* web_contents, int demuxer_client_id)
      : web_contents_{web_contents}, demuxer_client_id_{demuxer_client_id} {}

  std::string GetUserAgent() const {
    std::string user_agent = web_contents_->GetUserAgentOverride();
    if (user_agent.empty())
      user_agent = content::GetContentClient()->GetUserAgent();
    return user_agent;
  }

  int GetAudioLatencyMode() const {
    auto browser_context =
        static_cast<BrowserContextEfl*>(web_contents_->GetBrowserContext());
    return browser_context->GetAudioLatencyMode();
  }

  GURL GetParsedUrl(const GURL& url) const {
    auto browser_context =
        static_cast<BrowserContextEfl*>(web_contents_->GetBrowserContext());
    GURL parsed_url;
    if (!browser_context->GetWrtParsedUrl(url, parsed_url))
      parsed_url = url;
    return parsed_url;
  }

  std::unique_ptr<media::DemuxerEfl> CreateDemuxer(
      RenderProcessHostImpl* host) const {
    auto browser_demuxer_efl =
        static_cast<BrowserDemuxerEfl*>(host->browser_demuxer_efl().get());
    return browser_demuxer_efl->CreateDemuxer(demuxer_client_id_);
  }

  RenderProcessHostImpl* HostFromMainFrame() const {
    return static_cast<RenderProcessHostImpl*>(
        web_contents_->GetMainFrame()->GetProcess());
  }

  RenderProcessHostImpl* HostFromRenderer() const {
    return static_cast<RenderProcessHostImpl*>(
        web_contents_->GetRenderProcessHost());
  }

  RenderWidgetHostViewEfl* GetRenderWidgetHostView() const {
    return static_cast<RenderWidgetHostViewEfl*>(
        web_contents_->GetRenderWidgetHostView());
  }

 private:
  WebContents* web_contents_;
  int demuxer_client_id_;
};

bool BrowserMediaPlayerManagerEfl::Initialize(
    RenderFrameHost* render_frame_host,
    int player_id,
    const MediaPlayerInitConfig& config) {
  RemovePlayer(player_id);

  if (!render_frame_host) {
    LOG(ERROR) << "Invalid RenderFrameHost";
    OnNetworkStateChange(player_id, blink::WebMediaPlayer::kNetworkStateEmpty);
    return false;
  }

  bool use_video_hole = false;
  auto type = config.type;
  const auto orig_type = type;
#if defined(TIZEN_VIDEO_HOLE)
  if (type == MEDIA_PLAYER_TYPE_URL_WITH_VIDEO_HOLE) {
    type = MEDIA_PLAYER_TYPE_URL;
    use_video_hole = true;
  } else if (type == MEDIA_PLAYER_TYPE_MEDIA_SOURCE_WITH_VIDEO_HOLE) {
    type = MEDIA_PLAYER_TYPE_MEDIA_SOURCE;
    use_video_hole = true;
  } else if (type == MEDIA_PLAYER_TYPE_MEDIA_STREAM_WITH_VIDEO_HOLE) {
    type = MEDIA_PLAYER_TYPE_MEDIA_STREAM;
    use_video_hole = true;
  } else if (type == MEDIA_PLAYER_TYPE_WEBRTC_MEDIA_STREAM_WITH_VIDEO_HOLE) {
    type = MEDIA_PLAYER_TYPE_MEDIA_STREAM;
    use_video_hole = true;

    const auto has_any =
        PlayersCountHelper::GetEmptyCount(media_capability_manager_);
    if (!has_any) {
      LOG(ERROR)
          << "Not initializing WebRTC player due to other players being active";
      return false;
    }
  }
#endif

#if defined(TIZEN_TBM_SUPPORT) && defined(OS_TIZEN_TV_PRODUCT)
  media_packet_mode_ = gfx::MEDIA_PACKET_NORMAL_MODE;
#endif
  LOG(INFO) << __FUNCTION__
            << " [BROWSER] route:" << render_frame_host->GetRoutingID()
            << ", player:" << player_id << ", type:" << static_cast<int>(type)
            << ":" << static_cast<int>(orig_type);

  render_frame_host_map_[player_id] = render_frame_host;
  AddPlayerHelper helper{WebContents::FromRenderFrameHost(render_frame_host),
                         config.demuxer_client_id};

  media::MediaPlayerInterfaceEfl* player = nullptr;
  switch (type) {
    case MEDIA_PLAYER_TYPE_URL:
      AddPlayer(player_id, player = media::SuspendingMediaPlayer::CreatePlayer(
                               player_id, helper.GetParsedUrl(config.url),
                               config.mime_type, this, helper.GetUserAgent(),
                               helper.GetAudioLatencyMode(), use_video_hole));
      break;
    case MEDIA_PLAYER_TYPE_MEDIA_SOURCE:
      AddPlayer(player_id,
                player = media::SuspendingMediaPlayer::CreatePlayer(
                    player_id, helper.CreateDemuxer(helper.HostFromMainFrame()),
                    this, use_video_hole));
      break;
    case MEDIA_PLAYER_TYPE_MEDIA_STREAM:
      AddPlayer(player_id,
                player = media::SuspendingMediaPlayer::CreateStreamPlayer(
                    player_id, helper.CreateDemuxer(helper.HostFromRenderer()),
                    this, use_video_hole));
      break;
    case MEDIA_PLAYER_TYPE_WEBRTC_PLACEHOLDER:
      AddPlayer(player_id, player = media::PlaceholderMediaPlayer::CreatePlayer(
                               player_id, this));
      LOG(INFO) << player_id << " -> placeholder:" << player;
      break;
    default:
      LOG(ERROR) << "Load type is wrong!";
      OnNetworkStateChange(player_id,
                           blink::WebMediaPlayer::kNetworkStateEmpty);
      return false;
  }

#if defined(TIZEN_VIDEO_HOLE) && defined(OS_TIZEN_TV_PRODUCT)
  if (auto view_efl = helper.GetRenderWidgetHostView()) {
    view_efl->SetWebViewMovedCallback(base::Bind(
        &BrowserMediaPlayerManagerEfl::OnWebViewMoved, base::Unretained(this)));
    LOG(INFO) << "SetPositionMovedCallbacks called!";
  }
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
  auto player_e = GetPlayer(player_id);
  if (!player_e) {
    LOG(ERROR) << "No player!";
    return false;
  }
  player_e->SetHasEncryptedListenerOrCdm(config.has_encrypted_listener_or_cdm);
#endif

// Hbbtv run in preloading mode
// delay to activePlayer when really play
#if defined(OS_TIZEN_TV_PRODUCT)
  if (IsHbbTV() || ActivatePlayer(player))
#else
  if (ActivatePlayer(player))
#endif
    player->Initialize();

  return true;
}

void BrowserMediaPlayerManagerEfl::OnInitialize(
    RenderFrameHost* render_frame_host,
    int player_id,
    const MediaPlayerInitConfig& config) {
  Initialize(render_frame_host, player_id, config);
}

void BrowserMediaPlayerManagerEfl::OnInitializeSync(
    RenderFrameHost* render_frame_host,
    int player_id,
    const MediaPlayerInitConfig& config,
    bool* success) {
  bool ret = Initialize(render_frame_host, player_id, config);
  *success = ret;
}

void BrowserMediaPlayerManagerEfl::OnDestroy(
    RenderFrameHost* /*render_frame_host*/,
    int player_id) {
  RemovePlayer(player_id);
  render_frame_host_map_.erase(player_id);
}

void BrowserMediaPlayerManagerEfl::OnPlayerDestroyed(int player_id) {
  Send(player_id, new MediaPlayerEflMsg_PlayerDestroyed(GetRoutingID(player_id),
                                                        player_id));
}

bool BrowserMediaPlayerManagerEfl::ActivatePlayer(
    media::MediaPlayerInterfaceEfl* player) {
  PlayersCountHelper here{this, "ActivatePlayer"};
  return media_capability_manager_.MarkActive(player, PreemptPlayerCb());
}

void BrowserMediaPlayerManagerEfl::Activate(
    media::MediaPlayerInterfaceEfl* player) {
  if (ActivatePlayer(player)) {
#if defined(OS_TIZEN_TV_PRODUCT)
    player->ResetResourceConflict();
#endif
    player->Resume();
  }
}

void BrowserMediaPlayerManagerEfl::Deactivate(
    media::MediaPlayerInterfaceEfl* player) {
  PlayersCountHelper here{this, "Deactivate"};
  media_capability_manager_.MarkInactive(player);
  SuspendPlayer(player, true);
}

void BrowserMediaPlayerManagerEfl::Show(
    media::MediaPlayerInterfaceEfl* player) {
  PlayersCountHelper here{this, "Show"};

#if defined(OS_TIZEN_TV_PRODUCT)
  if (player->IsResourceConflict()) {
    ActivatePlayer(player);
    player->ResetResourceConflict();
  }
#endif

  if (media_capability_manager_.MarkVisible(player, PreemptPlayerCb()))
    player->Resume();
}

void BrowserMediaPlayerManagerEfl::Hide(
    media::MediaPlayerInterfaceEfl* player) {
  PlayersCountHelper here{this, "Hide"};
  media_capability_manager_.MarkHidden(player);
  SuspendPlayer(player, false);
}

void BrowserMediaPlayerManagerEfl::SuspendPlayer(
    media::MediaPlayerInterfaceEfl* player,
    bool preempted) {
  player->Suspend();
  auto player_id = player->GetPlayerId();
  Send(player_id, new MediaPlayerEflMsg_PlayerSuspend(GetRoutingID(player_id),
                                                      player_id, preempted));
}

std::function<bool(media::MediaPlayerInterfaceEfl*)>
BrowserMediaPlayerManagerEfl::PreemptPlayerCb() {
  return [this](media::MediaPlayerInterfaceEfl* player) {
    SuspendPlayer(player, true);
    return true;
  };
}

#if defined(OS_TIZEN_TV_PRODUCT)
void BrowserMediaPlayerManagerEfl::OnNotifySubtitlePlay(
    int player_id,
    int id,
    const std::string& url,
    const std::string& lang) {
  media::MediaPlayerInterfaceEfl* player = GetPlayer(player_id);
  if (!player) {
    LOG(ERROR) << "Invalid player ID : " << player_id;
    return;
  }
  player->NotifySubtitlePlay(id, url, lang);
}

void BrowserMediaPlayerManagerEfl::GetPlayTrackInfo(int player_id, int id) {
  Send(player_id, new MediaPlayerEflMsg_GetPlayTrackInfo(
                      GetRoutingID(player_id), player_id, id));
}

void BrowserMediaPlayerManagerEfl::OnDrmError(int player_id) {
  Send(player_id,
       new MediaPlayerEflMsg_DrmError(GetRoutingID(player_id), player_id));
}

void BrowserMediaPlayerManagerEfl::OnPlayerResourceConflict(int player_id) {
  LOG(INFO) << "OnPlayerResourceConflict :" << player_id;
  if (auto player = GetPlayer(player_id)) {
    Deactivate(player);
  }
}

void BrowserMediaPlayerManagerEfl::OnSeekableTimeChange(
    int player_id,
    base::TimeDelta min_time,
    base::TimeDelta max_time) {
  Send(player_id, new MediaPlayerEflMsg_SeekableTimeChanged(
                      GetRoutingID(player_id), player_id, min_time, max_time));
}

void BrowserMediaPlayerManagerEfl::OnRegisterTimeline(
    int player_id,
    const std::string& timeline_selector,
    bool* ret) {
  media::MediaPlayerInterfaceEfl* player = GetPlayer(player_id);
  if (!player) {
    LOG(ERROR) << "Invalid player ID : " << player_id;
    *ret = false;
    return;
  }
  *ret = player->RegisterTimeline(timeline_selector);
}

void BrowserMediaPlayerManagerEfl::OnUnRegisterTimeline(
    int player_id,
    const std::string& timeline_selector,
    bool* ret) {
  media::MediaPlayerInterfaceEfl* player = GetPlayer(player_id);
  if (!player) {
    LOG(ERROR) << "Invalid player ID : " << player_id;
    *ret = false;
    return;
  }
  *ret = player->UnRegisterTimeline(timeline_selector);
}

void BrowserMediaPlayerManagerEfl::OnGetTimelinePositions(
    int player_id,
    const std::string& timeline_selector,
    uint32_t* units_per_tick,
    uint32_t* units_per_second,
    int64_t* content_time,
    bool* paused) {
  media::MediaPlayerInterfaceEfl* player = GetPlayer(player_id);
  if (!player) {
    LOG(ERROR) << "Invalid player ID : " << player_id;
    return;
  }
  player->GetTimelinePositions(timeline_selector, units_per_tick,
                               units_per_second, content_time, paused);
}

void BrowserMediaPlayerManagerEfl::OnGetSpeed(int player_id, double* speed) {
  media::MediaPlayerInterfaceEfl* player = GetPlayer(player_id);
  if (!player) {
    LOG(ERROR) << "Invalid player ID : " << player_id;
    *speed = 0.0;
    return;
  }
  *speed = player->GetSpeed();
}

void BrowserMediaPlayerManagerEfl::OnGetMrsUrl(int player_id,
                                               std::string* url) {
  DCHECK(url);
  media::MediaPlayerInterfaceEfl* player = GetPlayer(player_id);
  if (!player) {
    LOG(ERROR) << "Invalid player ID : " << player_id;
    *url = "";
    return;
  }
  *url = player->GetMrsUrl();
}

void BrowserMediaPlayerManagerEfl::OnGetContentId(int player_id,
                                                  std::string* contentId) {
  DCHECK(contentId);
  media::MediaPlayerInterfaceEfl* player = GetPlayer(player_id);
  if (!player) {
    LOG(ERROR) << "Invalid player ID : " << player_id;
    *contentId = "";
    return;
  }
  *contentId = player->GetContentId();
}

void BrowserMediaPlayerManagerEfl::OnSyncTimeline(
    int player_id,
    const std::string& timeline_selector,
    int64_t timeline_pos,
    int64_t wallclock_pos,
    int tolerance,
    bool* ret) {
  media::MediaPlayerInterfaceEfl* player = GetPlayer(player_id);
  if (!player) {
    LOG(ERROR) << "Invalid player ID : " << player_id;
    *ret = false;
    return;
  }

  *ret = player->SyncTimeline(timeline_selector, timeline_pos, wallclock_pos,
                              tolerance);
}

void BrowserMediaPlayerManagerEfl::OnSetWallClock(
    int player_id,
    const std::string& wallclock_url,
    bool* ret) {
  media::MediaPlayerInterfaceEfl* player = GetPlayer(player_id);
  if (!player) {
    LOG(ERROR) << "Invalid player ID : " << player_id;
    *ret = false;
    return;
  }
  *ret = player->SetWallClock(wallclock_url);
}

void BrowserMediaPlayerManagerEfl::OnRegisterTimelineCbInfo(
    int player_id,
    const blink::WebMediaPlayer::register_timeline_cb_info_s& info) {
  Send(player_id, new MediaPlayerEflMsg_RegisterTimelineCb(
                      GetRoutingID(player_id), player_id, info));
}

void BrowserMediaPlayerManagerEfl::OnSyncTimelineCbInfo(
    int player_id,
    const std::string& timeline_selector,
    int sync) {
  Send(player_id,
       new MediaPlayerEflMsg_SyncTimelineCb(GetRoutingID(player_id), player_id,
                                            timeline_selector, sync));
}

void BrowserMediaPlayerManagerEfl::OnMrsUrlChange(int player_id,
                                                  const std::string& url) {
  Send(player_id, new MediaPlayerEflMsg_MrsUrlChange(GetRoutingID(player_id),
                                                     player_id, url));
}

void BrowserMediaPlayerManagerEfl::OnContentIdChange(int player_id,
                                                     const std::string& id) {
  Send(player_id, new MediaPlayerEflMsg_ContentIdChange(GetRoutingID(player_id),
                                                        player_id, id));
}

void BrowserMediaPlayerManagerEfl::OnAddAudioTrack(
    int player_id,
    const blink::WebMediaPlayer::audio_video_track_info_s& info) {
  Send(player_id, new MediaPlayerEflMsg_AddAudioTrack(GetRoutingID(player_id),
                                                      player_id, info));
}

void BrowserMediaPlayerManagerEfl::OnAddVideoTrack(
    int player_id,
    const blink::WebMediaPlayer::audio_video_track_info_s& info) {
  Send(player_id, new MediaPlayerEflMsg_AddVideoTrack(GetRoutingID(player_id),
                                                      player_id, info));
}

void BrowserMediaPlayerManagerEfl::OnAddTextTrack(int player_id,
                                                  const std::string& label,
                                                  const std::string& language,
                                                  const std::string& id) {
  Send(player_id, new MediaPlayerEflMsg_AddTextTrack(
                      GetRoutingID(player_id), player_id, label, language, id));
}

void BrowserMediaPlayerManagerEfl::OnSetActiveTextTrack(int player_id,
                                                        int id,
                                                        bool is_in_band) {
  media::MediaPlayerInterfaceEfl* player = GetPlayer(player_id);
  if (!player) {
    LOG(ERROR) << "Invalid player ID : " << player_id;
    return;
  }
  player->SetActiveTextTrack(id, is_in_band);
}

void BrowserMediaPlayerManagerEfl::OnSetActiveAudioTrack(int player_id,
                                                         int index) {
  media::MediaPlayerInterfaceEfl* player = GetPlayer(player_id);
  if (!player) {
    LOG(ERROR) << "Invalid player ID : " << player_id;
    return;
  }
  player->SetActiveAudioTrack(index);
}

void BrowserMediaPlayerManagerEfl::OnSetActiveVideoTrack(int player_id,
                                                         int index) {
  media::MediaPlayerInterfaceEfl* player = GetPlayer(player_id);
  if (!player) {
    LOG(ERROR) << "Invalid player ID : " << player_id;
    return;
  }
  player->SetActiveVideoTrack(index);
}

void BrowserMediaPlayerManagerEfl::OnInbandEventTextTrack(
    int player_id,
    const std::string& info,
    int action) {
  Send(player_id, new MediaPlayerEflMsg_InbandEventTextTrack(
                      GetRoutingID(player_id), player_id, info, action));
}

void BrowserMediaPlayerManagerEfl::OnInbandEventTextCue(
    int player_id,
    const std::string& info,
    unsigned int id,
    long long int start_time,
    long long int end_time) {
  Send(player_id,
       new MediaPlayerEflMsg_InbandEventTextCue(
           GetRoutingID(player_id), player_id, info, id, start_time, end_time));
}

void BrowserMediaPlayerManagerEfl::OnGetDroppedFrameCount(
    int player_id,
    unsigned* dropped_frame_count) {
  media::MediaPlayerInterfaceEfl* player = GetPlayer(player_id);
  if (player)
    *dropped_frame_count = player->GetDroppedFrameCount();
  else
    *dropped_frame_count = 0;
}

void BrowserMediaPlayerManagerEfl::OnGetDecodedFrameCount(
    int player_id,
    unsigned* decoded_frame_count) {
  media::MediaPlayerInterfaceEfl* player = GetPlayer(player_id);
  if (player)
    *decoded_frame_count = player->GetDecodedFrameCount();
  else
    *decoded_frame_count = 0;
}

void BrowserMediaPlayerManagerEfl::OnElementRemove(int player_id) {
  media::MediaPlayerInterfaceEfl* player = GetPlayer(player_id);
  if (!player) {
    LOG(ERROR) << "Invalid player ID : " << player_id;
    return;
  }
  player->ElementRemove();
}

void BrowserMediaPlayerManagerEfl::OnSetParentalRatingResult(int player_id,
                                                             bool is_pass) {
  media::MediaPlayerInterfaceEfl* player = GetPlayer(player_id);
  if (!player) {
    LOG(ERROR) << "Invalid player ID : " << player_id;
    return;
  }
  player->SetParentalRatingResult(is_pass);
}

void BrowserMediaPlayerManagerEfl::OnPlayerStarted(int player_id,
                                                   bool play_started) {
  Send(player_id, new MediaPlayerEflMsg_IsPlayerStarted(
                      GetRoutingID(player_id), player_id, play_started));
}

void BrowserMediaPlayerManagerEfl::OnCheckHLS(const std::string& url,
                                              bool* support) {
  media::MediaPlayerBridgeCapi::CheckHLSSupport(url, support);
}
#endif

void BrowserMediaPlayerManagerEfl::OnPlay(
    RenderFrameHost* /*render_frame_host*/,
    int player_id) {
  media::MediaPlayerInterfaceEfl* player = GetPlayer(player_id);
  if (!player)
    return;

// For ADINS transition perfermance,only resume play need ActivatePlayer
// For normal play,don't active player first,wait for resouce conflict
#if defined(OS_TIZEN_TV_PRODUCT)
  if (IsHbbTV() && player->IsPlayerSuspended())
    Activate(player);
#endif

#if defined(TIZEN_SOUND_FOCUS)
  media::SoundFocusManager::GetInstance()->RegisterClient(player);
  player->SetResumePlayingBySFM(false);
#endif

  player->Play();
}

void BrowserMediaPlayerManagerEfl::OnPause(
    RenderFrameHost* /*render_frame_host*/,
    int player_id,
    bool is_media_related_action) {
  media::MediaPlayerInterfaceEfl* player = GetPlayer(player_id);
  if (player) {
#if defined(TIZEN_SOUND_FOCUS)
    media::SoundFocusManager::GetInstance()->DeregisterClient(player);
#endif

    player->Pause(is_media_related_action);
  }
}

void BrowserMediaPlayerManagerEfl::OnSetVolume(
    RenderFrameHost* /*render_frame_host*/,
    int player_id,
    double volume) {
  media::MediaPlayerInterfaceEfl* player = GetPlayer(player_id);
  if (player) {
#if defined(OS_TIZEN_TV_PRODUCT)
    volume = GetAmplifiedVolume(volume);
#endif
    player->SetVolume(volume);
  }
}

void BrowserMediaPlayerManagerEfl::OnSetRate(
    RenderFrameHost* /*render_frame_host*/,
    int player_id,
    double rate) {
  media::MediaPlayerInterfaceEfl* player = GetPlayer(player_id);
  if (player)
    player->SetRate(rate);
}

void BrowserMediaPlayerManagerEfl::OnSeek(
    RenderFrameHost* /*render_frame_host*/,
    int player_id,
    base::TimeDelta time) {
  media::MediaPlayerInterfaceEfl* player = GetPlayer(player_id);
  if (player)
    player->Seek(time);
}

void BrowserMediaPlayerManagerEfl::OnSuspend(
    RenderFrameHost* /*render_frame_host*/,
    int player_id) {
  LOG(INFO) << "player_id : " << player_id;
  if (auto player = GetPlayer(player_id))
    Hide(player);
}

void BrowserMediaPlayerManagerEfl::OnResume(
    RenderFrameHost* /*render_frame_host*/,
    int player_id) {
  LOG(INFO) << "player_id : " << player_id;
  if (auto player = GetPlayer(player_id))
    Show(player);
}

void BrowserMediaPlayerManagerEfl::OnActivate(
    RenderFrameHost* /*render_frame_host*/,
    int player_id) {
  if (auto player = GetPlayer(player_id))
    Activate(player);
}

void BrowserMediaPlayerManagerEfl::OnDeactivate(
    RenderFrameHost* /*render_frame_host*/,
    int player_id) {
  if (auto player = GetPlayer(player_id))
    Deactivate(player);
}

void BrowserMediaPlayerManagerEfl::OnGetStartDate(int player_id,
                                                  double* start_date) {
  media::MediaPlayerInterfaceEfl* player = GetPlayer(player_id);
  // Get start date from mmplayer
  if (player)
    *start_date = player->GetStartDate();
  else
    *start_date = std::numeric_limits<double>::quiet_NaN();
}

void BrowserMediaPlayerManagerEfl::OnVideoSlotsAvailableChanged(
    unsigned slots_available) {
  LOG(INFO) << "[VSAC] should distribute " << slots_available << " to "
            << players_.size() << " players...";

  std::set<RenderFrameHost*> seen;

  for (const auto& pair : players_) {
    RenderFrameHost* rfh = GetRenderFrameHostFromPlayerID(pair.first);
    LOG(INFO) << "[VSAC] player:" << pair.first << " -> host:" << rfh;
    if (!rfh)
      continue;

    bool succeeded = false;
    std::tie(std::ignore, succeeded) = seen.insert(rfh);
    if (!succeeded) {
      LOG(INFO) << "[VSAC] host:" << rfh << " (route:" << rfh->GetRoutingID()
                << ") seen...";
      continue;  // we have already seen this RFH...
    }

    LOG(INFO) << "[VSAC] send " << slots_available << " to frame host " << rfh
              << " (via route:" << rfh->GetRoutingID() << ")";
    rfh->Send(new MediaPlayerEflMsg_VideoSlotsAvailableChanged(
        rfh->GetRoutingID(), slots_available));
  }
}

void BrowserMediaPlayerManagerEfl::CleanUpBelongToRenderFrameHost(
    RenderFrameHost* render_frame_host) {
  // clean up player.
  for (auto iter = render_frame_host_map_.begin();
       iter != render_frame_host_map_.end();) {
    if (render_frame_host == iter->second) {
      RemovePlayer(iter->first);
      iter = render_frame_host_map_.erase(iter);
    } else {
      ++iter;
    }
  }

#if defined(TIZEN_TBM_SUPPORT)
  // Clean up all media packet belong to |render_frame_host|.
  CleanUpMediaPacket(render_frame_host);
#endif
}

WebContents* BrowserMediaPlayerManagerEfl::GetWebContents(int player_id) {
  auto iter = render_frame_host_map_.find(player_id);
  if (iter == render_frame_host_map_.end()) {
    // TODO(sam) : Does it better remove player in here?
    // Logically player should be removed before destroy render frame host.
    // If we find the path for this case, consider remove player here or not.
    LOG(ERROR) << " No RenderFrameHost for id : " << player_id;
    return nullptr;
  }

  RenderFrameHost* frameHost = iter->second;
  return WebContents::FromRenderFrameHost(frameHost);
}

void BrowserMediaPlayerManagerEfl::AddPlayer(
    int player_id,
    media::MediaPlayerInterfaceEfl* player) {
  DCHECK(player);
  PlayersCountHelper here{this, "AddPlayer"};
  media_capability_manager_.AddPlayer(player);
  players_.insert(std::make_pair(
      player_id, std::unique_ptr<media::MediaPlayerInterfaceEfl>(player)));
}

void BrowserMediaPlayerManagerEfl::RemovePlayer(int player_id) {
  auto iter = players_.find(player_id);
  if (iter != players_.end()) {
    PlayersCountHelper here{this, "RemovePlayer"};
    media_capability_manager_.RemovePlayer(iter->second.get());
    players_.erase(iter);
  }
}

#if defined(TIZEN_VIDEO_HOLE)
gfx::Rect BrowserMediaPlayerManagerEfl::GetViewportRect(int player_id) const {
  if (render_frame_host_map_.empty()) {
    LOG(ERROR) << "RenderFrameHostMap is empty!";
    return gfx::Rect();
  }

  RenderFrameHost* rfh = GetRenderFrameHostFromPlayerID(player_id);
  if (!rfh) {
    LOG(ERROR) << "Can't find render frame host for player id " << player_id;
    return gfx::Rect();
  }

  WebContents* web_contents = WebContents::FromRenderFrameHost(rfh);

  if (RenderWidgetHostViewEfl* view_efl = static_cast<RenderWidgetHostViewEfl*>(
          web_contents->GetRenderWidgetHostView())) {
    return view_efl->GetViewBoundsInPix();
  }
  return gfx::Rect();
}

#if defined(OS_TIZEN_TV_PRODUCT)
void BrowserMediaPlayerManagerEfl::NotifyPlayingVideoRect(
    int player_id,
    const gfx::RectF& rect) {
  LOG(INFO) << __FUNCTION__ << " player_id: " << player_id;
  if (WebContents* web_contents = GetWebContents(player_id)) {
    if (WebContentsDelegateEfl* delegate =
            static_cast<WebContentsDelegateEfl*>(web_contents->GetDelegate())) {
      delegate->NotifyPlayingVideoRect(rect);
    }
  }
}
#endif

void BrowserMediaPlayerManagerEfl::OnSetGeometry(int player_id,
                                                 const gfx::RectF& rect) {
  media::MediaPlayerInterfaceEfl* player = GetPlayer(player_id);
  if (!player) {
    LOG(ERROR) << "Invalid player ID : " << player_id;
    return;
  }

#if defined(OS_TIZEN_TV_PRODUCT)
  // Floating Video Window only for WebBrowser
  if (IsWebBrowser())
    NotifyPlayingVideoRect(player_id, rect);
#endif

  player->SetGeometry(rect);
}

void BrowserMediaPlayerManagerEfl::OnWebViewMoved() {
  auto active_videos =
      media_capability_manager_.GetActive<media::MediaType::Video>();
  for (const auto player : active_videos) {
    if (!player)
      continue;

    player->OnWebViewMoved();
    LOG(INFO) << "Reset WebView-Position : " << player;
  }
}
#endif

void BrowserMediaPlayerManagerEfl::OnEnteredFullscreen(int player_id) {
  media::MediaPlayerInterfaceEfl* player = GetPlayer(player_id);
  if (!player) {
    LOG(ERROR) << "Invalid player ID : " << player_id;
    return;
  }

  player->EnteredFullscreen();
}

void BrowserMediaPlayerManagerEfl::OnExitedFullscreen(int player_id) {
  media::MediaPlayerInterfaceEfl* player = GetPlayer(player_id);
  if (!player) {
    LOG(ERROR) << "Invalid player ID : " << player_id;
    return;
  }

  player->ExitedFullscreen();
}

#if defined(OS_TIZEN_TV_PRODUCT)
void BrowserMediaPlayerManagerEfl::OnSetDecryptorHandle(
    int player_id,
    eme::eme_decryptor_t decryptor) {
  auto player = GetPlayer(player_id);
  if (!player) {
    LOG(ERROR) << "Invalid player ID : " << player_id;
    return;
  }
  player->SetDecryptor(decryptor);
}

void BrowserMediaPlayerManagerEfl::OnInitData(
    int player_id,
    const std::vector<unsigned char>& init_data) {
  Send(player_id, new MediaPlayerEflMsg_InitData(GetRoutingID(player_id),
                                                 player_id, init_data));
}

void BrowserMediaPlayerManagerEfl::OnWaitingForKey(
    int player_id) {
  Send(player_id, new MediaPlayerEflMsg_WaitingForKey(GetRoutingID(player_id),
                                                      player_id));
}

void BrowserMediaPlayerManagerEfl::OnDestroySync(int player_id) {
  OnDestroy(nullptr, player_id);
}
#endif

}  // namespace content
