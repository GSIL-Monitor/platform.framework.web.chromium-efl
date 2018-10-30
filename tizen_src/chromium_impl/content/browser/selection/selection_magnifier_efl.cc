// Copyright 2013 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "selection_magnifier_efl.h"

#include <Elementary.h>

#include "base/files/file_path.h"
#include "base/path_service.h"
#include "content/browser/renderer_host/render_widget_host_view_efl.h"
#include "content/browser/selection/selection_controller_efl.h"
#include "content/common/paths_efl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/web_preferences.h"

namespace content {

// Inherited from WK2 w.r.t tizen style
const int kHeightOffset = 60;
const float kZoomScale = 0.66;

SelectionMagnifierEfl::SelectionMagnifierEfl(
    content::SelectionControllerEfl* controller)
    : controller_(controller), content_image_(0), shown_(false) {
  base::FilePath edj_dir;
  base::FilePath magnifier_edj;
  PathService::Get(PathsEfl::EDJE_RESOURCE_DIR, &edj_dir);
  magnifier_edj = edj_dir.Append(FILE_PATH_LITERAL("Magnifier.edj"));

  container_ = elm_layout_add(controller_->rwhv()->content_image());
  elm_layout_file_set(container_, magnifier_edj.AsUTF8Unsafe().c_str(),
                      "magnifier");
  edje_object_part_geometry_get(elm_layout_edje_get(container_), "bg", 0, 0,
                                &width_, &height_);
}

SelectionMagnifierEfl::~SelectionMagnifierEfl() {
  DisconnectCallbacks();
  if (content_image_) {
    evas_object_del(content_image_);
    content_image_ = 0;
  }
  if (container_)
    evas_object_del(container_);
}

void SelectionMagnifierEfl::HandleLongPress(const gfx::Point& touch_point,
                                            bool is_in_edit_field) {
  evas_object_event_callback_add(controller_->rwhv()->content_image(),
                                 EVAS_CALLBACK_MOUSE_MOVE, OnAnimatorMove,
                                 this);
  evas_object_event_callback_add(controller_->rwhv()->content_image(),
                                 EVAS_CALLBACK_MOUSE_UP, OnAnimatorUp, this);
  // Call OnAnimatorMove here, so that the magnifier widget
  // gets properly placed. Other calls to it are expected to
  // happen as a react to finger/mouse movements.
  if (!is_in_edit_field && !controller_->HandleBeingDragged()) {
    OnAnimatorMove();
    Show();
  }
}

void SelectionMagnifierEfl::DisconnectCallbacks() {
  evas_object_event_callback_del(controller_->rwhv()->content_image(),
                                 EVAS_CALLBACK_MOUSE_MOVE, OnAnimatorMove);
  evas_object_event_callback_del(controller_->rwhv()->content_image(),
                                 EVAS_CALLBACK_MOUSE_UP, OnAnimatorUp);
}

void SelectionMagnifierEfl::OnAnimatorMove(void* data,
                                           Evas*,
                                           Evas_Object*,
                                           void*) {
  auto magnifier_data = static_cast<SelectionMagnifierEfl*>(data);
  DCHECK(magnifier_data);

  if (magnifier_data)
    magnifier_data->OnAnimatorMove();
}

void SelectionMagnifierEfl::OnAnimatorMove() {
  Evas_Coord_Point point;
  evas_pointer_canvas_xy_get(
      evas_object_evas_get(controller_->rwhv()->content_image()), &point.x,
      &point.y);
  gfx::Point display_point(point.x, point.y);
  controller_->HandleLongPressMoveEvent(display_point);

  if (controller_->GetVisibleViewportRect().Contains(display_point)) {
    UpdateLocation(display_point);
    Move(display_point);
  }
}

void SelectionMagnifierEfl::OnAnimatorUp(void* data,
                                         Evas*,
                                         Evas_Object*,
                                         void*) {
  auto magnifier_data = static_cast<SelectionMagnifierEfl*>(data);
  DCHECK(magnifier_data);

  if (magnifier_data)
    magnifier_data->OnAnimatorUp();
}

void SelectionMagnifierEfl::OnAnimatorUp() {
  DisconnectCallbacks();
  Hide();
  controller_->HandleLongPressEndEvent();
}

void SelectionMagnifierEfl::MagnifierGetSnapshotCb(Evas_Object* image,
                                                   void* user_data) {
  auto selection_magnifier = static_cast<SelectionMagnifierEfl*>(user_data);

  if (selection_magnifier->content_image_) {
    evas_object_del(selection_magnifier->content_image_);
    selection_magnifier->content_image_ = 0;
  }

  selection_magnifier->content_image_ = image;
  evas_object_show(selection_magnifier->content_image_);

  elm_object_part_content_set(selection_magnifier->container_, "swallow",
                              selection_magnifier->content_image_);
  evas_object_pass_events_set(selection_magnifier->content_image_, EINA_TRUE);
// TODO(soorya.r) : Handle clipping |content_image_| when its size is greater
// than |container_|. |container_| should be of type image/rectangle object.
#if !defined(EWK_BRINGUP)
  evas_object_clip_set(selection_magnifier->content_image_,
      selection_magnifier->container_);
#endif

  evas_object_layer_set(selection_magnifier->container_, EVAS_LAYER_MAX);
  evas_object_layer_set(selection_magnifier->content_image_, EVAS_LAYER_MAX);
}

void SelectionMagnifierEfl::UpdateLocation(const gfx::Point& location) {
  auto visible_viewport_rect = controller_->GetVisibleViewportRect();
  int visible_viewport_x = visible_viewport_rect.x();
  int visible_viewport_y = visible_viewport_rect.y();
  int visible_viewport_width = visible_viewport_rect.width();
  int visible_viewport_height = visible_viewport_rect.height();

  int zoomedWidth = width_ * kZoomScale;
  int zoomedHeight = height_ * kZoomScale;

  if (zoomedWidth > visible_viewport_width)
    zoomedWidth = visible_viewport_width;

  if (zoomedHeight > visible_viewport_height)
    zoomedHeight = visible_viewport_height;

  // Place screenshot area center on touch point
  gfx::Rect content_rect(location.x() - zoomedWidth / 2,
                         location.y() - zoomedHeight / 2, zoomedWidth,
                         zoomedHeight);

  // Move magnifier rect so it stays inside webview area
  if (content_rect.x() < visible_viewport_x) {
    content_rect.set_x(visible_viewport_x);
  } else if ((content_rect.x() + zoomedWidth) >=
             visible_viewport_x + visible_viewport_width) {
    content_rect.set_x(visible_viewport_x + visible_viewport_width -
                       zoomedWidth - 1);
  }

  if (content_rect.y() < visible_viewport_y) {
    content_rect.set_y(visible_viewport_y);
  } else if ((content_rect.y() + zoomedHeight) >=
             visible_viewport_y + visible_viewport_height) {
    content_rect.set_y(visible_viewport_y + visible_viewport_height -
                       zoomedHeight - 1);
  }

  // Convert to webview parameter space
  Eina_Rectangle rect;
  rect.x = content_rect.x() - visible_viewport_x;
  rect.y = content_rect.y() - visible_viewport_y;
  rect.w = content_rect.width();
  rect.h = content_rect.height();

  controller_->rwhv()->RequestMagnifierSnapshotAsync(
      rect, MagnifierGetSnapshotCb, this);
}

void SelectionMagnifierEfl::Move(const gfx::Point& location) {
  auto visible_viewport_rect = controller_->GetVisibleViewportRect();
  int visible_viewport_x = visible_viewport_rect.x();
  int visible_viewport_y = visible_viewport_rect.y();
  int visible_viewport_width = visible_viewport_rect.width();
  int visible_viewport_height = visible_viewport_rect.height();

  int magnifier_x = location.x();
  int magnifier_y = location.y() - height_ - kHeightOffset;

  // Do not let location to be out of screen
  if (magnifier_y < visible_viewport_y) {
    magnifier_y = visible_viewport_y;
  } else if (magnifier_y >
             visible_viewport_height + visible_viewport_y - height_) {
    magnifier_y = visible_viewport_height + visible_viewport_y - height_;
  }
  // Do not let location to be out of screen
  if (magnifier_x < visible_viewport_x + width_ / 2) {
    magnifier_x = visible_viewport_x + width_ / 2;
  } else if (magnifier_x >
             visible_viewport_width + visible_viewport_x - width_ / 2) {
    magnifier_x = visible_viewport_width + visible_viewport_x - width_ / 2;
  }

  evas_object_move(container_, magnifier_x, magnifier_y);
}

void SelectionMagnifierEfl::Show() {
  RenderViewHost* render_view_host =
      controller_->web_contents()->GetRenderViewHost();
  if (!render_view_host ||
      !render_view_host->GetWebkitPreferences().selection_magnifier_enabled) {
    return;
  }
  shown_ = true;
  evas_object_show(container_);

  controller_->rwhv()->set_magnifier(true);
  if (controller_->rwhv()->ewk_view()) {
    evas_object_smart_callback_call(controller_->rwhv()->ewk_view(),
                                    "magnifier,show", nullptr);
  }
}

void SelectionMagnifierEfl::Hide() {
  if (!shown_)
    return;

  shown_ = false;
  evas_object_hide(content_image_);
  evas_object_hide(container_);

  controller_->rwhv()->set_magnifier(false);
  if (controller_->rwhv()->ewk_view()) {
    evas_object_smart_callback_call(controller_->rwhv()->ewk_view(),
                                    "magnifier,hide", nullptr);
  }
}

}
