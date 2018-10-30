// Copyright 2014 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_TIZEN_MEDIA_PLAYER_EFL_H_
#define MEDIA_BASE_TIZEN_MEDIA_PLAYER_EFL_H_

#include <string>

#include "base/callback.h"
#include "base/time/time.h"
#include "media/base/tizen/media_player_interface_efl.h"
#include "media/base/tizen/media_player_util_efl.h"
#include "media/base/tizen/video_plane_controller_capi.h"
#if defined(OS_TIZEN_TV_PRODUCT)
#include "content/public/browser/web_contents_delegate.h"
#define USE_DIRECT_CALL_MEDIA_PACKET_THREAD 1
#endif

typedef struct media_packet_s* media_packet_h;

namespace gfx {
class RectF;
class Rect;
}  // namespace gfx

namespace media {

class DemuxerEfl;
class MediaPlayerManager;

// media type masks
const int MEDIA_AUDIO_MASK = 0x02;
const int MEDIA_VIDEO_MASK = 0x01;

#if defined(TIZEN_VD_MULTIPLE_MIXERDECODER)
const int DEPRECATED_PARAM = 0;
const int MIXER_SCREEN_HEIGHT = 1080;
const int MIXER_SCREEN_WIDTH = 1920;
#endif

#if defined(TIZEN_TBM_SUPPORT) && defined(OS_TIZEN_TV_PRODUCT)
const char* const kWeb360PlayStateElmHint = "wm.monitor.use.vr360";
#endif

const double kInvalidPlayRate = -12345.0;
const double kPrecisionUnit = 0.00001;
#define DOUBLE_EQUAL(a, b) \
  (((a) - (b) > -kPrecisionUnit) && ((a) - (b) < kPrecisionUnit))

class MEDIA_EXPORT MediaPlayerEfl : public MediaPlayerInterfaceEfl {
 public:
  static MediaPlayerEfl* CreatePlayer(int player_id,
                                      const GURL& url,
                                      const std::string& mime_type,
                                      MediaPlayerManager* manager,
                                      const std::string& user_agent,
                                      int audio_latency_mode,
                                      bool video_hole);

  static MediaPlayerEfl* CreatePlayer(int player_id,
                                      std::unique_ptr<DemuxerEfl> demuxer,
                                      MediaPlayerManager* manager,
                                      bool video_hole);
  static MediaPlayerEfl* CreateStreamPlayer(int player_id,
                                            std::unique_ptr<DemuxerEfl> demuxer,
                                            MediaPlayerManager* manager,
                                            bool video_hole);
  ~MediaPlayerEfl() override;

#if defined(TIZEN_VIDEO_HOLE)
  static void SetSharedVideoWindowHandle(void* handle, bool is_video_window) {
    main_window_handle_ = handle;
    is_video_window_ = is_video_window;
  }

  static void* main_window_handle() { return main_window_handle_; }
  static bool is_video_window() { return is_video_window_; }
  gfx::Rect GetViewportRect();

  virtual void SetGeometry(const gfx::RectF&) = 0;
  virtual void OnWebViewMoved() = 0;

#endif

  MediaTypeFlags GetMediaType() const override { return media_type_; }
  void SetMediaType(MediaTypeFlags type) { media_type_ |= type; }

  int GetPlayerId() const override { return player_id_; }

  virtual void Initialize() {}

#if defined(OS_TIZEN_TV_PRODUCT)
  void SetDecryptor(eme::eme_decryptor_t) override {}
  void SetHasEncryptedListenerOrCdm(bool) override {}
#endif

  virtual void EnteredFullscreen();
  virtual void ExitedFullscreen();

  void Resume() override { is_suspended_ = false; }
  void Suspend() override { is_suspended_ = true; }
  bool IsPlayerSuspended() const override { return is_suspended_; }

  bool IsPaused() const override { return is_paused_; }

