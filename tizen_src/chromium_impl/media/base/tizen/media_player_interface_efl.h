// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_TIZEN_MEDIA_PLAYER_INTERFACE_EFL_H_
#define MEDIA_BASE_TIZEN_MEDIA_PLAYER_INTERFACE_EFL_H_

#include "base/time/time.h"
#include "base/tizen/flags.h"

#if defined(OS_TIZEN_TV_PRODUCT)
#include <emeCDM/IEME.h>
#endif

#if defined(TIZEN_SOUND_FOCUS)
#include "media/base/tizen/sound_focus_manager.h"
#endif

namespace gfx {
class RectF;
}

namespace media {

enum class MediaType {
  Video = 0x1,
  Audio = Video << 1,
  Text = Video << 2,
  Neither = Video << 3,
};

using MediaTypeFlags = base::Flags<MediaType>;
DEFINE_OPERATORS_FOR_FLAGS(MediaTypeFlags);

enum class PlayerRole {
  UrlBased = 0x1,
  ElementaryStreamBased = UrlBased << 1,
  RemotePeerStream = ElementaryStreamBased << 1
};

using PlayerRoleFlags = base::Flags<PlayerRole>;
DEFINE_OPERATORS_FOR_FLAGS(PlayerRoleFlags);

class MediaPlayerInterfaceEfl
#if defined(TIZEN_SOUND_FOCUS)
    : public SoundFocusClient
#endif
{
 public:
  virtual ~MediaPlayerInterfaceEfl() = default;

  virtual int GetPlayerId() const = 0;

#if defined(OS_TIZEN_TV_PRODUCT)
  virtual void SetDecryptor(eme::eme_decryptor_t) = 0;
  virtual void SetHasEncryptedListenerOrCdm(bool) = 0;
#endif

  // Initialize a player to prepare it for accepting playback commands.
  virtual void Initialize() = 0;

  // Start playing the media.
  virtual void Play() = 0;

  // Pause the player. If |is_media_related_action| is set to |true| it
  // indicates that the player itself needs to pause to buffer. |false| means
  // that HTMLMediaElement has requested the player to pause.
  virtual void Pause(bool is_media_related_action) = 0;

  // Set playback rate.
  virtual void SetRate(double rate) = 0;

  // Seek to a particular position based on renderer signaling actual seek.
  // If the player finishes seeking, OnSeekComplete() will be called.
  virtual void Seek(base::TimeDelta time) = 0;

  // Set the player audio volume.
  virtual void SetVolume(double volume) = 0;

  // Get the current playback position.
  virtual base::TimeDelta GetCurrentTime() = 0;

  // The player is now displayed in fullscreen.
  virtual void EnteredFullscreen() = 0;

  // The player is back to a normal display mode.
  virtual void ExitedFullscreen() = 0;

  // Return the type of media which this player currently plays and needs.
  // resources for.
  virtual MediaTypeFlags GetMediaType() const = 0;

  // Free all the resources returned by |GetMediaType|.
  virtual void Suspend() = 0;

  // Reallocate resources after they have been freed using |Suspend|. The
  // player needs all the resouces as returned by |GetMediaType|.
  virtual void Resume() = 0;

  virtual bool IsPlayerSuspended() const = 0;

  // Return whether the player is playing or is it in pause state.
  virtual bool IsPaused() const = 0;

  // Return whether the player should call seek after resume
  virtual bool ShouldSeekAfterResume() const = 0;

#if defined(OS_TIZEN_TV_PRODUCT)
  virtual void NotifySubtitlePlay(int id,
                                  const std::string& url,
                                  const std::string& lang) = 0;
  virtual bool RegisterTimeline(const std::string& timeline_selector) = 0;
  virtual bool UnRegisterTimeline(const std::string& timeline_selector) = 0;
  virtual void GetTimelinePositions(const std::string& timeline_selector,
                                    uint32_t* units_per_tick,
                                    uint32_t* units_per_second,
                                    int64_t* content_time,
                                    bool* paused) = 0;
  virtual double GetSpeed() = 0;
  virtual std::string GetMrsUrl() = 0;
  virtual std::string GetContentId() = 0;
  virtual bool SyncTimeline(const std::string& timeline_selector,
                            int64_t timeline_pos,
                            int64_t wallclock_pos,
                            int tolerance) = 0;
  virtual bool SetWallClock(const std::string& wallclock_url) = 0;
  virtual void SetActiveTextTrack(int, bool) = 0;
  virtual void SetActiveAudioTrack(int) = 0;
  virtual void SetActiveVideoTrack(int) = 0;
  virtual unsigned GetDroppedFrameCount() = 0;
  virtual unsigned GetDecodedFrameCount() const = 0;
  virtual void ElementRemove() = 0;
  virtual void SetParentalRatingResult(bool is_pass) = 0;
  virtual bool IsResourceConflict() const = 0;
  virtual void ResetResourceConflict() = 0;
#endif

#if defined(TIZEN_VIDEO_HOLE)
  virtual void SetGeometry(const gfx::RectF&) = 0;
  virtual void OnWebViewMoved() = 0;
#endif

#if defined(TIZEN_SOUND_FOCUS)
  virtual void SetResumePlayingBySFM(bool resume_playing) = 0;
#endif

  virtual double GetStartDate() = 0;

  virtual bool HasConfigs() const = 0;

  virtual PlayerRoleFlags GetRoles() const noexcept = 0;
};

}  // namespace media


namespace std {
inline ostream& operator<<(ostream& o, media::MediaType type) {
  switch (type) {
    case media::MediaType::Video:
      return o << 'V';
    case media::MediaType::Audio:
      return o << 'A';
    case media::MediaType::Text:
      return o << 'T';
    case media::MediaType::Neither:
      return o << "(neither)";
  }
  return o << "MediaType(" << (int)type << ")";
}
}  // namespace std

#endif  // MEDIA_BASE_TIZEN_MEDIA_PLAYER_INTERFACE_EFL_H_
