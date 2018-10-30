// Copyright 2015 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/tizen/media_player_efl.h"

#include <media_packet.h>

#include "content/public/browser/browser_thread.h"
#include "media/base/tizen/media_player_manager_efl.h"
#include "media/base/tizen/media_player_util_efl.h"
#include "third_party/libyuv/include/libyuv.h"

#if defined(OS_TIZEN_TV_PRODUCT)
#if defined(TIZEN_TBM_SUPPORT)
#include <Elementary.h>
#include <Evas.h>
#endif
#include "content/browser/media/tizen/browser_media_player_manager_efl.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/web_preferences.h"
#endif

namespace media {

#if defined(TIZEN_VIDEO_HOLE)
void* MediaPlayerEfl::main_window_handle_ = nullptr;
bool MediaPlayerEfl::is_video_window_ = false;

gfx::Rect MediaPlayerEfl::GetViewportRect() {
  return manager()->GetViewportRect(player_id_);
}
#endif

MediaPlayerEfl::MediaPlayerEfl(int player_id,
                               MediaPlayerManager* manager,
                               bool video_hole)
    : width_(0),
      height_(0),
      player_(NULL),
      is_paused_(true),
      should_seek_after_resume_(true),
      is_fullscreen_(false),
      playback_rate_(1.0f),
      pending_playback_rate_(kInvalidPlayRate),
      is_video_hole_(video_hole),
      volume_(1.0),
      player_prepared_(false),
#if defined(TIZEN_SOUND_FOCUS)
      should_be_resumed_by_sound_focus_manager_(false),
#endif
#if defined(OS_TIZEN_TV_PRODUCT)
      notify_playback_state_(blink::WebMediaPlayer::kPlaybackStop),
      resource_conflict_event_(false),
#endif
#if defined(TIZEN_TBM_SUPPORT) && defined(OS_TIZEN_TV_PRODUCT)
      web360_status_hint_id_(-1),
#endif
      player_id_(player_id),
      is_suspended_(false),
      media_type_(0),
      manager_(manager) {
  LOG(INFO) << "video hole of media player efl is " << is_video_hole_;
#if TIZEN_MM_DEBUG_VIDEO_DUMP_DECODED_FRAME == 1
  frameDumper.reset(new FrameDumper(std::to_string(player_id)));
#endif
#if TIZEN_MM_DEBUG_VIDEO_PRINT_FPS == 1
  fpsCounter.reset(new FPSCounter(std::to_string(player_id)));
#endif

#if defined(TIZEN_TBM_SUPPORT) && defined(OS_TIZEN_TV_PRODUCT)
  // Enhance 360 performance by no window effect.
  if (!is_video_hole_ &&
      (web360_status_hint_id_ = elm_win_aux_hint_add(
           static_cast<Evas_Object*>(MediaPlayerEfl::main_window_handle()),
           kWeb360PlayStateElmHint, "0")) == -1)
    LOG(WARNING) << "[elm hint] Add fail : " << kWeb360PlayStateElmHint;
#endif
}

MediaPlayerEfl::~MediaPlayerEfl() {
#if defined(TIZEN_TBM_SUPPORT) && defined(OS_TIZEN_TV_PRODUCT)
  if (!is_video_hole_ &&
      !elm_win_aux_hint_del(
          static_cast<Evas_Object*>(MediaPlayerEfl::main_window_handle()),
          web360_status_hint_id_))
    LOG(WARNING) << "[elm hint] Del fail : " << kWeb360PlayStateElmHint;
#endif
}

#if defined(OS_TIZEN_TV_PRODUCT)
content::WebContentsDelegate* MediaPlayerEfl::GetWebContentsDelegate() const {
  content::WebContents* web_contents =
      static_cast<content::BrowserMediaPlayerManagerEfl*>(manager())
          ->GetWebContents(GetPlayerId());
  if (!web_contents) {
    LOG(ERROR) << "GetWebContents failed";
    return nullptr;
  }

  content::WebContentsDelegate* web_contents_delegate =
      web_contents->GetDelegate();
  if (!web_contents_delegate) {
    LOG(ERROR) << "GetDelagate failed";
    return nullptr;
  }
  return web_contents_delegate;
}

void MediaPlayerEfl::NotifyPlaybackState(int state,
                                         const std::string& url,
                                         const std::string& mime_type,
                                         bool* media_resource_acquired,
                                         std::string* translated_url,
                                         std::string* drm_info) {
  if (PlaybackNotificationEnabled()) {
    content::WebContentsDelegate* web_contents_delegate =
        GetWebContentsDelegate();
    if (web_contents_delegate && notify_playback_state_ != state) {
      if (notify_playback_state_ < blink::WebMediaPlayer::kPlaybackReady &&
          state == blink::WebMediaPlayer::kPlaybackStop) {
        LOG(INFO) << "player not Ready but notify Stop,ignore it";
        return;
      }
      notify_playback_state_ = state;
      web_contents_delegate->NotifyPlaybackState(state, url, mime_type,
                                                 media_resource_acquired,
                                                 translated_url, drm_info);
    }
  }
}

bool MediaPlayerEfl::CheckMarlinEnable() {
  content::WebContentsDelegate* web_contents_delegate =
      GetWebContentsDelegate();
  if (!web_contents_delegate) {
    LOG(ERROR) << "get web_contents_delegate fail";
    return false;
  }
  bool ret = web_contents_delegate->GetMarlinEnable();
  LOG(INFO) << "get marlin enable:" << (ret ? "true" : "false");
  return ret;
}

void MediaPlayerEfl::UpdateCurrentTime(base::TimeDelta current_time) {
  content::WebContentsDelegate* web_contents_delegate =
      GetWebContentsDelegate();
  if (!web_contents_delegate)
    return;
  web_contents_delegate->UpdateCurrentTime(current_time.InSecondsF());
}

bool MediaPlayerEfl::SubtitleNotificationEnabled() {
  content::WebContents* web_contents = nullptr;
  content::RenderViewHost* render_view_host = nullptr;
  web_contents = static_cast<content::BrowserMediaPlayerManagerEfl*>(manager())
                     ->GetWebContents(GetPlayerId());
  if (!web_contents) {
    LOG(ERROR) << "GetWebContents failed";
    return false;
  }

  render_view_host = web_contents->GetRenderViewHost();
  if (!render_view_host) {
    LOG(ERROR) << "GetRenderViewHost failed";
    return false;
  }

  content::WebPreferences web_preference =
      render_view_host->GetWebkitPreferences();
  bool enable = web_preference.media_subtitle_notification_enabled;
  LOG(INFO) << "subtitle_playback_notification_enabled:" << enable;
  return enable;
}

bool MediaPlayerEfl::PlaybackNotificationEnabled() {
  content::WebContents* web_contents = nullptr;
  content::RenderViewHost* render_view_host = nullptr;
  web_contents = static_cast<content::BrowserMediaPlayerManagerEfl*>(manager())
                     ->GetWebContents(GetPlayerId());
  if (!web_contents) {
    LOG(ERROR) << "GetWebContents failed";
    return false;
  }

  render_view_host = web_contents->GetRenderViewHost();
  if (!render_view_host) {
    LOG(ERROR) << "GetRenderViewHost failed";
    return false;
  }

  content::WebPreferences web_preference =
      render_view_host->GetWebkitPreferences();
  bool enable = web_preference.media_playback_notification_enabled;
  LOG(INFO) << "media_playback_notification_enabled:" << enable;
  return enable;
}

bool MediaPlayerEfl::CheckHighBitRate() {
  content::WebContentsDelegate* web_contents_delegate =
      GetWebContentsDelegate();
  if (!web_contents_delegate) {
    LOG(ERROR) << "get web_contents_delegate fail";
    return false;
  }
  bool ret = web_contents_delegate->IsHighBitRate();
  LOG(INFO) << "get high bit rate:" << (ret ? "true" : "false");
  return ret;
}
#endif

void MediaPlayerEfl::DeliverMediaPacket(ScopedMediaPacket packet) {
  tbm_surface_info_s suf_info = {
      0,
  };
  tbm_surface_h tbm_surface = nullptr;

  if (MEDIA_PACKET_ERROR_NONE !=
      media_packet_get_tbm_surface(packet.get(), &tbm_surface)) {
    LOG(ERROR) << "|media_packet_get_tbm_surface| failed";
    return;
  }

  if (TBM_SURFACE_ERROR_NONE != tbm_surface_get_info(tbm_surface, &suf_info)) {
    LOG(ERROR) << "|tbm_surface_get_info| failed";
    return;
  }

  if (width_ != static_cast<int>(suf_info.width) ||
      height_ != static_cast<int>(suf_info.height)) {
    width_ = static_cast<int>(suf_info.width);
    height_ = static_cast<int>(suf_info.height);
    media_type_ |= MediaType::Video;

    manager()->OnMediaDataChange(GetPlayerId(), width_, height_, media_type_);
  }

  uint64_t pts = 0;
  base::TimeDelta timestamp = GetCurrentTime();
  if (MEDIA_PACKET_ERROR_NONE == media_packet_get_pts(packet.get(), &pts)) {
    timestamp = base::TimeDelta::FromMicroseconds(
        pts / base::Time::kNanosecondsPerMicrosecond);
  } else {
    LOG(WARNING) << "Cannot read PTS from mediapacket."
                 << " Current position is used instead.";
    timestamp = GetCurrentTime();
  }

#if TIZEN_MM_DEBUG_VIDEO_PRINT_FPS == 1
  fpsCounter->CountFrame();
#endif
#if TIZEN_MM_DEBUG_VIDEO_DUMP_DECODED_FRAME == 1
  frameDumper->DumpFrame(&suf_info);
#endif

#if defined(TIZEN_MULTIMEDIA_TBM_ZEROCOPY_SUPPORT)
  gfx::TbmBufferHandle tbm_handle;
  tbm_handle.tbm_surface = tbm_surface;
  tbm_handle.media_packet = packet.release();
  manager()->OnNewTbmBufferAvailable(GetPlayerId(), tbm_handle, timestamp);
#else
  base::SharedMemory shared_memory;
  uint32 shared_memory_size =
      suf_info.planes[0].size + (suf_info.planes[0].size / 2);
  if (!shared_memory.CreateAndMapAnonymous(shared_memory_size)) {
    LOG(ERROR) << "Shared Memory creation failed.";
    return;
  }

  base::SharedMemoryHandle foreign_memory_handle =
      shared_memory.handle().Duplicate();
  if (!foreign_memory_handle.IsValid()) {
    LOG(ERROR) << "Shared Memory handle could not be obtained";
    return;
  }

  unsigned char* y_ptr = static_cast<unsigned char*>(shared_memory.memory());

  // Video format will always be converted to I420
  switch (suf_info.format) {
    case TBM_FORMAT_NV12: {
      unsigned char* u_ptr = y_ptr + suf_info.planes[0].size;
      unsigned char* v_ptr = u_ptr + (suf_info.planes[0].size / 4);
      libyuv::NV12ToI420(suf_info.planes[0].ptr, suf_info.planes[0].stride,
                         suf_info.planes[1].ptr, suf_info.planes[1].stride,
                         y_ptr, suf_info.planes[0].stride, u_ptr,
                         suf_info.planes[1].stride / 2, v_ptr,
                         suf_info.planes[1].stride / 2, suf_info.width,
                         suf_info.height);
      break;
    }
    case TBM_FORMAT_YUV420: {
      unsigned char* u_ptr = y_ptr + suf_info.planes[0].size;
      unsigned char* v_ptr = u_ptr + suf_info.planes[1].size;
      libyuv::I420Copy(
          suf_info.planes[0].ptr, suf_info.planes[0].stride,
          suf_info.planes[1].ptr, suf_info.planes[1].stride,
          suf_info.planes[2].ptr, suf_info.planes[2].stride, y_ptr,
          suf_info.planes[0].stride, u_ptr, suf_info.planes[1].stride, v_ptr,
          suf_info.planes[2].stride, suf_info.width, suf_info.height);
      break;
    }
    default: {
      NOTIMPLEMENTED();
      LOG(WARNING) << "Not supported format";
      return;
    }
  }
  manager()->OnNewFrameAvailable(GetPlayerId(), foreign_memory_handle,
                                 shared_memory_size, timestamp);
#endif
}

void MediaPlayerEfl::OnMediaError(MediaError err) {
  blink::WebMediaPlayer::NetworkState state =
      blink::WebMediaPlayer::kNetworkStateEmpty;

  switch (err) {
    case MEDIA_ERROR_FORMAT:
      state = blink::WebMediaPlayer::kNetworkStateFormatError;
      break;
    case MEDIA_ERROR_DECODE:
      state = blink::WebMediaPlayer::kNetworkStateDecodeError;
      break;
    case MEDIA_ERROR_NETWORK:
      state = blink::WebMediaPlayer::kNetworkStateNetworkError;
      break;
  }

  manager()->OnNetworkStateChange(GetPlayerId(), state);
  ReleaseDisplayLock();
}

void MediaPlayerEfl::OnPlayerSuspendRequest() const {
  manager_->OnPlayerSuspendRequest(GetPlayerId());
}

#if defined(OS_TIZEN_TV_PRODUCT)
void MediaPlayerEfl::OnPlayerResourceConflict() const {
  resource_conflict_event_ = true;
  manager_->OnPlayerResourceConflict(GetPlayerId());
}
#endif

#if defined(TIZEN_SOUND_FOCUS)
void MediaPlayerEfl::OnFocusAcquired() {
  LOG(INFO) << "[SFM] Focus Acquired :" << static_cast<void*>(this);
  // Don't resume for video.
  if ((media_type_ & MEDIA_VIDEO_MASK) == MEDIA_VIDEO_MASK)
    return;

  if (should_be_resumed_by_sound_focus_manager_) {
    should_be_resumed_by_sound_focus_manager_ = false;
    Play();

    // Notify to webcore that playing state is changed.
    manager()->OnPauseStateChange(GetPlayerId(), is_paused_);
  }
}

void MediaPlayerEfl::OnFocusReleased(bool resume_playing) {
  LOG(INFO) << "[SFM] Focus Released : " << static_cast<void*>(this);
  should_be_resumed_by_sound_focus_manager_ = resume_playing;
  Pause(false);

  // Notify to webcore that playing state is changed.
  manager()->OnPauseStateChange(GetPlayerId(), is_paused_);
}
#endif

void MediaPlayerEfl::EnteredFullscreen() {
  is_fullscreen_ = true;
}

void MediaPlayerEfl::ExitedFullscreen() {
  is_fullscreen_ = false;
}

}  // namespace media
