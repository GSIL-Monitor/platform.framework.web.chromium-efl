// Copyright 2016 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_TIZEN_VIDEO_PLANE_CONTROLLER_CAPI_H_
#define MEDIA_BASE_TIZEN_VIDEO_PLANE_CONTROLLER_CAPI_H_

#include <player.h>

#include "media/base/tizen/video_plane_manager_tizen.h"

namespace media {

class MEDIA_EXPORT VideoPlaneControllerCapi : public VideoPlaneManagerTizen {
 public:
  VideoPlaneControllerCapi(void* handle,
                           MediaPlayerEfl* player,
                           WindowlessMode mode);

  ~VideoPlaneControllerCapi() override;

  int Initialize() override;
  void* GetVideoPlaneHandle() override;
  int SetGeometry(const gfx::RectF& rect) override;
  player_h GetPlayerHandle(MediaPlayerEfl* media_player) const;
  void SetDisplayRotation() override;

 private:
  player_state_e GetPlayerState();
  int SetVideoRect(const gfx::RectF&) override;
  int PlayerSetCropRatio(const gfx::RectF& rect) override;
  bool IsAvailableSetGeometry() override;

  player_h player_handle_;
  WindowlessMode windowless_mode_;
  DISALLOW_COPY_AND_ASSIGN(VideoPlaneControllerCapi);
};

}  // namespace media

#endif  // MEDIA_BASE_TIZEN_VIDEO_PLANE_CONTROLLER_CAPI_H_
