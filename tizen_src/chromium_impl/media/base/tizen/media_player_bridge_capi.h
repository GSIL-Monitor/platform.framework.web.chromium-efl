// Copyright 2014 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_TIZEN_MEDIA_PLAYER_BRIDGE_CAPI_H_
#define MEDIA_BASE_TIZEN_MEDIA_PLAYER_BRIDGE_CAPI_H_

#include <player.h>

#include "base/location.h"
#include "base/memory/weak_ptr.h"
#include "base/synchronization/condition_variable.h"
#include "base/timer/timer.h"
#include "content/public/browser/browser_message_filter.h"
#include "media/base/ranges.h"
#include "media/base/timestamp_constants.h"
#include "media/base/tizen/media_player_efl.h"
#include "media/base/video_frame.h"

#if defined(TIZEN_VIDEO_HOLE)
#include "ui/gfx/geometry/rect_f.h"
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
#include <emeCDM/IEME.h>
#include <player_product.h>
#endif

namespace media {

class VideoPlaneManagerTizen;

#if defined(OS_TIZEN_TV_PRODUCT)
enum StreamType {
  HLS_STREAM = 0,
  DASH_STREAM,
  TS_STREAM,
  OTHER_STREAM,
};

enum InitDataError {
  ERROR_NONE = 0,
  NOT_CENC_DATA,
  NO_USER_DATA,
};

enum TrackAction {
  ADD_INBAND_TEXT_TRACK = 0,
  DEL_INBAND_TEXT_TRACK,
};
#endif

#if defined(TIZEN_VD_MULTIPLE_MIXERDECODER)
// Now support at most 3 video playing at the same time(hd-main: 1, hd-sub:2,
// sw: 3)
const int kMaxPlayerMixerNum = 3;

enum StateValue {
  NOT_USED,
  IN_USED,
};

enum MixerDecoderType {
  HW_MAIN,
  HW_SUB,
  OTHER,
};
#endif

class MEDIA_EXPORT MediaPlayerBridgeCapi : public MediaPlayerEfl {
 public:
  typedef base::Callback<void(const bool)> CompleteCB;

  MediaPlayerBridgeCapi(int player_id,
                        const GURL& url,
                        const std::string& mime_type,
                        MediaPlayerManager* manager,
                        const std::string& user_agent,
                        int audio_latency_mode,
                        bool video_hole);
  ~MediaPlayerBridgeCapi() override;

  // MediaPlayerTizen implementation.
  void Initialize() override;
  void Resume() override;
  void Suspend() override;
  void Play() override;
  void Pause(bool is_media_related_action) override;
  void SetRate(double rate) override;
  void Seek(base::TimeDelta time) override;
  void SetVolume(double volume) override;
  base::TimeDelta GetCurrentTime() override;
  double GetStartDate() override;

  void EnteredFullscreen() override;
  void ExitedFullscreen() override;

#if defined(TIZEN_VIDEO_HOLE)
  void SetGeometry(const gfx::RectF&) override;
  void OnWebViewMoved() override;
#endif
  void ExecuteDelayedPlayerState();

  void OnPlaybackCompleteUpdate();
  void OnSeekCompleteUpdate();
  void OnPlayerPrepared();
  void OnHandleBufferingStatus(int percent);
  void OnHandlePlayerError(int player_error_code, const base::Location& loc);
  void OnResumeComplete(bool success);
  void OnInitComplete(bool success);
  void OnMediaPacketUpdated(media_packet_h packet);
#if !defined(OS_TIZEN_TV_PRODUCT)
  void OnInterrupted(player_interrupted_code_e code);
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
  void OnPlayerPreloading();
  void OnResourceConflict();
  void OnOtherEvent(int event_type, void* event_data);

