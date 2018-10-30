// Copyright 2016 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_TIZEN_VIDEO_PLANE_CONTROLLER_WINDOW_EFL_H_
#define MEDIA_BASE_TIZEN_VIDEO_PLANE_CONTROLLER_WINDOW_EFL_H_

#include "media/base/tizen/video_plane_manager_tizen.h"
#include "ui/gfx/geometry/rect_f.h"

namespace media {

class MEDIA_EXPORT VideoPlaneControllerWindowEfl
    : public VideoPlaneManagerTizen {
 public:
  VideoPlaneControllerWindowEfl(void* handle, MediaPlayerEfl* player);

  ~VideoPlaneControllerWindowEfl() override;

  static void* CreateVideoWindow(void* parentWindow);
  int Initialize() override;
  int SetVideoRect(const gfx::RectF&) override;
  void* GetVideoPlaneHandle() override;

 private:
  void* video_window_;

  DISALLOW_COPY_AND_ASSIGN(VideoPlaneControllerWindowEfl);
};

}  // namespace media

#endif  // MEDIA_BASE_TIZEN_VIDEO_PLANE_CONTROLLER_WINDOW_EFL_H_
