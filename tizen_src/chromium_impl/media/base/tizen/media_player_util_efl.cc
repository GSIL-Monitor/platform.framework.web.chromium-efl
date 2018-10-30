// Copyright 2014 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/tizen/media_player_util_efl.h"

#include <fcntl.h>
#include <player.h>
#include <unistd.h>
#include <iomanip>

#include "base/logging.h"
#include "base/time/time.h"
#include "build/tizen_version.h"
#include "ewk/efl_integration/wrt/wrt_widget_host.h"
#include "tizen/system_info.h"

#include <device/power.h>
#include <player.h>

#if defined(OS_TIZEN_TV_PRODUCT)
#include <player_product.h>
#endif

namespace media {

// Check display is locked on a chromium side.
// Integer type is for handling multiple videos.
static int s_internal_power_lock_count = 0;

FrameDumper::FrameDumper(const std::string& player_id)
    : dump_fd_(-1), player_id_(player_id) {
  OpenDumpFile();
}

FrameDumper::~FrameDumper() {
  CloseDumpFile();
}

void FrameDumper::OpenDumpFile() {
  std::stringstream dump_file;
  dump_file << "/opt/usr/yuv_dump_" << player_id_ << ".raw";
  dump_fd_ = open(dump_file.str().c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);

  if (dump_fd_ == -1)
    LOG(ERROR) << "Player " << player_id_ << " : File Open Failed";
}

void FrameDumper::CloseDumpFile() {
  if (dump_fd_ != -1)
    close(dump_fd_);
}

void FrameDumper::DumpFrame(tbm_surface_info_s* suf_info) {
  if (!suf_info)
    return;

  int written =
      write(dump_fd_, suf_info->planes[0].ptr, suf_info->planes[0].size);
  written += write(dump_fd_, suf_info->planes[1].ptr, suf_info->planes[1].size);
  written += write(dump_fd_, suf_info->planes[2].ptr, suf_info->planes[2].size);

  if (written != static_cast<int>(suf_info->size))
    LOG(ERROR) << "Player " << player_id_ << " : Decoded frame dump failed";
}

FPSCounter::FPSCounter(const std::string& player_id)
    : frame_count_(0), total_frame_count_(0), player_id_(player_id) {}

FPSCounter::~FPSCounter() {}

void FPSCounter::CountFrame() {
  ++frame_count_;
  ++total_frame_count_;
  if (started_.is_null()) {
    started_ = base::Time::Now();
    last_printed_ = base::Time::Now();
    fps_timer_.Start(FROM_HERE, base::TimeDelta::FromSeconds(1), this,
                     &FPSCounter::PrintFPS);
  }
}

void FPSCounter::PrintFPS() {
  base::Time now = base::Time::Now();
  base::TimeDelta delta_tick = now - last_printed_;
  base::TimeDelta delta_start = now - started_;
  double fps_tick =
      static_cast<double>(frame_count_) * 1000 / delta_tick.InMilliseconds();
  double fps_start = static_cast<double>(total_frame_count_) * 1000 /
                     delta_start.InMilliseconds();
  LOG(INFO) << "Player(" << player_id_ << "): " << std::fixed
            << std::setprecision(2) << fps_tick << "FPS"
            << "(AVG:" << std::setprecision(2) << fps_start << "FPS)";
  frame_count_ = 0;
  last_printed_ = now;
}

#define ENUM_CASE(x) \
  case x:            \
    return #x;       \
    break

const char* GetString(player_error_e error) {
  switch (error) {
    ENUM_CASE(PLAYER_ERROR_NONE);
    ENUM_CASE(PLAYER_ERROR_OUT_OF_MEMORY);
    ENUM_CASE(PLAYER_ERROR_INVALID_PARAMETER);
    ENUM_CASE(PLAYER_ERROR_NO_SUCH_FILE);
    ENUM_CASE(PLAYER_ERROR_INVALID_OPERATION);
    ENUM_CASE(PLAYER_ERROR_FILE_NO_SPACE_ON_DEVICE);
    ENUM_CASE(PLAYER_ERROR_FEATURE_NOT_SUPPORTED_ON_DEVICE);
    ENUM_CASE(PLAYER_ERROR_SEEK_FAILED);
    ENUM_CASE(PLAYER_ERROR_INVALID_STATE);
    ENUM_CASE(PLAYER_ERROR_NOT_SUPPORTED_FILE);
    ENUM_CASE(PLAYER_ERROR_INVALID_URI);
    ENUM_CASE(PLAYER_ERROR_SOUND_POLICY);
    ENUM_CASE(PLAYER_ERROR_CONNECTION_FAILED);
    ENUM_CASE(PLAYER_ERROR_VIDEO_CAPTURE_FAILED);
#if defined(OS_TIZEN_TV_PRODUCT)
    ENUM_CASE(PLAYER_ERROR_DRM_INFO);
    ENUM_CASE(PLAYER_ERROR_NETWORK_ERROR);
#endif
    ENUM_CASE(PLAYER_ERROR_DRM_EXPIRED);
    ENUM_CASE(PLAYER_ERROR_DRM_NO_LICENSE);
    ENUM_CASE(PLAYER_ERROR_DRM_FUTURE_USE);
    ENUM_CASE(PLAYER_ERROR_DRM_NOT_PERMITTED);
    ENUM_CASE(PLAYER_ERROR_RESOURCE_LIMIT);
    ENUM_CASE(PLAYER_ERROR_PERMISSION_DENIED);
#if defined(OS_TIZEN) && TIZEN_VERSION_AT_LEAST(3, 0, 0)
    ENUM_CASE(PLAYER_ERROR_SERVICE_DISCONNECTED);
    ENUM_CASE(PLAYER_ERROR_BUFFER_SPACE);
#endif
    default:
      LOG(WARNING) << "Unknown player_error_e! (code=" << error << ")";
  }
  NOTREACHED() << "Invalid player_error_e! (code=" << error << ")";
  return "";
}

const char* GetString(player_state_e state) {
  switch (state) {
    ENUM_CASE(PLAYER_STATE_NONE);
    ENUM_CASE(PLAYER_STATE_IDLE);
    ENUM_CASE(PLAYER_STATE_READY);
    ENUM_CASE(PLAYER_STATE_PLAYING);
    ENUM_CASE(PLAYER_STATE_PAUSED);
  };
  NOTREACHED() << "Invalid player_state_e! (code=" << state << ")";
  return "";
}

#if defined(TIZEN_WEBRTC)
const char* GetString(mediacodec_error_e error) {
  switch (error) {
    ENUM_CASE(MEDIACODEC_ERROR_NONE);
    ENUM_CASE(MEDIACODEC_ERROR_OUT_OF_MEMORY);
    ENUM_CASE(MEDIACODEC_ERROR_INVALID_PARAMETER);
    ENUM_CASE(MEDIACODEC_ERROR_INVALID_OPERATION);
    ENUM_CASE(MEDIACODEC_ERROR_NOT_SUPPORTED_ON_DEVICE);
    ENUM_CASE(MEDIACODEC_ERROR_PERMISSION_DENIED);
    ENUM_CASE(MEDIACODEC_ERROR_INVALID_STATE);
    ENUM_CASE(MEDIACODEC_ERROR_INVALID_INBUFFER);
    ENUM_CASE(MEDIACODEC_ERROR_INVALID_OUTBUFFER);
    ENUM_CASE(MEDIACODEC_ERROR_INTERNAL);
    ENUM_CASE(MEDIACODEC_ERROR_NOT_INITIALIZED);
    ENUM_CASE(MEDIACODEC_ERROR_INVALID_STREAM);
    ENUM_CASE(MEDIACODEC_ERROR_CODEC_NOT_FOUND);
    ENUM_CASE(MEDIACODEC_ERROR_DECODE);
    ENUM_CASE(MEDIACODEC_ERROR_NO_FREE_SPACE);
    ENUM_CASE(MEDIACODEC_ERROR_STREAM_NOT_FOUND);
    ENUM_CASE(MEDIACODEC_ERROR_NOT_SUPPORTED_FORMAT);
    ENUM_CASE(MEDIACODEC_ERROR_BUFFER_NOT_AVAILABLE);
    ENUM_CASE(MEDIACODEC_ERROR_OVERFLOW_INBUFFER);
    ENUM_CASE(MEDIACODEC_ERROR_RESOURCE_OVERLOADED);
    ENUM_CASE(MEDIACODEC_ERROR_RESOURCE_CONFLICT);
  };
  NOTREACHED() << "Invalid mediacodec_error_e! (code=" << error << ")";
  return "";
}
#endif

const char* GetString(MediaSeekState state) {
  switch (state) {
    ENUM_CASE(MEDIA_SEEK_NONE);
    ENUM_CASE(MEDIA_SEEK_DEMUXER);
    ENUM_CASE(MEDIA_SEEK_DEMUXER_DONE);
    ENUM_CASE(MEDIA_SEEK_PLAYER);
  };
  NOTREACHED() << "Invalid MediaSeekState! (code=" << state << ")";
  return "";
}
#if defined(TIZEN_SOUND_FOCUS)
const char* GetString(sound_stream_focus_change_reason_e reason) {
  switch (reason) {
    ENUM_CASE(SOUND_STREAM_FOCUS_CHANGED_BY_MEDIA);
    ENUM_CASE(SOUND_STREAM_FOCUS_CHANGED_BY_SYSTEM);
    ENUM_CASE(SOUND_STREAM_FOCUS_CHANGED_BY_ALARM);
    ENUM_CASE(SOUND_STREAM_FOCUS_CHANGED_BY_NOTIFICATION);
    ENUM_CASE(SOUND_STREAM_FOCUS_CHANGED_BY_EMERGENCY);
    ENUM_CASE(SOUND_STREAM_FOCUS_CHANGED_BY_VOICE_INFORMATION);
    ENUM_CASE(SOUND_STREAM_FOCUS_CHANGED_BY_VOICE_RECOGNITION);
    ENUM_CASE(SOUND_STREAM_FOCUS_CHANGED_BY_RINGTONE);
    ENUM_CASE(SOUND_STREAM_FOCUS_CHANGED_BY_VOIP);
    ENUM_CASE(SOUND_STREAM_FOCUS_CHANGED_BY_CALL);
    ENUM_CASE(SOUND_STREAM_FOCUS_CHANGED_BY_MEDIA_EXTERNAL_ONLY);
  };
  NOTREACHED() << "Invalid sound_stream_focus_change_reason_e! (reason ="
               << reason << ")";
  return "";
}
#endif

#undef ENUM_CASE

MediaError GetMediaError(int capi_player_error) {
  MediaError retval = MEDIA_ERROR_DECODE;

  if (capi_player_error == PLAYER_ERROR_NO_SUCH_FILE ||
      capi_player_error == PLAYER_ERROR_INVALID_URI ||
      capi_player_error == PLAYER_ERROR_NOT_SUPPORTED_FILE)
    retval = MEDIA_ERROR_FORMAT;
  else if (capi_player_error == PLAYER_ERROR_CONNECTION_FAILED
#if defined(OS_TIZEN_TV_PRODUCT)
           || capi_player_error == PLAYER_ERROR_NETWORK_ERROR
#endif
           )
    retval = MEDIA_ERROR_NETWORK;

  return retval;
}

void MediaPacketDeleter::operator()(media_packet_s* ptr) const {
  if (ptr != nullptr)
    media_packet_destroy(ptr);
}

void MediaFormatDeleter::operator()(media_format_s* ptr) const {
  if (ptr != nullptr)
    media_format_unref(ptr);
}

double ConvertNanoSecondsToSeconds(int64_t time) {
  return base::TimeDelta::FromMicroseconds(
             time / base::Time::kNanosecondsPerMicrosecond)
      .InSecondsF();
}

double ConvertMilliSecondsToSeconds(int time) {
  return base::TimeDelta::FromMilliseconds(time).InSecondsF();
}

double ConvertSecondsToMilliSeconds(double time) {
  if (time < 0) {
    LOG(ERROR) << "Invalid time:" << time << " Reset to 0";
    time = 0;
  }

  return base::TimeDelta::FromSecondsD(time).InMillisecondsF();
}

GURL GetCleanURL(std::string url) {
  // FIXME: Need to consider "app://" scheme.
  CHECK(url.compare(0, 6, "app://"));
  if (!url.compare(0, 7, "file://")) {
    int position = url.find("?");
    if (position != -1)
      url.erase(url.begin() + position, url.end());
  }
  GURL url_(url);
  return url_;
}

int PlayerGetPlayPosition(player_h player, base::TimeDelta* position) {
#if defined(OS_TIZEN_TV_PRODUCT)
  int64_t player_position = 0;
  const int ret_val = player_get_play_position_ex(player, &player_position);
#else
  int player_position = 0;
  const int ret_val = player_get_play_position(player, &player_position);
#endif
  *position = base::TimeDelta::FromMilliseconds(player_position);
  return ret_val;
}

int PlayerSetPlayPosition(player_h player,
                          base::TimeDelta position,
                          bool accurate,
                          player_seek_completed_cb callback,
                          void* user_data) {
#if defined(OS_TIZEN_TV_PRODUCT)
  return player_set_play_position_ex(player, position.InMilliseconds(),
                                     accurate, callback, user_data);
#else
  return player_set_play_position(player, position.InMilliseconds(), accurate,
                                  callback, user_data);
#endif
}

player_display_rotation_e ConvertToPlayerDisplayRotation(int rotation) {
  switch (rotation) {
#if defined(OS_TIZEN_TV_PRODUCT) && defined(TIZEN_VD_LFD_ROTATE)
    case 90:
      return PLAYER_DISPLAY_ROTATION_270;
    case 270:
      return PLAYER_DISPLAY_ROTATION_90;
#else
    case 90:
      return PLAYER_DISPLAY_ROTATION_90;
    case 270:
      return PLAYER_DISPLAY_ROTATION_270;
#endif
    default:
      return PLAYER_DISPLAY_ROTATION_NONE;
  }
}

void WakeUpDisplayAndAcquireDisplayLock() {
  if (!IsMobileProfile() && !IsWearableProfile())
    return;

  // If an user locks with the power webapi,
  // chromium doesn't handle power lock internally.
  if (!WrtWidgetHost::Get()->IsPowerLockEnabled()) {
    s_internal_power_lock_count = 0;
    return;
  }

  if (device_power_wakeup(false) != DEVICE_ERROR_NONE)
    LOG(ERROR) << "|device_power_wakeup| request failed";
  if (device_power_request_lock(POWER_LOCK_DISPLAY, 0) != DEVICE_ERROR_NONE)
    LOG(ERROR) << "|device_power_request_lock| request failed";
  s_internal_power_lock_count++;
}

void ReleaseDisplayLock() {
  if (!IsMobileProfile() && !IsWearableProfile())
    return;

  if (!WrtWidgetHost::Get()->IsPowerLockEnabled()) {
    s_internal_power_lock_count = 0;
    return;
  }
  // In case of the last video, release display lock.
  if (s_internal_power_lock_count == 1) {
    if (device_power_release_lock(POWER_LOCK_DISPLAY) != DEVICE_ERROR_NONE)
      LOG(ERROR) << "|device_power_release_lock| request failed";
  }

  if (s_internal_power_lock_count > 0)
    s_internal_power_lock_count--;
  else
    LOG(WARNING) << "A mismatched release was found.";
}

}  // namespace media