  void UpdateActiveTextTrack(int) override;
  void SubtitleDataCB(unsigned long long,
                      unsigned,
                      const std::string&,
                      unsigned);
  void NotifySubtitleData(int track_id,
                          double time_stamp,
                          const std::string& data,
                          unsigned size);
  void NotifySubtitleState(int state, double time_stamp = 0.0);
  void NotifySubtitlePlay(int id,
                          const std::string& url,
                          const std::string& lang) override;
  void NotifyStopTrack();
  void NotifyPlayTrack();
  void HandleDrmError(int err_code);
  void OnDrmError(int err_code, char* err_str);
  bool RegisterTimeline(const std::string& timeline_selector) override;
  void OnRegisterTimeline(const char* timeline_selector,
                          uint32_t units_per_tick,
                          uint32_t units_per_second,
                          int64_t content_time,
                          ETimelineState timeline_state);
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
  void OnSyncTimeline(const char* timeline_selector, int sync);
  bool SetWallClock(const std::string& wallclock_url) override;
  void HandleRegisterTimeline(const char* timeline_selector,
                              uint32_t units_per_tick,
                              uint32_t units_per_second,
                              int64_t content_time,
                              ETimelineState timeline_state);
  void HandleSyncTimeline(const char* timeline_selector, int sync);

  void HandleMrsUrlChange(const std::string& mrsUrl);
  void HandlePeriodIdChange(const std::string& contentId);
  void SetActiveTextTrack(int, bool) override;
  void SetActiveAudioTrack(int) override;
  void SetActiveVideoTrack(int) override;
  void ElementRemove() override;
  void SetParentalRatingResult(bool is_pass) override;
  static void CheckHLSSupport(const std::string& url, bool* support);
  void SetHasEncryptedListenerOrCdm(bool value) override;
  void SetDecryptor(eme::eme_decryptor_t decryptor) override;
#endif

  PlayerRoleFlags GetRoles() const noexcept override;

 protected:
  void Release() override;
  void Prepare(CompleteCB cb);
  void OnMediaError(MediaError error_type) override;

 private:
  bool CreatePlayerHandleIfNeeded();
  // |duration_update_timer_| related
  void OnCurrentTimeUpdateTimerFired();
  void StartCurrentTimeUpdateTimer();
  void StopCurrentTimeUpdateTimer();

  // |buffering_update_timer_| related
  void OnBufferingUpdateTimerFired();
  void StartBufferingUpdateTimer();
  void StopBufferingUpdateTimer();
  void DoPrepare(const std::string& fs_path);
  void HandleCookies(const std::string& cookies);

  void UpdateMediaType();
  void UpdateSeekState(bool state);
  void UpdateDuration();

  void PlaybackCompleteUpdate();
  void SeekCompleteUpdate();
  void PlayerPrepared();
  void HandleBufferingStatus(int percent);
  void RunCompleteCB(bool success, const base::Location& from);
  player_state_e GetPlayerState();
#if defined(OS_TIZEN_TV_PRODUCT)
  bool CheckLiveStreaming() const;
  bool GetLiveStreamingDuration(int64_t* min, int64_t* max) const;
  bool GetDashLiveDuration(int64_t* duration);
  void ParseDashInfo();
  std::string ParseDashKeyword(const std::string& keyword);
  void StartSeekableTimeUpdateTimer();
  void StopSeekableTimeUpdateTimer();
  void OnSeekableTimeUpdateTimerFired();
  void UpdateSeekableTime();
  // HBBTV preloading feature support.
  bool HBBTVResourceAcquired();
  void PlayerPreloaded();
  bool PlayerPrePlay();
  bool SetDrmInfo(std::string& drm_info);
  bool SetMediaDRMInfo(const std::string&, const std::string&);
  void UpdateAudioTrackInfo();
  void UpdateVideoTrackInfo();
  void UpdateTextTrackInfo();
  std::string MapMediaTrackKind(const std::string& role, const int size);
  bool GetUserPreferAudioLanguage(std::string& prefer_audio_lang);
  void UpdatePreferAudio();
  void HandleInbandTextTrack(const std::string& info, int action);
  void HandleInbandTextCue(const std::string& info,
                           unsigned int id,
                           long long int start_time,
                           long long int end_time);
  void HandleParentalRatingInfo(const std::string& info,
                                const std::string& url);
  std::string MapDashMediaTrackKind(const std::string& role,
                                    const int size,
                                    bool isAudio);
  void RemoveUrlSuffixForCleanUrl();
  // HBBTV marlin feature support
  void AppendUrlHighBitRate(const std::string& url);
  bool GetMarlinUrl();
  void HandleBufferingStatusOnTV(int percent);
  bool IsStandaloneEme() const;
  int PreparePlayerForDrmEme();
  static int OnPlayerDrmInitData(drm_init_data_type init_type,
                                 void* data,
                                 int data_length,
                                 void* user_data);
  static bool OnPlayerDrmInitComplete(int* drmhandle,
                                      unsigned int length,
                                      unsigned char* psshdata,
                                      void* user_data);
  bool CompleteDrmInit(int& drmhandle);
#endif