  void OnPlayerSuspendRequest() const;
  bool ShouldSeekAfterResume() const override {
    return should_seek_after_resume_;
  }

#if defined(TIZEN_SOUND_FOCUS)
  // APIs for Sound Focus Manager
  void SetResumePlayingBySFM(bool resume_playing) override {
    should_be_resumed_by_sound_focus_manager_ = resume_playing;
  }
  void OnFocusAcquired() override;
  void OnFocusReleased(bool resume_playing) override;
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
  void OnPlayerResourceConflict() const;
  void ResetResourceConflict() { resource_conflict_event_ = false; }
  bool IsResourceConflict() const { return resource_conflict_event_; }
  virtual void UpdateActiveTextTrack(int) {}
  virtual void NotifySubtitlePlay(int id,
                                  const std::string& url,
                                  const std::string& lang) {}
  virtual bool RegisterTimeline(const std::string& timeline_selector) {
    return false;
  }
  virtual bool UnRegisterTimeline(const std::string& timeline_selector) {
    return false;
  }
  virtual void GetTimelinePositions(const std::string& timeline_selector,
                                    uint32_t* units_per_tick,
                                    uint32_t* units_per_second,
                                    int64_t* content_time,
                                    bool* paused) {}
  virtual double GetSpeed() { return 0.0; }
  virtual std::string GetMrsUrl() { return ""; }
  virtual std::string GetContentId() { return ""; }
  virtual bool SyncTimeline(const std::string& timeline_selector,
                            int64_t timeline_pos,
                            int64_t wallclock_pos,
                            int tolerance) {
    return false;
  }
  virtual bool SetWallClock(const std::string& wallclock_url) { return false; }
  bool CheckMarlinEnable();
  unsigned GetDroppedFrameCount() override { return 0; }
  unsigned GetDecodedFrameCount() const override { return 0; }
  bool CheckHighBitRate();
  virtual void ElementRemove() {}
  virtual void SetParentalRatingResult(bool is_pass) {}
#endif

  double GetStartDate() override {
    return std::numeric_limits<double>::quiet_NaN();
  }
  bool HasConfigs() const { return true; }

 protected:
  explicit MediaPlayerEfl(int player_id,
                          MediaPlayerManager* manager,
                          bool video_hole);

  friend player_h VideoPlaneControllerCapi::GetPlayerHandle(
      MediaPlayerEfl* media_player) const;

  // Release the player resources.
  virtual void Release() = 0;
  MediaPlayerManager* manager() const { return manager_; }

  void DeliverMediaPacket(ScopedMediaPacket packet);
  virtual void OnMediaError(MediaError error_type);

#if defined(OS_TIZEN_TV_PRODUCT)
  bool PlaybackNotificationEnabled();
  bool SubtitleNotificationEnabled();
  void UpdateCurrentTime(base::TimeDelta current_time);
  content::WebContentsDelegate* GetWebContentsDelegate() const;
  void NotifyPlaybackState(int state,
                           const std::string& url = "",
                           const std::string& mime_type = "",
                           bool* media_resource_acquired = NULL,
                           std::string* translated_url = NULL,
                           std::string* drm_info = NULL);
  virtual void SetActiveTextTrack(int, bool) {}
  virtual void SetActiveAudioTrack(int) {}
  virtual void SetActiveVideoTrack(int) {}
#endif

  int width_;
  int height_;
  player_h player_;
  bool is_paused_;
  bool should_seek_after_resume_;
  bool is_fullscreen_;

  double playback_rate_;
  double pending_playback_rate_;
  bool is_video_hole_;
  double volume_;
  bool player_prepared_;

#if defined(TIZEN_SOUND_FOCUS)
  bool should_be_resumed_by_sound_focus_manager_;
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
  int notify_playback_state_;
  mutable bool resource_conflict_event_;
#endif

#if defined(TIZEN_TBM_SUPPORT) && defined(OS_TIZEN_TV_PRODUCT)
  int web360_status_hint_id_;
#endif

 private:
  // Player ID assigned to this player.
  int player_id_;
  bool is_suspended_;
  MediaTypeFlags media_type_;

#if TIZEN_MM_DEBUG_VIDEO_DUMP_DECODED_FRAME == 1
  std::unique_ptr<FrameDumper> frameDumper;
#endif

#if TIZEN_MM_DEBUG_VIDEO_PRINT_FPS == 1
  std::unique_ptr<FPSCounter> fpsCounter;
#endif

  // Resource manager for all the media players.
  MediaPlayerManager* manager_;

#if defined(TIZEN_VIDEO_HOLE)
  static void* main_window_handle_;
  static bool is_video_window_;
#endif

  DISALLOW_COPY_AND_ASSIGN(MediaPlayerEfl);
};

}  // namespace media

#endif  // MEDIA_BASE_TIZEN_MEDIA_PLAYER_EFL_H_
