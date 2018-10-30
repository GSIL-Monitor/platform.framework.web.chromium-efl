// Copyright 2016 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_TIZEN_VIDEO_PLANE_MANAGER_TIZEN_H_
#define MEDIA_BASE_TIZEN_VIDEO_PLANE_MANAGER_TIZEN_H_

#include "media/base/media_export.h"
#include "ui/gfx/geometry/rect_f.h"

namespace media {

class MediaPlayerEfl;
enum WindowlessMode { WINDOWLESS_MODE_NORMAL, WINDOWLESS_MODE_FULL_SCREEN };

class MEDIA_EXPORT VideoPlaneManagerTizen {
 public:
  static VideoPlaneManagerTizen* Create(void* handle, MediaPlayerEfl* player);
  static VideoPlaneManagerTizen* Create(void* handle,
                                        MediaPlayerEfl* player,
                                        WindowlessMode mode);

  virtual ~VideoPlaneManagerTizen();

  virtual int Initialize() = 0;
  virtual void* GetVideoPlaneHandle() { return nullptr; }
  virtual int SetGeometry(const gfx::RectF& rect);

  void ApplyDeferredVideoRectIfNeeded();

  MediaPlayerEfl* GetMediaPlayer() const { return media_player_; }
  void* main_window_handle() const { return main_window_handle_; }
  void ResetGeometry();
  virtual void SetDisplayRotation() {}

 protected:
  VideoPlaneManagerTizen(void* handle, MediaPlayerEfl* player);

 private:
  gfx::RectF SetCropRatio(const gfx::Rect&, const gfx::RectF&);

  // If player_prepare_async is not complete, store the video rect.
  void SetDeferredVideoRect(const gfx::RectF&);

  virtual int SetVideoRect(const gfx::RectF&) = 0;
  virtual int PlayerSetCropRatio(const gfx::RectF& rect) { return 0; }
  virtual bool IsAvailableSetGeometry() { return true; }

  void* main_window_handle_;
  MediaPlayerEfl* media_player_;
  bool should_crop_video_;
  gfx::RectF deferred_video_rect_;
  gfx::RectF current_rect_;
};

}  // namespace media

#endif  // MEDIA_BASE_TIZEN_VIDEO_PLANE_MANAGER_TIZEN_H_