  void ClearDelayedPlayerStateQueue();
  void PushDelayedPlayerState(int new_state);

#if defined(TIZEN_VD_MULTIPLE_MIXERDECODER)
  bool IsMuted(void);
#endif

 private:
  const scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  GURL url_;
  double volume_;
  std::string mime_type_;

  bool is_end_reached_;
  bool is_file_url_;
  bool is_seeking_;
  bool is_resuming_;
  bool is_initialized_;
  bool is_media_related_action_;
  bool is_getting_cookies_;
  bool error_occured_;
  bool pending_prepare_;

  base::TimeDelta duration_;
  base::TimeDelta playback_time_;
  base::TimeDelta seek_duration_;
  base::TimeDelta pending_seek_duration_;
  base::TimeDelta stored_seek_time_during_resume_;
  std::deque<int> delayed_player_state_queue_;

  CompleteCB complete_callback_;
  base::RepeatingTimer current_time_update_timer_;
  base::RepeatingTimer buffering_update_timer_;
  audio_latency_mode_e audio_latency_mode_;
  int current_progress_;
  std::string user_agent_;
  std::string cookies_;

#if defined(OS_TIZEN_TV_PRODUCT)
  bool is_buffering_compeleted_ : 1;
  bool is_inband_text_track_ : 1;
  bool is_live_stream_ : 1;
  bool is_player_released_ : 1;
  bool is_player_seek_available_ : 1;
  bool need_report_buffering_state_ : 1;
  bool player_started_ : 1;

  int active_audio_track_id_;
  int active_text_track_id_;
  int active_video_track_id_;
  int last_prefer_audio_adaptionset_idx_;
  int pending_active_audio_track_id_;
  int pending_active_text_track_id_;
  int pending_active_video_track_id_;
  int player_ready_state_;
  int prefer_audio_adaptionset_idx_;
  int subtitle_state_;

  StreamType stream_type_;
  std::string clean_url_;   // Original url without suffix t/period
  std::string content_id_;  // clean_url + period,only report to APP
  std::string data_url_;
  std::string hbbtv_url_;  // url_ + HIGHBITRATE(if mpd)
  std::string mrs_url_;
  std::string prefer_audio_lang_;
  base::RepeatingTimer seekable_time_update_timer_;
  base::TimeDelta min_seekable_time_;
  base::TimeDelta max_seekable_time_;
  // flag if corresponding media element has at least one "encrypted" event
  // handler
  bool has_encrypted_listener_or_cdm_{false};
  // TODO b.chechlinsk: Supposed to flag if HbbTV is used to decrypt content.
  // Currently set if player gets "DRM info" from HbbTV through
  // WebContentsDelegate.
  bool uses_hbbtv_drm_{false};
  eme::eme_decryptor_t decryptor_;
  base::Lock decryptor_lock_;
  base::ConditionVariable decryptor_condition_;
  bool parental_rating_pass_;
#endif

#if defined(TIZEN_VIDEO_HOLE)
  std::unique_ptr<VideoPlaneManagerTizen> video_plane_manager_;
#endif

#if defined(TIZEN_VD_MULTIPLE_MIXERDECODER)
  int player_index_;
  bool mixer_decoder_hw_main_;
  bool mixer_decoder_hw_sub_;
  bool mixer_decoder_sw_;
  static player_h mixer_player_group_[kMaxPlayerMixerNum];
  static int mixer_decoder_hw_state_[2];
  static player_h mixer_player_active_audio_;
  gfx::RectF video_rect_;
#endif

  // NOTE: Weak pointers must be invalidated before all other member variables.
  base::WeakPtrFactory<MediaPlayerBridgeCapi> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MediaPlayerBridgeCapi);
};

}  // namespace media

#endif  // MEDIA_BASE_TIZEN_MEDIA_PLAYER_BRIDGE_CAPI_H_
