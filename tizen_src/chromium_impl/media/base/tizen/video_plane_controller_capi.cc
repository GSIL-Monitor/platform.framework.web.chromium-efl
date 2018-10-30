// Copyright 2016 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/tizen/video_plane_controller_capi.h"

#include "media/base/tizen/media_player_efl.h"
#include "tizen_src/ewk/efl_integration/common/application_type.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"

#include <Evas.h>

#if defined(OS_TIZEN_TV_PRODUCT)
#include <player_product.h>
#endif

namespace media {

// static
VideoPlaneManagerTizen* VideoPlaneManagerTizen::Create(void* handle,
                                                       MediaPlayerEfl* player,
                                                       WindowlessMode mode) {
  LOG(INFO) << "VideoPlaneControllerCapi handle=" << handle;
  return new VideoPlaneControllerCapi(handle, player, mode);
}

VideoPlaneControllerCapi::VideoPlaneControllerCapi(void* handle,
                                                   MediaPlayerEfl* player,
                                                   WindowlessMode mode)
    : VideoPlaneManagerTizen(handle, player),
      player_handle_(NULL),
      windowless_mode_(mode) {}

VideoPlaneControllerCapi::~VideoPlaneControllerCapi() {}

player_h VideoPlaneControllerCapi::GetPlayerHandle(
    MediaPlayerEfl* media_player) const {
  return media_player->player_;
}

int VideoPlaneControllerCapi::Initialize() {
  if (!GetMediaPlayer()) {
    LOG(ERROR) << "failed: MediaPlayer is null!";
    return PLAYER_ERROR_INVALID_OPERATION;
  }
  player_handle_ = GetPlayerHandle(GetMediaPlayer());
  int ret = PLAYER_ERROR_NONE;

#if defined(OS_TIZEN_TV_PRODUCT)
  ret = player_set_display_mode(player_handle_, PLAYER_DISPLAY_MODE_DST_ROI);

  if (ret != PLAYER_ERROR_NONE)
    return ret;

  // init video size
  gfx::RectF init_rect(0.0f, 0.0f, 1.0f, 1.0f);
  ret = SetVideoRect(init_rect);
#endif

  return ret;
}

int VideoPlaneControllerCapi::SetVideoRect(const gfx::RectF& rect) {
  int ret = PLAYER_ERROR_NONE;
#if defined(OS_TIZEN_TV_PRODUCT)
  ret = player_set_display_roi_area(player_handle_, rect.x(), rect.y(),
                                    rect.width(), rect.height());
#endif
#if defined(TIZEN_VD_MULTIPLE_MIXERDECODER)
  if (content::IsWebBrowser())
    return ret;
  int mix_ret = player_video_mixer_set_displayarea(
      player_handle_, 0, 0, rect.x(), rect.y(), rect.width(), rect.height());
  LOG(INFO) << " mixer_displayarea : (" << rect.x() << ", " << rect.y() << ", "
            << rect.width() << ", " << rect.height() << ")";
  if (mix_ret != PLAYER_ERROR_NONE)
    LOG(ERROR) << "player_video_mixer_set_displayarea, ret =" << mix_ret;
#endif

  return ret;
}

int VideoPlaneControllerCapi::PlayerSetCropRatio(const gfx::RectF& rect) {
  int ret = PLAYER_ERROR_NONE;
#if defined(OS_TIZEN_TV_PRODUCT)
  LOG(INFO) << "crop_ratio : (" << rect.x() << ", " << rect.y() << ", "
            << rect.width() << ", " << rect.height() << ")";

  ret = player_set_crop_ratio(player_handle_, rect.x(), rect.y(), rect.width(),
                              rect.height());
#endif
  return ret;
}

int VideoPlaneControllerCapi::SetGeometry(const gfx::RectF& rect) {
  if (!GetMediaPlayer()) {
    LOG(ERROR) << "failed: MediaPlayer is null!";
    return PLAYER_ERROR_INVALID_OPERATION;
  }
  player_handle_ = GetPlayerHandle(GetMediaPlayer());
  return VideoPlaneManagerTizen::SetGeometry(rect);
}

player_state_e VideoPlaneControllerCapi::GetPlayerState() {
  player_state_e state = PLAYER_STATE_NONE;
  player_get_state(player_handle_, &state);
  return state;
}

bool VideoPlaneControllerCapi::IsAvailableSetGeometry() {
  return (GetPlayerState() >= PLAYER_STATE_READY);
}

void VideoPlaneControllerCapi::SetDisplayRotation() {
  player_display_rotation_e screen_rotation = ConvertToPlayerDisplayRotation(
      display::Screen::GetScreen()->GetPrimaryDisplay().RotationAsDegree());

  player_display_rotation_e rotation = PLAYER_DISPLAY_ROTATION_NONE;
  int ret = player_get_display_rotation(player_handle_, &rotation);
  if (ret == PLAYER_ERROR_NONE || screen_rotation != rotation) {
    LOG(INFO) << "video should rotate angle : " << screen_rotation;
#if defined(TIZEN_VD_MULTIPLE_MIXERDECODER)
    if (!content::IsWebBrowser()) {
      player_video_mixer_set_position_of_mixedframe(
          player_handle_, 0, 0, DEPRECATED_PARAM, MIXER_SCREEN_WIDTH,
          MIXER_SCREEN_HEIGHT);
      ret = player_video_mixer_set_rotation_degree_of_mixedframe(
          player_handle_, 0, screen_rotation);
      if (ret != PLAYER_ERROR_NONE) {
        LOG(ERROR)
            << "player_video_mixer_set_rotation_degree_of_mixedframe, ret ="
            << ret;
      }
    }
#endif
    ret = player_set_display_rotation(player_handle_, screen_rotation);
    if (ret != PLAYER_ERROR_NONE) {
      LOG(ERROR) << "Cannot set display rotation : "
                 << GetString(static_cast<player_error_e>(ret));
    }
  }
}

void* VideoPlaneControllerCapi::GetVideoPlaneHandle() {
  if (!main_window_handle())
    LOG(ERROR) << "failed: main window is null!";

  return main_window_handle();
}

}  // namespace media
