// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_TIZEN_SUSPENDING_MEDIA_PLAYER_H_
#define MEDIA_BASE_TIZEN_SUSPENDING_MEDIA_PLAYER_H_

#include <queue>
#include <utility>

#include "base/memory/ptr_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "media/base/timestamp_constants.h"
#include "media/base/tizen/demuxer_efl.h"
#include "media/base/tizen/media_player_efl.h"
#include "media/base/tizen/media_player_interface_efl.h"
#include "media/base/tizen/media_player_manager_efl.h"

#if defined(OS_TIZEN_TV_PRODUCT)
#include <emeCDM/IEME.h>
#endif

namespace media {

class SuspendingMediaPlayer;

class SuspendingMediaSourcePlayer : public DemuxerEflClient, public DemuxerEfl {
 public:
  SuspendingMediaSourcePlayer(SuspendingMediaPlayer* parent,
                              std::unique_ptr<DemuxerEfl> demuxer)
      : parent_(parent),
        demuxer_client_(nullptr),
        demuxer_(std::move(demuxer)) {}

  // DemuxerEflClient interface
  void OnDemuxerConfigsAvailable(const DemuxerConfigs& params) override;
  void OnDemuxerDataAvailable(base::SharedMemoryHandle foreign_memory_handle,
                              const DemuxedBufferMetaData& meta_data) override;
  void OnDemuxerSeekDone(const base::TimeDelta& actual_browser_seek_time,
                         const base::TimeDelta& video_key_frame) override;
  void OnDemuxerDurationChanged(base::TimeDelta duration) override;
#if defined(OS_TIZEN_TV_PRODUCT)
  void OnDemuxerStateChanged(const DemuxerConfigs& params) override;
#endif
  void OnDemuxerBufferedChanged(
      const media::Ranges<base::TimeDelta>& ranges) override;

  // DemuxerEfl interface
  void Initialize(DemuxerEflClient* client) override;
  void RequestDemuxerSeek(const base::TimeDelta& time_to_seek) override;
  void RequestDemuxerData(DemuxerStream::Type type) override;

 private:
  SuspendingMediaPlayer* const parent_;
  DemuxerEflClient* demuxer_client_;
  const std::unique_ptr<DemuxerEfl> demuxer_;
};

class SuspendingMediaPlayer : public MediaPlayerInterfaceEfl,
                              public MediaPlayerManager {
  struct constructor_tag {};

 public:
  static MediaPlayerInterfaceEfl* CreatePlayer(int player_id,
                                               const GURL& url,
                                               const std::string& mime_type,
                                               MediaPlayerManager* manager,
                                               const std::string& user_agent,
                                               int audio_latency_mode,
                                               bool video_hole) {
    auto suspending_player =
        base::MakeUnique<SuspendingMediaPlayer>(constructor_tag());
    suspending_player->InjectMediaPlayer(MediaPlayerEfl::CreatePlayer(
        player_id, url, mime_type, suspending_player->InjectManager(manager),
        user_agent, audio_latency_mode, video_hole));
    return suspending_player.release();
  }

#if defined(OS_TIZEN_TV_PRODUCT)
  void SetDecryptor(eme::eme_decryptor_t decryptor) override {
    player_->SetDecryptor(decryptor);
  }
  void OnInitData(int player_id,
                  const std::vector<unsigned char>& init_data) override {
    manager_->OnInitData(player_id, init_data);
  }
  void OnWaitingForKey(int player_id) override {
    manager_->OnWaitingForKey(player_id);
  }
  void SetHasEncryptedListenerOrCdm(bool ret) override {
    player_->SetHasEncryptedListenerOrCdm(ret);
  }
#endif

  static MediaPlayerInterfaceEfl* CreatePlayer(
      int player_id,
      std::unique_ptr<DemuxerEfl> demuxer,
      MediaPlayerManager* manager,
      bool video_hole) {
    auto suspending_player =
        base::MakeUnique<SuspendingMediaPlayer>(constructor_tag());
    suspending_player->InjectMediaPlayer(MediaPlayerEfl::CreatePlayer(
        player_id, suspending_player->InjectDemuxer(std::move(demuxer)),
        suspending_player->InjectManager(manager), video_hole));
    return suspending_player.release();
  }

  static MediaPlayerInterfaceEfl* CreateStreamPlayer(
      int player_id,
      std::unique_ptr<DemuxerEfl> demuxer,
      MediaPlayerManager* manager,
      bool video_hole) {
    auto suspending_player =
        base::MakeUnique<SuspendingMediaPlayer>(constructor_tag());
    suspending_player->InjectMediaPlayer(MediaPlayerEfl::CreateStreamPlayer(
        player_id, suspending_player->InjectDemuxer(std::move(demuxer)),
        suspending_player->InjectManager(manager), video_hole));
    return suspending_player.release();
  }

  explicit SuspendingMediaPlayer(constructor_tag) : SuspendingMediaPlayer() {}

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

  // MediaPlayerManager interface
  MediaPlayerInterfaceEfl* GetPlayer(int player_id) override;
  void OnTimeChanged(int player_id) override;
  void OnTimeUpdate(int player_id, base::TimeDelta current_time) override;
  void OnRequestSeek(int player_id, base::TimeDelta seek_time) override;
  void OnPauseStateChange(int player_id, bool state) override;
  void OnSeekComplete(int player_id) override;
  void OnBufferUpdate(int player_id, int buffering_percentage) override;
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
                           base::SharedMemoryHandle handle,
                           uint32_t length,
                           base::TimeDelta timestamp) override;
  void OnPlayerDestroyed(int player_id);
  void OnInitComplete(int player_id, bool success) override;
  void OnResumeComplete(int player_id) override;
  void OnSuspendComplete(int player_id) override;

#if defined(TIZEN_VIDEO_HOLE)
  gfx::Rect GetViewportRect(int player_id) const override;
#endif

#if defined(TIZEN_TBM_SUPPORT)
  void OnNewTbmBufferAvailable(int player_id,
                               gfx::TbmBufferHandle tbm_handle,
                               base::TimeDelta timestamp) override;
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
  void GetPlayTrackInfo(int player_id, int id) override;
  void NotifySubtitlePlay(int id,
                          const std::string& url,
                          const std::string& lang) override;
  void OnSeekableTimeChange(int player_id,
                            base::TimeDelta min_time,
                            base::TimeDelta max_time) override;
  void OnPlayerResourceConflict(int player_id) override;
  void OnDrmError(int player_id) override;
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
  void OnRegisterTimelineCbInfo(
      int player_id,
      const blink::WebMediaPlayer::register_timeline_cb_info_s& info) override;
  void OnSyncTimelineCbInfo(int player_id,
                            const std::string& timeline_selector,
                            int sync) override;
  void OnMrsUrlChange(int player_id, const std::string& mrsurl) override;
  void OnContentIdChange(int player_id, const std::string& contentId) override;
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
  void SetActiveTextTrack(int id, bool is_in_band) override;
  void SetActiveAudioTrack(int index) override;
  void SetActiveVideoTrack(int index) override;
  void OnInbandEventTextTrack(int player_id,
                              const std::string& info,
                              int action) override;
  void OnInbandEventTextCue(int player_id,
                            const std::string& info,
                            unsigned int id,
                            long long int start_time,
                            long long int end_time) override;
  unsigned GetDroppedFrameCount() override;
  unsigned GetDecodedFrameCount() const override;
  void ElementRemove() override;
  void SetParentalRatingResult(bool is_pass) override;
  void OnPlayerStarted(int player_id, bool player_started) override;
  bool IsResourceConflict() const override;
  void ResetResourceConflict() override;
#endif

