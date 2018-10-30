// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/tizen/suspending_media_player.h"

namespace media {

void SuspendingMediaSourcePlayer::OnDemuxerConfigsAvailable(
    const DemuxerConfigs& params) {
  if (parent_->IsPlayerFullyResumed() || parent_->IsPlayerResuming())
    demuxer_client_->OnDemuxerConfigsAvailable(params);
  else
    parent_->AddFunctionToQueue(
        [this, params]() { this->OnDemuxerConfigsAvailable(params); });
}

void SuspendingMediaSourcePlayer::OnDemuxerDataAvailable(
    base::SharedMemoryHandle foreign_memory_handle,
    const DemuxedBufferMetaData& meta_data) {
  if (parent_->IsPlayerFullyResumed() || parent_->IsPlayerResuming())
    demuxer_client_->OnDemuxerDataAvailable(foreign_memory_handle, meta_data);
}

void SuspendingMediaSourcePlayer::OnDemuxerSeekDone(
    const base::TimeDelta& actual_browser_seek_time,
    const base::TimeDelta& video_key_frame) {
  parent_->stored_position_ = actual_browser_seek_time;
  if (parent_->IsPlayerFullyResumed() || parent_->IsPlayerResuming()) {
    parent_->seek_state_ = SuspendingMediaPlayer::seek_state::SEEK_ON_PLAYER;
    demuxer_client_->OnDemuxerSeekDone(actual_browser_seek_time,
                                       video_key_frame);
  }
}

void SuspendingMediaSourcePlayer::OnDemuxerDurationChanged(
    base::TimeDelta duration) {
  if (parent_->IsPlayerFullyResumed() || parent_->IsPlayerResuming())
    demuxer_client_->OnDemuxerDurationChanged(duration);
}
#if defined(OS_TIZEN_TV_PRODUCT)
void SuspendingMediaSourcePlayer::OnDemuxerStateChanged(
    const DemuxerConfigs& params) {
  if (parent_->IsPlayerFullyResumed() || parent_->IsPlayerResuming())
    demuxer_client_->OnDemuxerStateChanged(params);
}
#endif

void SuspendingMediaSourcePlayer::OnDemuxerBufferedChanged(
    const media::Ranges<base::TimeDelta>& ranges) {
  if (parent_->IsPlayerFullyResumed() || parent_->IsPlayerResuming())
    demuxer_client_->OnDemuxerBufferedChanged(ranges);
}

void SuspendingMediaSourcePlayer::Initialize(DemuxerEflClient* client) {
  demuxer_client_ = client;
  demuxer_->Initialize(this);
}

void SuspendingMediaSourcePlayer::RequestDemuxerSeek(
    const base::TimeDelta& time_to_seek) {
  if (parent_->IsPlayerFullyResumed() || parent_->IsPlayerResuming())
    demuxer_->RequestDemuxerSeek(time_to_seek);
}

void SuspendingMediaSourcePlayer::RequestDemuxerData(DemuxerStream::Type type) {
  if (parent_->IsPlayerFullyResumed() || parent_->IsPlayerResuming())
    demuxer_->RequestDemuxerData(type);
}

void SuspendingMediaPlayer::Play() {
  if (IsPlayerFullyResumed())
    player_->Play();
  else {
    is_paused_ = false;
    action_after_resume_ = action_after_resume::PLAY;
  }
}

void SuspendingMediaPlayer::Pause(bool is_media_related_action) {
  if (IsPlayerFullyResumed())
    player_->Pause(is_media_related_action);
  else {
    is_paused_ = !is_media_related_action;
    action_after_resume_ = is_media_related_action ? action_after_resume::PLAY
                                                   : action_after_resume::PAUSE;
  }
}

void SuspendingMediaPlayer::SetRate(double rate) {
  if (IsPlayerFullyResumed())
    player_->SetRate(rate);
  else
    AddFunctionToQueue([this, rate]() { this->SetRate(rate); });
}

void SuspendingMediaPlayer::Seek(base::TimeDelta time) {
  stored_position_ = time;
  if (IsPlayerSuspended()) {
    return;
  }
  seek_state_ = seek_state::SEEK_ON_DEMUXER;
  player_->Seek(time);
#if defined(OS_TIZEN_TV_PRODUCT)
  if (IsPlayerResuming() && is_seek_after_resume_)
    is_seek_after_resume_ = false;
#endif
}

void SuspendingMediaPlayer::SetVolume(double volume) {
  if (IsPlayerFullyResumed())
    player_->SetVolume(volume);
  else
    AddFunctionToQueue([this, volume]() { this->SetVolume(volume); });
}

base::TimeDelta SuspendingMediaPlayer::GetCurrentTime() {
  return player_->GetCurrentTime();
}

int SuspendingMediaPlayer::GetPlayerId() const {
  return player_->GetPlayerId();
}

void SuspendingMediaPlayer::Initialize() {
  player_->Initialize();
}

void SuspendingMediaPlayer::EnteredFullscreen() {
  player_->EnteredFullscreen();
}

void SuspendingMediaPlayer::ExitedFullscreen() {
  player_->ExitedFullscreen();
}

#if defined(TIZEN_SOUND_FOCUS)
void SuspendingMediaPlayer::SetResumePlayingBySFM(bool resume_playing) {
  player_->SetResumePlayingBySFM(resume_playing);
}

void SuspendingMediaPlayer::OnFocusAcquired() {
  player_->OnFocusAcquired();
}

void SuspendingMediaPlayer::OnFocusReleased(bool resume_playing) {
  player_->OnFocusReleased(resume_playing);
}
#endif

MediaTypeFlags SuspendingMediaPlayer::GetMediaType() const {
  return player_->GetMediaType();
}

bool SuspendingMediaPlayer::IsPlayerSuspended() const {
  return state_ == player_state::SUSPENDED ||
         state_ == player_state::SUSPENDING;
}

bool SuspendingMediaPlayer::IsPlayerFullyResumed() const {
  return state_ == player_state::RESUMED;
}

bool SuspendingMediaPlayer::IsPlayerResuming() const {
  return state_ == player_state::RESUMING;
}

void SuspendingMediaPlayer::Suspend() {
  if (IsPlayerSuspended()) {
    LOG(WARNING) << "player already suspended";
    return;
  }
  if (IsPlayerFullyResumed()) {
#if defined(OS_TIZEN_TV_PRODUCT)
    action_after_resume_ = player_->IsPaused() ? action_after_resume::PAUSE
                                               : action_after_resume::PLAY;
#endif
    if (seek_state_ == seek_state::SEEK_NONE && ShouldSeekAfterResume()) {
      stored_position_ = player_->GetCurrentTime();
    }
  }
  state_ = player_state::SUSPENDING;
  player_->Suspend();
}

void SuspendingMediaPlayer::Resume() {
  if (IsPlayerFullyResumed() || IsPlayerResuming()) {
    LOG(WARNING) << "player already resumed";
    return;
  }
  state_ = player_state::RESUMING;
  const bool should_seek_after_resume = ShouldSeekAfterResume();
  player_->Resume();
  if ((should_seek_after_resume || !stored_position_.is_zero()) &&
      HasConfigs()) {
    if (seek_state_ == seek_state::SEEK_NONE)
      is_seek_after_resume_ = true;
#if defined(OS_TIZEN_TV_PRODUCT)
    seek_state_ = seek_state::SEEK_ON_DEMUXER;
    player_->Seek(stored_position_);
#else
    Seek(stored_position_);
#endif
  }
}

double SuspendingMediaPlayer::GetStartDate() {
  return player_->GetStartDate();
}

#if defined(TIZEN_VIDEO_HOLE)
void SuspendingMediaPlayer::SetGeometry(const gfx::RectF& geometry) {
  player_->SetGeometry(geometry);
}

void SuspendingMediaPlayer::OnWebViewMoved() {
  player_->OnWebViewMoved();
}
#endif

MediaPlayerInterfaceEfl* SuspendingMediaPlayer::GetPlayer(int player_id) {
  return manager_->GetPlayer(player_id);
}

void SuspendingMediaPlayer::OnTimeChanged(int player_id) {
  if (IsPlayerFullyResumed())
    manager_->OnTimeChanged(player_id);
}

void SuspendingMediaPlayer::OnTimeUpdate(int player_id,
                                         base::TimeDelta current_time) {
  if (IsPlayerFullyResumed())
    manager_->OnTimeUpdate(player_id, current_time);
}

void SuspendingMediaPlayer::OnRequestSeek(int player_id,
                                          base::TimeDelta seek_time) {
  if (IsPlayerFullyResumed() || IsPlayerResuming())
    manager_->OnRequestSeek(player_id, seek_time);
}

void SuspendingMediaPlayer::OnPauseStateChange(int player_id, bool state) {
  if (IsPlayerFullyResumed() || IsPlayerResuming())
    manager_->OnPauseStateChange(player_id, state);
}

void SuspendingMediaPlayer::OnSeekComplete(int player_id) {
  if (IsPlayerFullyResumed() || IsPlayerResuming()) {
    if (!is_seek_after_resume_)
      manager_->OnSeekComplete(player_id);
    if (is_seek_after_resume_) {
      is_seek_after_resume_ = false;
    }
    seek_state_ = seek_state::SEEK_NONE;
  }
}

void SuspendingMediaPlayer::OnBufferUpdate(int player_id,
                                           int buffering_percentage) {
  if (IsPlayerFullyResumed() || IsPlayerResuming())
    manager_->OnBufferUpdate(player_id, buffering_percentage);
}

void SuspendingMediaPlayer::OnDurationChange(int player_id,
                                             base::TimeDelta duration) {
  if (IsPlayerFullyResumed() || IsPlayerResuming())
    manager_->OnDurationChange(player_id, duration);
}

void SuspendingMediaPlayer::OnReadyStateChange(
    int player_id,
    blink::WebMediaPlayer::ReadyState state) {
  if (IsPlayerFullyResumed() || IsPlayerResuming())
    manager_->OnReadyStateChange(player_id, state);
}

void SuspendingMediaPlayer::OnNetworkStateChange(
    int player_id,
    blink::WebMediaPlayer::NetworkState state) {
  if (IsPlayerFullyResumed() || IsPlayerResuming())
    manager_->OnNetworkStateChange(player_id, state);
}

void SuspendingMediaPlayer::OnMediaDataChange(int player_id,
                                              int width,
                                              int height,
                                              int media) {
  if (IsPlayerFullyResumed() || IsPlayerResuming())
    manager_->OnMediaDataChange(player_id, width, height, media);
}

void SuspendingMediaPlayer::OnNewFrameAvailable(int player_id,
                                                base::SharedMemoryHandle handle,
                                                uint32_t length,
                                                base::TimeDelta timestamp) {
  if (IsPlayerFullyResumed())
    manager_->OnNewFrameAvailable(player_id, handle, length, timestamp);
}

void SuspendingMediaPlayer::OnPlayerDestroyed(int player_id) {
  manager_->OnPlayerDestroyed(player_id);
}

void SuspendingMediaPlayer::OnInitComplete(int player_id, bool success) {
  if (IsPlayerFullyResumed() || IsPlayerResuming())
    manager_->OnInitComplete(player_id, success);
}

void SuspendingMediaPlayer::OnResumeComplete(int player_id) {
  state_ = player_state::RESUMED;
  manager_->OnResumeComplete(player_id);
  if (action_after_resume_ == action_after_resume::PLAY) {
    OnPauseStateChange(player_id, false);
    Play();
  } else if (action_after_resume_ == action_after_resume::PAUSE) {
    Pause(false);
  }
  action_after_resume_ = action_after_resume::NONE;
  if (!functions_to_be_called_after_resume_.empty()) {
    ExecuteQueuedFunctionsAfterResume();
  }
}

void SuspendingMediaPlayer::OnSuspendComplete(int player_id) {
  state_ = player_state::SUSPENDED;
  manager_->OnSuspendComplete(player_id);
}

#if defined(TIZEN_VIDEO_HOLE)
gfx::Rect SuspendingMediaPlayer::GetViewportRect(int player_id) const {
  return manager_->GetViewportRect(player_id);
}
#endif

#if defined(TIZEN_TBM_SUPPORT)
void SuspendingMediaPlayer::OnNewTbmBufferAvailable(
    int player_id,
    gfx::TbmBufferHandle tbm_handle,
    base::TimeDelta timestamp) {
  if (IsPlayerFullyResumed())
    manager_->OnNewTbmBufferAvailable(player_id, tbm_handle, timestamp);
}
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
void SuspendingMediaPlayer::GetPlayTrackInfo(int player_id, int id) {
  manager_->GetPlayTrackInfo(player_id, id);
}

void SuspendingMediaPlayer::NotifySubtitlePlay(int id,
                                               const std::string& url,
                                               const std::string& lang) {
  player_->NotifySubtitlePlay(id, url, lang);
}

void SuspendingMediaPlayer::OnSeekableTimeChange(int player_id,
                                                 base::TimeDelta min_time,
                                                 base::TimeDelta max_time) {
  if (IsPlayerFullyResumed())
    manager_->OnSeekableTimeChange(player_id, min_time, max_time);
  else
    AddFunctionToQueue([this, player_id, min_time, max_time]() {
      this->OnSeekableTimeChange(player_id, min_time, max_time);
    });
}

void SuspendingMediaPlayer::OnPlayerResourceConflict(int player_id) {
  manager_->OnPlayerResourceConflict(player_id);
}

void SuspendingMediaPlayer::OnDrmError(int player_id) {
  manager_->OnDrmError(player_id);
}

bool SuspendingMediaPlayer::RegisterTimeline(
    const std::string& timeline_selector) {
  return player_->RegisterTimeline(timeline_selector);
}

bool SuspendingMediaPlayer::UnRegisterTimeline(
    const std::string& timeline_selector) {
  return player_->UnRegisterTimeline(timeline_selector);
}

void SuspendingMediaPlayer::GetTimelinePositions(
    const std::string& timeline_selector,
    uint32_t* units_per_tick,
    uint32_t* units_per_second,
    int64_t* content_time,
    bool* paused) {
  player_->GetTimelinePositions(timeline_selector, units_per_tick,
                                units_per_second, content_time, paused);
}

double SuspendingMediaPlayer::GetSpeed() {
  return player_->GetSpeed();
}

std::string SuspendingMediaPlayer::GetMrsUrl() {
  return player_->GetMrsUrl();
}

std::string SuspendingMediaPlayer::GetContentId() {
  return player_->GetContentId();
}

bool SuspendingMediaPlayer::SyncTimeline(const std::string& timeline_selector,
                                         int64_t timeline_pos,
                                         int64_t wallclock_pos,
                                         int tolerance) {
  return player_->SyncTimeline(timeline_selector, timeline_pos, wallclock_pos,
                               tolerance);
}

bool SuspendingMediaPlayer::SetWallClock(const std::string& wallclock_url) {
  return player_->SetWallClock(wallclock_url);
}

void SuspendingMediaPlayer::OnRegisterTimelineCbInfo(
    int player_id,
    const blink::WebMediaPlayer::register_timeline_cb_info_s& info) {
  manager_->OnRegisterTimelineCbInfo(player_id, info);
}

void SuspendingMediaPlayer::OnSyncTimelineCbInfo(
    int player_id,
    const std::string& timeline_selector,
    int sync) {
  manager_->OnSyncTimelineCbInfo(player_id, timeline_selector, sync);
}

void SuspendingMediaPlayer::OnMrsUrlChange(int player_id,
                                           const std::string& url) {
  manager_->OnMrsUrlChange(player_id, url);
}

void SuspendingMediaPlayer::OnContentIdChange(int player_id,
                                              const std::string& contentId) {
  manager_->OnContentIdChange(player_id, contentId);
}

void SuspendingMediaPlayer::OnAddAudioTrack(
    int player_id,
    const blink::WebMediaPlayer::audio_video_track_info_s& info) {
  if (IsPlayerFullyResumed())
    manager_->OnAddAudioTrack(player_id, info);
}

void SuspendingMediaPlayer::OnAddVideoTrack(
    int player_id,
    const blink::WebMediaPlayer::audio_video_track_info_s& info) {
  if (IsPlayerFullyResumed())
    manager_->OnAddVideoTrack(player_id, info);
}

void SuspendingMediaPlayer::OnAddTextTrack(int player_id,
                                           const std::string& label,
                                           const std::string& language,
                                           const std::string& id) {
  if (IsPlayerFullyResumed())
    manager_->OnAddTextTrack(player_id, label, language, id);
}

void SuspendingMediaPlayer::SetActiveTextTrack(int id, bool is_in_band) {
  player_->SetActiveTextTrack(id, is_in_band);
}

void SuspendingMediaPlayer::SetActiveAudioTrack(int index) {
  player_->SetActiveAudioTrack(index);
}

void SuspendingMediaPlayer::SetActiveVideoTrack(int index) {
  player_->SetActiveVideoTrack(index);
}

void SuspendingMediaPlayer::OnInbandEventTextTrack(int player_id,
                                                   const std::string& info,
                                                   int action) {
  manager_->OnInbandEventTextTrack(player_id, info, action);
}

void SuspendingMediaPlayer::OnInbandEventTextCue(int player_id,
                                                 const std::string& info,
                                                 unsigned int id,
                                                 long long int start_time,
                                                 long long int end_time) {
  manager_->OnInbandEventTextCue(player_id, info, id, start_time, end_time);
}

unsigned SuspendingMediaPlayer::GetDroppedFrameCount() {
  return player_->GetDroppedFrameCount();
}

unsigned SuspendingMediaPlayer::GetDecodedFrameCount() const {
  return player_->GetDecodedFrameCount();
}

void SuspendingMediaPlayer::ElementRemove() {
  return player_->ElementRemove();
}

void SuspendingMediaPlayer::SetParentalRatingResult(bool is_pass) {
  return player_->SetParentalRatingResult(is_pass);
}

void SuspendingMediaPlayer::OnPlayerStarted(int player_id,
                                            bool player_started) {
  manager_->OnPlayerStarted(player_id, player_started);
}

void SuspendingMediaPlayer::ResetResourceConflict() {
  player_->ResetResourceConflict();
}

bool SuspendingMediaPlayer::IsResourceConflict() const {
  return player_->IsResourceConflict();
}
#endif

content::WebContents* SuspendingMediaPlayer::GetWebContents(int player_id) {
  return manager_->GetWebContents(player_id);
}

void SuspendingMediaPlayer::OnPlayerSuspendRequest(int player_id) {
  manager_->OnPlayerSuspendRequest(player_id);
}

void SuspendingMediaPlayer::AddFunctionToQueue(std::function<void()> function) {
  functions_to_be_called_after_resume_.push(std::move(function));
}

void SuspendingMediaPlayer::ExecuteFunction(
    const std::function<void()>& function) {
  function();
}

void SuspendingMediaPlayer::ExecuteQueuedFunctionsAfterResume() {
  while (!functions_to_be_called_after_resume_.empty()) {
    task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&SuspendingMediaPlayer::ExecuteFunction,
                   weak_factory_.GetWeakPtr(),
                   std::move(functions_to_be_called_after_resume_.front())));
    functions_to_be_called_after_resume_.pop();
  }
}

PlayerRoleFlags SuspendingMediaPlayer::GetRoles() const noexcept {
  return player_->GetRoles();
}

}  // namespace media
