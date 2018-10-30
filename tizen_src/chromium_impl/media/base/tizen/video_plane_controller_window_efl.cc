// Copyright 2016 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/tizen/video_plane_controller_window_efl.h"

#include <Elementary.h>
#include <Evas.h>
#include <player.h>

namespace media {

// static
VideoPlaneManagerTizen* VideoPlaneManagerTizen::Create(void* handle,
                                                       MediaPlayerEfl* player) {
  LOG(INFO) << "createVideoPlaneController handle=" << handle;
  return new VideoPlaneControllerWindowEfl(handle, player);
}

VideoPlaneControllerWindowEfl::VideoPlaneControllerWindowEfl(
    void* handle,
    MediaPlayerEfl* player)
    : VideoPlaneManagerTizen(handle, player), video_window_(nullptr) {
  video_window_ = CreateVideoWindow(main_window_handle());

  if (!video_window_) {
    LOG(ERROR) << "failed: video window is null!";
  }
}

VideoPlaneControllerWindowEfl::~VideoPlaneControllerWindowEfl() {
  if (video_window_)
    evas_object_del(static_cast<Evas_Object*>(video_window_));
}

// static
void* VideoPlaneControllerWindowEfl::CreateVideoWindow(void* parentWindow) {
  if (!parentWindow) {
    LOG(ERROR) << "createVideoWindow: parent window is null";
    return nullptr;
  }

  // Does not set transient relation(child window).
  Evas_Object* sharedVideoWindow =
      elm_win_add(nullptr, "BlinkVideo", ELM_WIN_BASIC);

  if (!sharedVideoWindow) {
    LOG(ERROR) << "createVideoWindow: video window is null";
    return nullptr;
  }

  elm_win_title_set(sharedVideoWindow, "BlinkVideo");
  elm_win_borderless_set(sharedVideoWindow, EINA_TRUE);

  elm_win_alpha_set(static_cast<Elm_Win*>(parentWindow), true);
  elm_win_alpha_set(sharedVideoWindow, true);

  Evas_Object* bg = evas_object_rectangle_add(sharedVideoWindow);
  evas_object_color_set(bg, 0, 0, 0, 0);  // red=0, green=0, blue=0, alpha=0

  elm_win_aux_hint_add(sharedVideoWindow, "wm.policy.win.user.geometry", "1");

  LOG(INFO) << "createVideoWindow video=" << sharedVideoWindow;
  LOG(INFO) << "createVideoWindow main=" << parentWindow;

  return static_cast<void*>(sharedVideoWindow);
}

int VideoPlaneControllerWindowEfl::Initialize() {
  if (!video_window_) {
    LOG(ERROR) << "failed: video window is null!";
    return PLAYER_ERROR_INVALID_OPERATION;
  }

  LOG(INFO) << "InitializeVideoWindow: sharedVideoWin=" << video_window_;

  Evas_Object* videoWindow = static_cast<Evas_Object*>(video_window_);

  evas_object_resize(videoWindow, 1, 1);  // width=1, height=1
  evas_object_move(videoWindow, 0, 0);    // x=0, y=0

  evas_object_show(videoWindow);
  elm_win_raise(static_cast<Elm_Win*>(video_window_));
  elm_win_raise(static_cast<Elm_Win*>(main_window_handle()));

  return PLAYER_ERROR_NONE;
}

int VideoPlaneControllerWindowEfl::SetVideoRect(const gfx::RectF& rect) {
  if (!video_window_) {
    LOG(ERROR) << "failed: video window is null!";
    return PLAYER_ERROR_INVALID_OPERATION;
  }

  Evas_Object* videoWindow = static_cast<Evas_Object*>(video_window_);

  evas_object_move(videoWindow, rect.x(), rect.y());
  evas_object_resize(videoWindow, rect.width(), rect.height());
  return PLAYER_ERROR_NONE;
}

void* VideoPlaneControllerWindowEfl::GetVideoPlaneHandle() {
  if (!video_window_)
    LOG(ERROR) << "failed: video window is null!";

  return video_window_;
}

}  // namespace media
