// Copyright 2016 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/tizen/video_plane_manager_tizen.h"

#include "media/base/tizen/media_player_efl.h"

#if defined(OS_TIZEN_TV_PRODUCT)
#include "tizen_src/ewk/efl_integration/common/application_type.h"
#include "ui/display/device_display_info_efl.h"
#endif

#include <player.h>

namespace media {

VideoPlaneManagerTizen::VideoPlaneManagerTizen(void* handle,
                                               MediaPlayerEfl* player)
    : main_window_handle_(handle),
      media_player_(player),
      should_crop_video_(false) {}

VideoPlaneManagerTizen::~VideoPlaneManagerTizen() {}

gfx::RectF VideoPlaneManagerTizen::SetCropRatio(const gfx::Rect& viewport_rect,
                                                const gfx::RectF& video_rect) {
  if (!GetMediaPlayer()) {
    LOG(ERROR) << "failed: MediaPlayer is null!";
    return gfx::RectF();
  }
  gfx::RectF view_rect =
      gfx::RectF(viewport_rect.x(), viewport_rect.y(), viewport_rect.width(),
                 viewport_rect.height());

  // Offset the position of the video to fit the screen.
  // Because of the player(MM) use the based on the
  // screen(EFL window) coordinates, the offset this.
  gfx::RectF screen_video_rect = video_rect;
  screen_video_rect.Offset(view_rect.x(), view_rect.y());

  gfx::RectF visible_rect = screen_video_rect;
  visible_rect.Intersect(view_rect);

  int ret = PLAYER_ERROR_NONE;

  if (visible_rect.size() == screen_video_rect.size() ||
      visible_rect.IsEmpty()) {
    if (should_crop_video_) {
      ret = PlayerSetCropRatio(gfx::RectF(0.0f, 0.0f, 1.0f, 1.0f));
      if (ret != PLAYER_ERROR_NONE)
        return gfx::RectF();
    }
    should_crop_video_ = false;

    if (visible_rect.IsEmpty())
      return gfx::RectF(view_rect.x(), view_rect.y(), 1, 1);
    return screen_video_rect;
  }

  gfx::RectF cropratio_rect = screen_video_rect;
  cropratio_rect.set_x(screen_video_rect.x() < visible_rect.x()
                           ? abs(screen_video_rect.x() - visible_rect.x()) /
                                 screen_video_rect.width()
                           : 0.0);

  cropratio_rect.set_width(
      screen_video_rect.width() > visible_rect.width()
          ? 1.0 - abs(screen_video_rect.width() - visible_rect.width()) /
                      screen_video_rect.width()
          : 1.0);

  cropratio_rect.set_y(screen_video_rect.y() < visible_rect.y()
                           ? abs(screen_video_rect.y() - visible_rect.y()) /
                                 screen_video_rect.height()
                           : 0.0);

  cropratio_rect.set_height(
      screen_video_rect.height() > visible_rect.height()
          ? 1.0 - abs(screen_video_rect.height() - visible_rect.height()) /
                      screen_video_rect.height()
          : 1.0);

  ret = PlayerSetCropRatio(cropratio_rect);
  if (ret != PLAYER_ERROR_NONE)
    return gfx::RectF();

  should_crop_video_ = true;

  return visible_rect;
}

void VideoPlaneManagerTizen::SetDeferredVideoRect(const gfx::RectF& rect) {
  if (!GetMediaPlayer()) {
    LOG(ERROR) << "failed: MediaPlayer is null!";
    return;
  }

  deferred_video_rect_ = rect;
}

void VideoPlaneManagerTizen::ApplyDeferredVideoRectIfNeeded() {
  MediaPlayerEfl* player = GetMediaPlayer();

  if (!player) {
    LOG(ERROR) << "failed: MediaPlayer is null!";
    return;
  }

  if (!deferred_video_rect_.IsEmpty()) {
    SetGeometry(deferred_video_rect_);
    deferred_video_rect_ = gfx::RectF();
  }
}

int VideoPlaneManagerTizen::SetGeometry(const gfx::RectF& rect) {
  int ret = PLAYER_ERROR_NONE;
  SetDisplayRotation();
#if defined(OS_TIZEN_TV_PRODUCT)
  if (rect.IsEmpty()) {
    LOG(INFO) << "skip: rect is empty!";
    return ret;
  }

  current_rect_ = rect;

  if (!IsAvailableSetGeometry()) {
    LOG(INFO) << "store the video rect because of player was not ready.";
    SetDeferredVideoRect(rect);
    return ret;
  }

  LOG(INFO) << "video_geometry(" << rect.x() << ", " << rect.y() << ", "
            << rect.width() << ", " << rect.height() << ")";

  gfx::RectF display_rect =
      SetCropRatio(GetMediaPlayer()->GetViewportRect(), rect);

#if defined(OS_TIZEN_TV_PRODUCT)
  if (content::IsHbbTV()) {
    display::DeviceDisplayInfoEfl display_info;
    if (!GetMediaPlayer()->GetViewportRect().IsEmpty())
      display_rect.Scale(
          (float)display_info.GetDisplayWidth() /
              (float)(GetMediaPlayer()->GetViewportRect()).width(),
          (float)display_info.GetDisplayHeight() /
              (float)(GetMediaPlayer()->GetViewportRect()).height());
  }
#endif
  if (display_rect.IsEmpty()) {
    LOG(INFO) << "skip: crop rect is empty!";
    return ret;
  }

  LOG(INFO) << "display_video_geometry(" << display_rect.x() << ", "
            << display_rect.y() << ", " << display_rect.width() << ", "
            << display_rect.height() << ")";

  ret = SetVideoRect(display_rect);
#endif
  return ret;
}

void VideoPlaneManagerTizen::ResetGeometry() {
  SetGeometry(current_rect_);
}

}  // namespace media
