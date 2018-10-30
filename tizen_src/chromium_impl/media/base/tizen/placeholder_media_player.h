// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_TIZEN_PLACEHOLDER_MEDIA_PLAYER_H_
#define MEDIA_BASE_TIZEN_PLACEHOLDER_MEDIA_PLAYER_H_

#include <queue>
#include <utility>

#include "base/memory/ptr_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "media/base/timestamp_constants.h"
#include "media/base/tizen/demuxer_efl.h"
#include "media/base/tizen/media_player_efl.h"
#include "media/base/tizen/media_player_interface_efl.h"
#include "media/base/tizen/media_player_manager_efl.h"

namespace media {

class PlaceholderMediaPlayer : public MediaPlayerInterfaceEfl {
 public:
  static MediaPlayerInterfaceEfl* CreatePlayer(int player_id,
                                               MediaPlayerManager* manager) {
    return new PlaceholderMediaPlayer(player_id, manager);
  }

  // MediaPlayerEfl interface
  int GetPlayerId() const override;
  void Initialize() override;
  void Play() override;
  void Pause(bool is_media_related_action) override;
  void SetRate(double rate) override;
  void Seek(base::TimeDelta time) override;
  void SetVolume(double volume) override;
  base::TimeDelta GetCurrentTime() override;
  void EnteredFullscreen() override;
  void ExitedFullscreen() override;

#if defined(TIZEN_SOUND_FOCUS)
  // SoundFocusClient interface
  void SetResumePlayingBySFM(bool resume_playing) override;
  void OnFocusAcquired() override;
  void OnFocusReleased(bool resume_playing) override;
#endif

  // Interface for resource handling
  MediaTypeFlags GetMediaType() const override;
  bool IsPlayerSuspended() const override;
  bool IsPlayerFullyResumed() const;
  bool IsPlayerResuming() const;
  void Suspend() override;
  void Resume() override;

  double GetStartDate() override;

#if defined(TIZEN_VIDEO_HOLE)
  void SetGeometry(const gfx::RectF&) override;
  void OnWebViewMoved() override;
#endif

  void OnPlayerDestroyed(int player_id);

#if defined(OS_TIZEN_TV_PRODUCT)
  void NotifySubtitlePlay(int id,
                          const std::string& url,
                          const std::string& lang) override;
  bool RegisterTimeline(const std::string& timeline_selector) override;
  bool UnRegisterTimeline(const std::string& timeline_selector) override;
  void GetTimelinePositions(const std::string& timeline_selector,
                            uint32_t* units_per_tick,
                            uint32_t* units_per_second,
                            int64_t* content_time,
                            bool* paused) override;
  double GetSpeed() override;
  std::string GetMrsUrl() override;
  std::string GetContentId() override;
  bool SyncTimeline(const std::string& timeline_selector,
                    int64_t timeline_pos,
                    int64_t wallclock_pos,
                    int tolerance) override;
  bool SetWallClock(const std::string& wallclock_url) override;
  void SetActiveTextTrack(int id, bool is_in_band) override;
  void SetActiveAudioTrack(int index) override;
  void SetActiveVideoTrack(int index) override;
  unsigned GetDroppedFrameCount() override;
  unsigned GetDecodedFrameCount() const override;
  void ElementRemove() override;
  void SetParentalRatingResult(bool is_pass) override;
  void SetDecryptor(eme::eme_decryptor_t) override;
  void SetHasEncryptedListenerOrCdm(bool) override;
  bool IsResourceConflict() const override;
  void ResetResourceConflict() override;
#endif

  bool IsPaused() const override;
  bool ShouldSeekAfterResume() const override;
  bool HasConfigs() const override;

  PlayerRoleFlags GetRoles() const noexcept override;

 private:
  PlaceholderMediaPlayer(int player_id, MediaPlayerManager* manager)
      : player_id_{player_id}, manager_{manager} {}

  int player_id_;
  MediaPlayerManager* manager_;
  bool is_paused_{false};
  DISALLOW_COPY_AND_ASSIGN(PlaceholderMediaPlayer);
};

}  // namespace media

#endif  // MEDIA_BASE_TIZEN_PLACEHOLDER_MEDIA_PLAYER_H_
