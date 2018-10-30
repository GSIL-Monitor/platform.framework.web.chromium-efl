// Copyright 2014 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_TIZEN_MEDIA_PLAYER_UTIL_EFL_H_
#define MEDIA_BASE_TIZEN_MEDIA_PLAYER_UTIL_EFL_H_

#include <player.h>
#include <tbm_surface.h>

#include "base/timer/timer.h"
#include "media/base/media_export.h"
#include "url/gurl.h"

#if defined(TIZEN_SOUND_FOCUS)
#include <sound_manager.h>
#endif

#if defined(TIZEN_WEBRTC)
#include <media_codec.h>
#endif

#define BLINKFREE(p) \
  do {               \
    if (p) {         \
      free(p);       \
      p = 0;         \
    }                \
  } while (0)

namespace media {

struct MediaFormatDeleter {
  void operator()(media_format_s* ptr) const;
};

struct MediaPacketDeleter {
  void operator()(media_packet_s* ptr) const;
};

typedef std::unique_ptr<media_format_s, MediaFormatDeleter> ScopedMediaFormat;
typedef std::unique_ptr<media_packet_s, MediaPacketDeleter> ScopedMediaPacket;

class FrameDumper final {
 public:
  explicit FrameDumper(const std::string& player_id);
  ~FrameDumper();

  void DumpFrame(tbm_surface_info_s* suf_info);

 private:
  void OpenDumpFile();
  void CloseDumpFile();

  int dump_fd_;
  const std::string player_id_;
};

class FPSCounter final {
 public:
  explicit FPSCounter(const std::string& player_id);
  ~FPSCounter();

  void CountFrame();

 private:
  void PrintFPS();

  base::RepeatingTimer fps_timer_;
  int frame_count_;
  long long total_frame_count_;
  const std::string player_id_;
  base::Time last_printed_;
  base::Time started_;
};

typedef enum {
  MEDIA_SEEK_NONE,          // No seek
  MEDIA_SEEK_DEMUXER,       // Demuxer seeking
  MEDIA_SEEK_DEMUXER_DONE,  // Demuxer seek done
  MEDIA_SEEK_PLAYER,        // Player is seeking
} MediaSeekState;

// Error types for MediaPlayerEfl
enum MediaError {
  MEDIA_ERROR_FORMAT,   // Unsupported Format
  MEDIA_ERROR_DECODE,   // Decoder error.
  MEDIA_ERROR_NETWORK,  // Network error.
};

const char* GetString(player_error_e error);
const char* GetString(player_state_e state);
const char* GetString(MediaSeekState state);

#if defined(TIZEN_SOUND_FOCUS)
const char* GetString(sound_stream_focus_change_reason_e reason);
#endif

#if defined(TIZEN_WEBRTC)
const char* GetString(mediacodec_error_e error);
#endif

MediaError GetMediaError(int capi_player_error);

double ConvertNanoSecondsToSeconds(int64_t time);
double ConvertMilliSecondsToSeconds(int time);
double ConvertSecondsToMilliSeconds(double time);

// Removes query string from URI.
MEDIA_EXPORT GURL GetCleanURL(std::string url);

int PlayerGetPlayPosition(player_h player, base::TimeDelta* position);
int PlayerSetPlayPosition(player_h player,
                          base::TimeDelta position,
                          bool accurate,
                          player_seek_completed_cb callback,
                          void* user_data);

player_display_rotation_e ConvertToPlayerDisplayRotation(int rotation);

void WakeUpDisplayAndAcquireDisplayLock();
void ReleaseDisplayLock();

}  // namespace media

#endif  // MEDIA_BASE_TIZEN_MEDIA_PLAYER_UTIL_EFL_H_
