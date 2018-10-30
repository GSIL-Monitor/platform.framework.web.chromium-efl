// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/tizen/placeholder_media_player.h"

namespace media {

void PlaceholderMediaPlayer::Play() {}

void PlaceholderMediaPlayer::Pause(bool) {}

void PlaceholderMediaPlayer::SetRate(double) {}

void PlaceholderMediaPlayer::Seek(base::TimeDelta) {}

void PlaceholderMediaPlayer::SetVolume(double) {}

base::TimeDelta PlaceholderMediaPlayer::GetCurrentTime() {
  return {};
}

int PlaceholderMediaPlayer::GetPlayerId() const {
  return player_id_;
}

void PlaceholderMediaPlayer::Initialize() {}

void PlaceholderMediaPlayer::EnteredFullscreen() {}

void PlaceholderMediaPlayer::ExitedFullscreen() {}

#if defined(TIZEN_SOUND_FOCUS)
void PlaceholderMediaPlayer::SetResumePlayingBySFM(bool) {}

void PlaceholderMediaPlayer::OnFocusAcquired() {}

void PlaceholderMediaPlayer::OnFocusReleased(bool resume_playing) {}
#endif

MediaTypeFlags PlaceholderMediaPlayer::GetMediaType() const {
  return MediaType::Neither;
}

bool PlaceholderMediaPlayer::IsPaused() const {
  return false;
}

bool PlaceholderMediaPlayer::IsPlayerSuspended() const {
  return true;
}

bool PlaceholderMediaPlayer::IsPlayerFullyResumed() const {
  return false;
}

bool PlaceholderMediaPlayer::IsPlayerResuming() const {
  return false;
}

bool PlaceholderMediaPlayer::ShouldSeekAfterResume() const {
  return false;
}

bool PlaceholderMediaPlayer::HasConfigs() const {
  return false;
}

void PlaceholderMediaPlayer::Suspend() {}

void PlaceholderMediaPlayer::Resume() {}

double PlaceholderMediaPlayer::GetStartDate() {
  return {};
}

#if defined(TIZEN_VIDEO_HOLE)
void PlaceholderMediaPlayer::SetGeometry(const gfx::RectF&) {}

void PlaceholderMediaPlayer::OnWebViewMoved() {}
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
void PlaceholderMediaPlayer::NotifySubtitlePlay(int,
                                                const std::string&,
                                                const std::string&) {}

bool PlaceholderMediaPlayer::RegisterTimeline(const std::string&) {
  return true;
}

bool PlaceholderMediaPlayer::UnRegisterTimeline(const std::string&) {
  return true;
}

void PlaceholderMediaPlayer::GetTimelinePositions(const std::string&,
                                                  uint32_t*,
                                                  uint32_t*,
                                                  int64_t*,
                                                  bool*) {}

double PlaceholderMediaPlayer::GetSpeed() {
  return {};
}

std::string PlaceholderMediaPlayer::GetMrsUrl() {
  return {};
}

std::string PlaceholderMediaPlayer::GetContentId() {
  return {};
}

bool PlaceholderMediaPlayer::SyncTimeline(const std::string&,
                                          int64_t,
                                          int64_t,
                                          int) {
  return true;
}

bool PlaceholderMediaPlayer::SetWallClock(const std::string&) {
  return true;
}

void PlaceholderMediaPlayer::SetActiveTextTrack(int, bool) {}

void PlaceholderMediaPlayer::SetActiveAudioTrack(int) {}

void PlaceholderMediaPlayer::SetActiveVideoTrack(int) {}

unsigned PlaceholderMediaPlayer::GetDroppedFrameCount() {
  return 0;
}

unsigned PlaceholderMediaPlayer::GetDecodedFrameCount() const {
  return 0;
}

void PlaceholderMediaPlayer::ElementRemove() {}
void PlaceholderMediaPlayer::SetParentalRatingResult(bool is_pass) {}
void PlaceholderMediaPlayer::SetDecryptor(eme::eme_decryptor_t) {}
void PlaceholderMediaPlayer::SetHasEncryptedListenerOrCdm(bool) {}
bool PlaceholderMediaPlayer::IsResourceConflict() const {
  return false;
}
void PlaceholderMediaPlayer::ResetResourceConflict() {}
#endif

PlayerRoleFlags PlaceholderMediaPlayer::GetRoles() const noexcept {
  return PlayerRole::RemotePeerStream;
}

}  // namespace media
