// Copyright 2014 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/tizen/renderer_media_player_manager_efl.h"

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "content/common/media/media_player_init_config.h"
#include "content/common/media/media_player_messages_efl.h"
#include "content/public/renderer/render_thread.h"
#include "content/renderer/media/tizen/webmediaplayer_efl.h"
#include "media/base/bind_to_current_loop.h"
#include "tizen_src/ewk/efl_integration/renderer/tizen_extensible.h"

#if defined(TIZEN_VIDEO_HOLE)
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"
#include "third_party/WebKit/public/web/WebView.h"
#endif
namespace content {

RendererMediaPlayerManager* CreateRendererMediaPlayerManager(
    RenderFrame* render_frame) {
  return new RendererMediaPlayerManager(render_frame);
}

#if defined(TIZEN_TBM_SUPPORT)
void OnTbmBufferRelease(gfx::TbmBufferHandle tbm_handle) {
  RenderThread::Get()->Send(
      new MediaPlayerEflHostMsg_ReleaseTbmBuffer(tbm_handle));
}
#endif

RendererMediaPlayerManager::RendererMediaPlayerManager(
    RenderFrame* render_frame)
    : RenderFrameObserver(render_frame) {
}

RendererMediaPlayerManager::~RendererMediaPlayerManager() {
  DCHECK(media_players_.empty())
      << "RendererMediaPlayerManager is owned by RenderFrameImpl and is "
         "destroyed only after all media players are destroyed.";
}

void RendererMediaPlayerManager::PausePlayingPlayers() {
  for (auto player_it : media_players_) {
    blink::WebMediaPlayer* player = player_it.second;
    if (player && !player->Paused() &&
        (player->HasVideo() ||
         (TizenExtensible::GetInstance()->GetExtensibleAPI(
              "background,music") == false)))
      player->RequestPause();
  }
}

void RendererMediaPlayerManager::Suspend(int player_id) {
  LOG(INFO) << "player_id : " << player_id;
  Send(new MediaPlayerEflHostMsg_Suspend(routing_id(), player_id));
}

void RendererMediaPlayerManager::Resume(int player_id) {
  LOG(INFO) << "player_id : " << player_id;
  Send(new MediaPlayerEflHostMsg_Resume(routing_id(), player_id));
}

void RendererMediaPlayerManager::Activate(int player_id) {
  LOG(INFO) << "player_id : " << player_id;
  Send(new MediaPlayerEflHostMsg_Activate(routing_id(), player_id));
}

void RendererMediaPlayerManager::Deactivate(int player_id) {
  LOG(INFO) << "player_id : " << player_id;
  Send(new MediaPlayerEflHostMsg_Deactivate(routing_id(), player_id));
}

bool RendererMediaPlayerManager::OnMessageReceived(
    const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(RendererMediaPlayerManager, message)
  IPC_MESSAGE_HANDLER(MediaPlayerEflMsg_MediaDataChanged, OnMediaDataChange)
  IPC_MESSAGE_HANDLER(MediaPlayerEflMsg_DurationChanged, OnDurationChange)
  IPC_MESSAGE_HANDLER(MediaPlayerEflMsg_TimeUpdate, OnTimeUpdate)
  IPC_MESSAGE_HANDLER(MediaPlayerEflMsg_BufferUpdate, OnBufferUpdate)
  IPC_MESSAGE_HANDLER(MediaPlayerEflMsg_ReadyStateChange, OnReadyStateChange)
  IPC_MESSAGE_HANDLER(MediaPlayerEflMsg_NetworkStateChange,
                      OnNetworkStateChange)
  IPC_MESSAGE_HANDLER(MediaPlayerEflMsg_TimeChanged, OnTimeChanged)
  IPC_MESSAGE_HANDLER(MediaPlayerEflMsg_PauseStateChanged, OnPauseStateChange)
  IPC_MESSAGE_HANDLER(MediaPlayerEflMsg_OnSeekComplete, OnSeekComplete)
  IPC_MESSAGE_HANDLER(MediaPlayerEflMsg_SeekRequest, OnRequestSeek)
  IPC_MESSAGE_HANDLER(MediaPlayerEflMsg_PlayerSuspend, OnPlayerSuspend)
  IPC_MESSAGE_HANDLER(MediaPlayerEflMsg_PlayerResumed, OnPlayerResumed)
  IPC_MESSAGE_HANDLER(MediaPlayerEflMsg_NewFrameAvailable, OnNewFrameAvailable)
#if defined(TIZEN_TBM_SUPPORT)
  IPC_MESSAGE_HANDLER(MediaPlayerEflMsg_NewTbmBufferAvailable,
                      OnNewTbmBufferAvailable)
#endif
#if defined(OS_TIZEN_TV_PRODUCT)
  IPC_MESSAGE_HANDLER(MediaPlayerEflMsg_GetPlayTrackInfo, OnGetPlayTrackInfo)
  IPC_MESSAGE_HANDLER(MediaPlayerEflMsg_DrmError, OnDrmError)
  IPC_MESSAGE_HANDLER(MediaPlayerEflMsg_SeekableTimeChanged,
                      OnSeekableTimeChange)
  IPC_MESSAGE_HANDLER(MediaPlayerEflMsg_SyncTimelineCb, OnSyncTimeline)
  IPC_MESSAGE_HANDLER(MediaPlayerEflMsg_RegisterTimelineCb, OnRegisterTimeline)
  IPC_MESSAGE_HANDLER(MediaPlayerEflMsg_MrsUrlChange, OnMrsUrlChange)
  IPC_MESSAGE_HANDLER(MediaPlayerEflMsg_ContentIdChange, OnContentIdChange)
  IPC_MESSAGE_HANDLER(MediaPlayerEflMsg_AddAudioTrack, OnAddAudioTrack)
  IPC_MESSAGE_HANDLER(MediaPlayerEflMsg_AddVideoTrack, OnAddVideoTrack)
  IPC_MESSAGE_HANDLER(MediaPlayerEflMsg_AddTextTrack, OnAddTextTrack)
  IPC_MESSAGE_HANDLER(MediaPlayerEflMsg_InbandEventTextTrack,
                      OnInbandEventTextTrack)
  IPC_MESSAGE_HANDLER(MediaPlayerEflMsg_InbandEventTextCue,
                      OnInbandEventTextCue)
  IPC_MESSAGE_HANDLER(MediaPlayerEflMsg_IsPlayerStarted, OnPlayerStarted)
  IPC_MESSAGE_HANDLER(MediaPlayerEflMsg_InitData, OnInitData)
  IPC_MESSAGE_HANDLER(MediaPlayerEflMsg_WaitingForKey, OnWaitingForKey)
  IPC_MESSAGE_HANDLER(MediaPlayerEflMsg_VideoSlotsAvailableChanged,
                      OnVideoSlotsAvailableChange)
#endif
  IPC_MESSAGE_HANDLER(MediaPlayerEflMsg_PlayerDestroyed, OnPlayerDestroyed)
  IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void RendererMediaPlayerManager::WasHidden() {
  LOG(INFO) << "RendererMediaPlayerManager::WasHidden()";
  // Note : During suspend sceinario like app switching in TV profile,
  // webview fires both hide and suspend event, so only handling it in suspend.
  // But in mobile, some scenario fires hide event only.
  // (browser app > tab manager)
#if !defined(OS_TIZEN_TV_PRODUCT)
  PausePlayingPlayers();
#endif
}

void RendererMediaPlayerManager::WasShown() {
  LOG(INFO) << "RendererMediaPlayerManager::WasShown()";
}

void RendererMediaPlayerManager::OnStop() {
  PausePlayingPlayers();
}

void RendererMediaPlayerManager::Initialize(
    int player_id,
    MediaPlayerHostMsg_Initialize_Type type,
    const GURL& url,
    const std::string& mime_type,
    int demuxer_client_id) {
  bool has_encrypted_listener_or_cdm = false;
#if defined(OS_TIZEN_TV_PRODUCT)
  auto player = GetMediaPlayer(player_id);
  if (player)
    has_encrypted_listener_or_cdm = player->hasEncryptedListenerOrCdm();
#endif
  MediaPlayerInitConfig config{type, url, mime_type, demuxer_client_id,
                               has_encrypted_listener_or_cdm};
  LOG(INFO) << __FUNCTION__ << " [RENDERER] route:" << routing_id()
            << ", player:" << player_id << ", type:" << static_cast<int>(type);
  Send(new MediaPlayerEflHostMsg_Init(routing_id(), player_id, config));
}

bool RendererMediaPlayerManager::InitializeSync(
    int player_id,
    MediaPlayerHostMsg_Initialize_Type type,
    const GURL& url,
    const std::string& mime_type,
    int demuxer_client_id) {
  LOG(INFO) << __FUNCTION__ << " [RENDERER] route:" << routing_id()
            << ", player:" << player_id << ", type:" << static_cast<int>(type);
  bool success = false;
  Send(new MediaPlayerEflHostMsg_InitSync(routing_id(), player_id, type, url,
                                          mime_type, demuxer_client_id,
                                          &success));
  LOG(INFO) << __FUNCTION__ << " [RENDERER] route:" << routing_id()
            << ", player:" << player_id
            << ", success:" << (success ? "yes" : "no");
  return success;
}

void RendererMediaPlayerManager::OnMediaDataChange(int player_id,
                                                   int width,
                                                   int height,
                                                   int media) {
  auto player = GetMediaPlayer(player_id);
  if (player)
    player->OnMediaDataChange(width, height, media);
}

void RendererMediaPlayerManager::OnPlayerDestroyed(int player_id) {
  auto player = GetMediaPlayer(player_id);
  if (player)
    player->OnPlayerDestroyed();
}

void RendererMediaPlayerManager::OnDurationChange(int player_id,
                                                  base::TimeDelta duration) {
  auto player = GetMediaPlayer(player_id);
  if (player)
    player->OnDurationChange(duration);
}

void RendererMediaPlayerManager::OnTimeUpdate(int player_id,
                                              base::TimeDelta current_time) {
  auto player = GetMediaPlayer(player_id);
  if (player)
    player->OnTimeUpdate(current_time);
}

void RendererMediaPlayerManager::OnBufferUpdate(int player_id, int percentage) {
  auto player = GetMediaPlayer(player_id);
  if (player)
    player->OnBufferUpdate(percentage);
}

void RendererMediaPlayerManager::OnReadyStateChange(
    int player_id,
    blink::WebMediaPlayer::ReadyState state) {
  auto player = GetMediaPlayer(player_id);
  if (player)
    player->SetReadyState(
        static_cast<blink::WebMediaPlayer::ReadyState>(state));
}

void RendererMediaPlayerManager::OnNetworkStateChange(
    int player_id,
    blink::WebMediaPlayer::NetworkState state) {
  auto player = GetMediaPlayer(player_id);
  if (player)
    player->SetNetworkState(
        static_cast<blink::WebMediaPlayer::NetworkState>(state));
}

void RendererMediaPlayerManager::OnTimeChanged(int player_id) {
  auto player = GetMediaPlayer(player_id);
  if (player)
    player->OnTimeChanged();
}

void RendererMediaPlayerManager::OnSeekComplete(int player_id) {
  auto player = GetMediaPlayer(player_id);
  if (player)
    player->OnSeekComplete();
}

void RendererMediaPlayerManager::OnPauseStateChange(int player_id, bool state) {
  auto player = GetMediaPlayer(player_id);
  if (player)
    player->OnPauseStateChange(state);
}

void RendererMediaPlayerManager::OnRequestSeek(int player_id,
                                               base::TimeDelta seek_time) {
  auto player = GetMediaPlayer(player_id);
  if (player)
    player->OnRequestSeek(seek_time);
}

void RendererMediaPlayerManager::OnPlayerSuspend(int player_id,
                                                 bool is_preempted) {
  if (auto player = GetMediaPlayer(player_id))
    player->OnPlayerSuspend(is_preempted);
}

void RendererMediaPlayerManager::OnPlayerResumed(int player_id,
                                                 bool is_preempted) {
  if (auto player = GetMediaPlayer(player_id))
    player->OnPlayerResumed(is_preempted);
}

void RendererMediaPlayerManager::OnNewFrameAvailable(
    int player_id,
    base::SharedMemoryHandle foreign_memory_handle,
    uint32_t length,
    base::TimeDelta timestamp) {
  auto player = GetMediaPlayer(player_id);
  if (player)
    player->OnNewFrameAvailable(foreign_memory_handle, length, timestamp);
}

#if defined(TIZEN_TBM_SUPPORT)
#if defined(OS_TIZEN_TV_PRODUCT)
void RendererMediaPlayerManager::ControlMediaPacket(int player_id, int mode) {
  Send(new MediaPlayerEflHostMsg_ControlMediaPacket(routing_id(), player_id,
                                                    mode));
}
#endif

void RendererMediaPlayerManager::OnNewTbmBufferAvailable(
    int player_id,
    gfx::TbmBufferHandle tbm_handle,
#if defined(OS_TIZEN_TV_PRODUCT)
    int scaler_physical_id,
#endif
    base::TimeDelta timestamp) {
  auto player = GetMediaPlayer(player_id);

  if (player) {
    player->OnNewTbmBufferAvailable(
        tbm_handle,
#if defined(OS_TIZEN_TV_PRODUCT)
        scaler_physical_id,
#endif
        timestamp,
        media::BindToCurrentLoop(
            base::Bind(&content::OnTbmBufferRelease, tbm_handle)));
  } else {
    LOG(INFO) << "Player[" << player_id << "] is already destroyed so "
              << "media packet : " << tbm_handle.media_packet
              << " will be released";
    OnTbmBufferRelease(tbm_handle);
  }
}
#endif

blink::WebMediaPlayer* RendererMediaPlayerManager::GetMediaPlayer(
    int player_id) {
  std::map<int, blink::WebMediaPlayer*>::iterator iter =
      media_players_.find(player_id);
  if (iter != media_players_.end())
    return iter->second;
  return NULL;
}

void RendererMediaPlayerManager::OnDestruct() {
  delete this;
}

void RendererMediaPlayerManager::Play(int player_id) {
  Send(new MediaPlayerEflHostMsg_Play(routing_id(), player_id));
}

void RendererMediaPlayerManager::Pause(int player_id,
                                       bool is_media_related_action) {
  Send(new MediaPlayerEflHostMsg_Pause(routing_id(), player_id,
                                       is_media_related_action));
}

void RendererMediaPlayerManager::Seek(int player_id, base::TimeDelta time) {
  Send(new MediaPlayerEflHostMsg_Seek(routing_id(), player_id, time));
}

void RendererMediaPlayerManager::SetVolume(int player_id, double volume) {
  Send(new MediaPlayerEflHostMsg_SetVolume(routing_id(), player_id, volume));
}

void RendererMediaPlayerManager::SetRate(int player_id, double rate) {
  Send(new MediaPlayerEflHostMsg_SetRate(routing_id(), player_id, rate));
}

void RendererMediaPlayerManager::DestroyPlayer(int player_id) {
  Send(new MediaPlayerEflHostMsg_DeInit(routing_id(), player_id));
}

void RendererMediaPlayerManager::EnteredFullscreen(int player_id) {
  Send(new MediaPlayerEflHostMsg_EnteredFullscreen(routing_id(), player_id));
}

void RendererMediaPlayerManager::ExitedFullscreen(int player_id) {
  Send(new MediaPlayerEflHostMsg_ExitedFullscreen(routing_id(), player_id));
}

double RendererMediaPlayerManager::GetStartDate(int player_id) {
  double start_date = std::numeric_limits<double>::quiet_NaN();
  Send(new MediaPlayerEflHostMsg_GetStartDate(routing_id(), player_id,
                                              &start_date));
  return start_date;
}

#if defined(TIZEN_VIDEO_HOLE)
void RendererMediaPlayerManager::SetMediaGeometry(int player_id,
                                                  const gfx::RectF& rect) {
  gfx::RectF video_rect = rect;
  Send(new MediaPlayerEflHostMsg_SetGeometry(routing_id(), player_id,
                                             video_rect));
}
#endif
int RendererMediaPlayerManager::RegisterMediaPlayer(
    blink::WebMediaPlayer* player) {
  // Note : For the unique player id among the all renderer process,
  // generate player id based on renderer process id.
  static int next_media_player_id_ = base::GetCurrentProcId() << 16;
  next_media_player_id_ = (next_media_player_id_ & 0xFFFF0000) |
                          ((next_media_player_id_ + 1) & 0x0000FFFF);
  media_players_[next_media_player_id_] = player;
  return next_media_player_id_;
}

void RendererMediaPlayerManager::UnregisterMediaPlayer(int player_id) {
  media_players_.erase(player_id);
}

#if defined(OS_TIZEN_TV_PRODUCT)
void RendererMediaPlayerManager::SetDecryptorHandle(
    int player_id,
    eme::eme_decryptor_t decryptor) {
  LOG(INFO) << "SetDecryptorHandle [" << reinterpret_cast<void*>(decryptor)
            << "]";

  Send(new MediaPlayerEflHostMsg_SetDecryptorHandle(routing_id(), player_id,
                                                    decryptor));
}

void RendererMediaPlayerManager::OnInitData(
    int player_id,
    const std::vector<unsigned char>& init_data) {
  auto player = GetMediaPlayer(player_id);
  if (player) {
    player->OnEncryptedMediaInitData(media::EmeInitDataType::CENC, init_data);
  }
}

void RendererMediaPlayerManager::OnWaitingForKey(
    int player_id) {
  auto player = GetMediaPlayer(player_id);
  if (player) {
    player->OnWaitingForDecryptionKey();
  }
}

void RendererMediaPlayerManager::NotifySubtitlePlay(int player_id,
                                                    int id,
                                                    const std::string& url,
                                                    const std::string& lang) {
  Send(new MediaPlayerEflHostMsg_NotifySubtitlePlay(routing_id(), player_id, id,
                                                    url, lang));
}

void RendererMediaPlayerManager::OnGetPlayTrackInfo(int player_id, int id) {
  auto player = GetMediaPlayer(player_id);
  if (player)
    player->GetPlayTrackInfo(id);
}

void RendererMediaPlayerManager::OnDrmError(int player_id) {
  auto player = GetMediaPlayer(player_id);
  if (player)
    player->OnDrmError();
}

void RendererMediaPlayerManager::OnSeekableTimeChange(
    int player_id,
    base::TimeDelta min_time,
    base::TimeDelta max_time) {
  auto player = GetMediaPlayer(player_id);
  if (player)
    player->OnSeekableTimeChange(min_time, max_time);
}

void RendererMediaPlayerManager::OnSyncTimeline(
    int player_id,
    const std::string& timeline_selector,
    int sync) {
  auto player = GetMediaPlayer(player_id);
  if (player)
    player->OnSyncTimeline(timeline_selector, sync);
}

void RendererMediaPlayerManager::OnRegisterTimeline(
    int player_id,
    const blink::WebMediaPlayer::register_timeline_cb_info_s& info) {
  auto player = GetMediaPlayer(player_id);
  if (player)
    player->OnRegisterTimeline(info);
}

void RendererMediaPlayerManager::OnMrsUrlChange(int player_id,
                                                const std::string& url) {
  auto player = GetMediaPlayer(player_id);
  if (player)
    player->OnMrsUrlChange(url);
}

void RendererMediaPlayerManager::OnContentIdChange(int player_id,
                                                   const std::string& id) {
  auto player = GetMediaPlayer(player_id);
  if (player)
    player->OnContentIdChange(id);
}

bool RendererMediaPlayerManager::RegisterTimeline(
    int player_id,
    const std::string& timeline_selector) {
  bool ret = false;
  Send(new MediaPlayerEflHostMsg_RegisterTimeline(routing_id(), player_id,
                                                  timeline_selector, &ret));

  return ret;
}

bool RendererMediaPlayerManager::UnRegisterTimeline(
    int player_id,
    const std::string& timeline_selector) {
  bool ret = false;
  Send(new MediaPlayerEflHostMsg_UnRegisterTimeline(routing_id(), player_id,
                                                    timeline_selector, &ret));
  return ret;
}

void RendererMediaPlayerManager::GetTimelinePositions(
    int player_id,
    const std::string& timeline_selector,
    uint32_t* units_per_tick,
    uint32_t* units_per_second,
    int64_t* content_time,
    bool* paused) {
  Send(new MediaPlayerEflHostMsg_GetTimelinePositions(
      routing_id(), player_id, timeline_selector, units_per_tick,
      units_per_second, content_time, paused));
}

double RendererMediaPlayerManager::GetSpeed(int player_id) {
  double speed = 0;
  Send(new MediaPlayerEflHostMsg_GetSpeed(routing_id(), player_id, &speed));
  return speed;
}

std::string RendererMediaPlayerManager::GetMrsUrl(int player_id) {
  std::string mrsUrl("");
  Send(new MediaPlayerEflHostMsg_GetMrsUrl(routing_id(), player_id, &mrsUrl));
  return mrsUrl;
}

#if defined(OS_TIZEN_TV_PRODUCT)
void RendererMediaPlayerManager::DestroyPlayerSync(int player_id) {
  LOG(INFO) << "Synchronous player destruction start.";
  Send(new MediaPlayerEflHostMsg_DeInitSync(routing_id(), player_id));
  LOG(INFO) << "Synchronous player destruction end.";
}
#endif

std::string RendererMediaPlayerManager::GetContentId(int player_id) {
  std::string contentId("");
  Send(new MediaPlayerEflHostMsg_GetContentId(routing_id(), player_id,
                                              &contentId));
  return contentId;
}

bool RendererMediaPlayerManager::SyncTimeline(
    int player_id,
    const std::string& timeline_selector,
    int64_t timeline_pos,
    int64_t wallclock_pos,
    int tolerance) {
  bool ret = false;
  Send(new MediaPlayerEflHostMsg_SyncTimeline(routing_id(), player_id,
                                              timeline_selector, timeline_pos,
                                              wallclock_pos, tolerance, &ret));
  return ret;
}

bool RendererMediaPlayerManager::SetWallClock(
    int player_id,
    const std::string& wallclock_url) {
  bool ret = false;
  Send(new MediaPlayerEflHostMsg_SetWallClock(routing_id(), player_id,
                                              wallclock_url, &ret));
  return ret;
}

void RendererMediaPlayerManager::OnAddAudioTrack(
    int player_id,
    const blink::WebMediaPlayer::audio_video_track_info_s& info) {
  auto player = GetMediaPlayer(player_id);
  if (player)
    player->OnAddAudioTrack(info);
}

void RendererMediaPlayerManager::OnAddVideoTrack(
    int player_id,
    const blink::WebMediaPlayer::audio_video_track_info_s& info) {
  auto player = GetMediaPlayer(player_id);
  if (player)
    player->OnAddVideoTrack(info);
}

void RendererMediaPlayerManager::OnAddTextTrack(int player_id,
                                                const std::string& label,
                                                const std::string& language,
                                                const std::string& id) {
  auto player = GetMediaPlayer(player_id);
  if (player)
    player->OnAddTextTrack(label, language, id);
}

void RendererMediaPlayerManager::SetActiveTextTrack(int player_id,
                                                    int id,
                                                    bool is_in_band) {
  Send(new MediaPlayerEflHostMsg_SetActiveTextTrack(routing_id(), player_id, id,
                                                    is_in_band));
}

void RendererMediaPlayerManager::SetActiveAudioTrack(int player_id, int index) {
  Send(new MediaPlayerEflHostMsg_SetActiveAudioTrack(routing_id(), player_id,
                                                     index));
}

void RendererMediaPlayerManager::SetActiveVideoTrack(int player_id, int index) {
  Send(new MediaPlayerEflHostMsg_SetActiveVideoTrack(routing_id(), player_id,
                                                     index));
}

void RendererMediaPlayerManager::OnInbandEventTextTrack(int player_id,
                                                        const std::string& info,
                                                        int action) {
  auto player = GetMediaPlayer(player_id);
  if (player)
    player->OnInbandEventTextTrack(info, action);
}

void RendererMediaPlayerManager::OnInbandEventTextCue(int player_id,
                                                      const std::string& info,
                                                      unsigned int id,
                                                      long long int start_time,
                                                      long long int end_time) {
  auto player = GetMediaPlayer(player_id);
  if (player)
    player->OnInbandEventTextCue(info, id, start_time, end_time);
}

unsigned RendererMediaPlayerManager::GetDecodedFrameCount(int player_id) {
  unsigned decoded_frame_count = 0;
  Send(new MediaPlayerEflHostMsg_GetDecodedFrameCount(routing_id(), player_id,
                                                      &decoded_frame_count));
  return decoded_frame_count;
}

unsigned RendererMediaPlayerManager::GetDroppedFrameCount(int player_id) {
  unsigned dropped_frame_count = 0;
  Send(new MediaPlayerEflHostMsg_GetDroppedFrameCount(routing_id(), player_id,
                                                      &dropped_frame_count));
  return dropped_frame_count;
}

void RendererMediaPlayerManager::NotifyElementRemove(int player_id) {
  Send(new MediaPlayerEflHostMsg_NotifyElementRemove(routing_id(), player_id));
}

void RendererMediaPlayerManager::SetParentalRatingResult(int player_id,
                                                         bool is_pass) {
  Send(new MediaPlayerEflHostMsg_SetParentalRatingResult(routing_id(),
                                                         player_id, is_pass));
}

void RendererMediaPlayerManager::OnPlayerStarted(int player_id,
                                                 bool player_started) {
  auto player = GetMediaPlayer(player_id);
  if (player)
    player->OnPlayerStarted(player_started);
}

void RendererMediaPlayerManager::OnVideoSlotsAvailableChange(
    unsigned slots_available) {
  LOG(INFO) << "[VSAC] got event for all players: " << slots_available;
  for (auto const& pair : media_players_) {
    auto player = pair.second;
    player->OnVideoSlotsAvailableChange(slots_available);
  }
}

// synchronous ipc message is sent by checkHLSSupport
void RendererMediaPlayerManager::CheckHLSSupport(const blink::WebString& url,
                                                 bool& support) {
  Send(new FrameHostMsg_CheckHLSSupport(routing_id(), url.Utf8(), &support));
  LOG(INFO) << "[HLS] url: " << url.Utf8() << ", support = " << support;
}
#endif

}  // namespace content