  void StorePosition(const base::TimeDelta& position);

  content::WebContents* GetWebContents(int player_id) override;

  bool IsPaused() const override { return player_->IsPaused(); }

  void OnPlayerSuspendRequest(int player_id) override;

  bool ShouldSeekAfterResume() const override {
    return player_->ShouldSeekAfterResume();
  }

  bool HasConfigs() const override { return player_->HasConfigs(); }

  PlayerRoleFlags GetRoles() const noexcept override;

 private:
  SuspendingMediaPlayer()
      : task_runner_(base::ThreadTaskRunnerHandle::Get()),
        weak_factory_(this) {}

  enum class player_state {
    RESUMED,
    SUSPENDING,
    SUSPENDED,
    RESUMING,
  };

  enum class action_after_resume { NONE, PLAY, PAUSE };

  enum class seek_state { SEEK_NONE, SEEK_ON_DEMUXER, SEEK_ON_PLAYER };

  MediaPlayerManager* InjectManager(MediaPlayerManager* manager) {
    manager_ = manager;
    return this;
  }

  // Must take ownership
  MediaPlayerInterfaceEfl* InjectMediaPlayer(MediaPlayerInterfaceEfl* player) {
    player_ = std::unique_ptr<MediaPlayerInterfaceEfl>(player);
    return this;
  }

  std::unique_ptr<DemuxerEfl> InjectDemuxer(
      std::unique_ptr<DemuxerEfl> demuxer) {
    return base::MakeUnique<SuspendingMediaSourcePlayer>(this,
                                                         std::move(demuxer));
  }

  void AddFunctionToQueue(std::function<void()> function);
  void ExecuteFunction(const std::function<void()>& function);
  void ExecuteQueuedFunctionsAfterResume();

  std::unique_ptr<MediaPlayerInterfaceEfl> player_;
  MediaPlayerManager* manager_ = nullptr;

  player_state state_{player_state::RESUMED};
  seek_state seek_state_{seek_state::SEEK_NONE};
  bool is_paused_{false};
  bool is_seek_after_resume_{false};
  base::TimeDelta stored_position_;
  action_after_resume action_after_resume_{action_after_resume::NONE};

  std::queue<std::function<void()>> functions_to_be_called_after_resume_;

  const scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  base::WeakPtrFactory<SuspendingMediaPlayer> weak_factory_;

  friend class SuspendingMediaSourcePlayer;

  DISALLOW_COPY_AND_ASSIGN(SuspendingMediaPlayer);
};

}  // namespace media

#endif  // MEDIA_BASE_TIZEN_SUSPENDING_MEDIA_PLAYER_H_
