// Copyright 2013 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "selection_handle_efl.h"

#include <Edje.h>
#include <Eina.h>
#include <Elementary.h>

#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/trace_event/trace_event.h"
#include "content/browser/renderer_host/render_widget_host_view_efl.h"
#include "content/browser/selection/selection_controller_efl.h"
#include "content/common/paths_efl.h"

namespace content {

// Size of the left/right margin which causes selection handles to be rotated
const int kReverseMargin = 32;
const char* kLeftHandlePath = "elm/entry/selection/block_handle_left";
const char* kRightHandlePath = "elm/entry/selection/block_handle_right";
const char* kInputHandlePath = "elm/entry/cursor_handle/default";

static const char* GetEdjeObjectGroupPath(SelectionHandleEfl::HandleType type) {
  switch (type) {
    case SelectionHandleEfl::HANDLE_TYPE_LEFT:
      return kLeftHandlePath;
    case SelectionHandleEfl::HANDLE_TYPE_RIGHT:
      return kRightHandlePath;
    case SelectionHandleEfl::HANDLE_TYPE_INPUT:
      return kInputHandlePath;
    default:
      return NULL;
  }
}

SelectionHandleEfl::SelectionHandleEfl(SelectionControllerEfl& controller,
                                       HandleType type)
    : controller_(controller),
      pressed_(false),
      is_top_(false),
      handle_type_(type) {
  Evas* evas = evas_object_evas_get(controller_.rwhv()->content_image());
  handle_ = edje_object_add(evas);

  const char* group = GetEdjeObjectGroupPath(type);

  bool theme_found = false;
  const Eina_List* default_theme_list = elm_theme_list_get(0);
  const Eina_List* l;
  void* theme;

  // Walk default platform theme paths for theme containing custom handles.
  EINA_LIST_FOREACH(default_theme_list, l, theme) {
    char* theme_path =
        elm_theme_list_item_path_get(static_cast<const char*>(theme), 0);
    if (theme_path) {
      if (edje_file_group_exists(theme_path, group)) {
        edje_object_file_set(handle_, theme_path, group);
        theme_found = true;
        free(theme_path);
        break;
      }
      free(theme_path);
    }
  }

  // No platform theme with custom handles, use fallback style.
  if (!theme_found) {
    base::FilePath edj_dir;
    PathService::Get(PathsEfl::EDJE_RESOURCE_DIR, &edj_dir);
    base::FilePath selection_handles_edj =
        edj_dir.Append(FILE_PATH_LITERAL("SelectionHandles.edj"));

    std::string theme_path = selection_handles_edj.AsUTF8Unsafe();
    if (edje_file_group_exists(theme_path.c_str(), group))
      edje_object_file_set(handle_, theme_path.c_str(), group);
  }

  edje_object_signal_emit(handle_, "edje,focus,in", "edje");
  edje_object_signal_emit(handle_, "elm,state,bottom", "elm");
  evas_object_smart_member_add(handle_, controller_.rwhv()->smart_parent());
  evas_object_propagate_events_set(handle_, false);

  evas_object_event_callback_add(handle_, EVAS_CALLBACK_MOUSE_DOWN, OnMouseDown, this);
  evas_object_event_callback_add(handle_, EVAS_CALLBACK_MOUSE_UP, OnMouseUp, this);
}

SelectionHandleEfl::~SelectionHandleEfl() {
  evas_object_event_callback_del(handle_, EVAS_CALLBACK_MOUSE_DOWN, OnMouseDown);
  evas_object_event_callback_del(handle_, EVAS_CALLBACK_MOUSE_UP, OnMouseUp);
  evas_object_smart_member_del(handle_);
  evas_object_del(handle_);
}

void SelectionHandleEfl::Show() {
  evas_object_show(handle_);
  evas_object_raise(handle_);
}

void SelectionHandleEfl::Hide() {
  evas_object_hide(handle_);
}

bool SelectionHandleEfl::IsVisible() const {
  int handle_x, handle_y;
  evas_object_geometry_get(handle_, &handle_x, &handle_y, nullptr, nullptr);

  auto view_rect = controller_.GetVisibleViewportRect();

  return evas_object_visible_get(handle_) &&
         view_rect.Contains(handle_x, handle_y);
}

void SelectionHandleEfl::Move(const gfx::Point& point) {
  if (!pressed_)
    ChangeObjectDirection(CalculateDirection(point));

  if (handle_type_ == HANDLE_TYPE_INPUT)
    MoveInputHandle(point);
  else
    MoveRangeHandle(point);
}

void SelectionHandleEfl::MoveInputHandle(const gfx::Point& point) {
  CHECK(handle_type_ == HANDLE_TYPE_INPUT);
  MoveObject(point);
}

void SelectionHandleEfl::MoveRangeHandle(const gfx::Point& point) {
  CHECK(handle_type_ != HANDLE_TYPE_INPUT);
  if (!pressed_)
    MoveObject(point);
}

void SelectionHandleEfl::OnMouseDown(void* data, Evas*, Evas_Object*, void* event_info) {
  Evas_Event_Mouse_Down* event = static_cast<Evas_Event_Mouse_Down*>(event_info);
  SelectionHandleEfl* handle = static_cast<SelectionHandleEfl*>(data);
  handle->pressed_ = true;
  //Save the diff_point between touch point and base point of the handle
  handle->diff_point_.SetPoint(event->canvas.x - handle->base_point_.x(),
      event->canvas.y - handle->base_point_.y());

  handle->controller_.HandleDragBeginNotification(handle);
}

void SelectionHandleEfl::UpdatePosition(const gfx::Point& position) {
  current_touch_point_ = position;
  ecore_job_add(UpdateMouseMove, this);
}

void SelectionHandleEfl::OnMouseUp(void* data, Evas*, Evas_Object*, void* /* event_info */) {
  SelectionHandleEfl* handle = static_cast<SelectionHandleEfl*>(data);
  handle->pressed_ = false;
  handle->controller_.HandleDragEndNotification();
}

void SelectionHandleEfl::UpdateMouseMove(void* data) {
  SelectionHandleEfl* handle = static_cast<SelectionHandleEfl*>(data);

  // As this method is queued and then precessed, it might happen that
  // it gets called once user has lifted up its finger.
  // Bail out in such cases.
  if (!handle->pressed_)
    return;

  int delta_x = handle->current_touch_point_.x() - handle->diff_point_.x();
  int delta_y = handle->current_touch_point_.y() - handle->diff_point_.y();
  handle->base_point_.SetPoint(delta_x, delta_y);

  handle->controller_.HandleDragUpdateNotification(handle);
}

SelectionHandleEfl::HandleDirection SelectionHandleEfl::CalculateDirection(
    const gfx::Point& point) const {
  bool reverse_horizontally = false, reverse_vertically = false;
  int handleHeight;
  edje_object_part_geometry_get(handle_, "handle", 0, 0, 0, &handleHeight);

  auto visible_viewport_rect = controller_.GetVisibleViewportRect();
  gfx::Point conv_point(point.x() + visible_viewport_rect.x(),
                        point.y() + visible_viewport_rect.y());

  gfx::Rect reverse_direction_rect = gfx::Rect(
      visible_viewport_rect.x() + kReverseMargin, visible_viewport_rect.y(),
      visible_viewport_rect.width() - 2 * kReverseMargin,
      visible_viewport_rect.height() - handleHeight);

  if (!reverse_direction_rect.Contains(conv_point)) {
    if (conv_point.x() <= reverse_direction_rect.x() ||
        conv_point.x() >= reverse_direction_rect.right()) {
      reverse_vertically = true;
    }
    if (conv_point.y() <= reverse_direction_rect.y() ||
        conv_point.y() >= reverse_direction_rect.bottom()) {
      reverse_horizontally = true;
    }
  }

  if (handle_type_ != HANDLE_TYPE_INPUT) {
    if (reverse_vertically) {
      if (reverse_horizontally)
        return DirectionTopReverse;
      else
        return DirectionBottomReverse;
    } else {
      if (reverse_horizontally)
        return DirectionTopNormal;
      else
        return DirectionBottomNormal;
    }
  } else {
    // Input handle can only be rotated horizontally
    if (reverse_horizontally)
      return DirectionTopNormal;
    else
      return DirectionBottomNormal;
  }

  NOTREACHED();
  return DirectionBottomNormal;
}

void SelectionHandleEfl::ChangeObjectDirection(HandleDirection direction) {
  TRACE_EVENT2("selection,efl", __PRETTY_FUNCTION__,
               "handle type", handle_type_, "direction", direction);

  is_top_ = (direction == DirectionTopNormal || direction == DirectionTopReverse);

  switch (direction) {
    case DirectionBottomNormal:
      if (handle_type_ == HANDLE_TYPE_INPUT)
        edje_object_signal_emit(handle_, "edje,cursor,handle,show", "edje");
      else
        edje_object_signal_emit(handle_, "elm,state,bottom", "elm");
      break;
    case DirectionBottomReverse:
      if (handle_type_ == HANDLE_TYPE_INPUT)
        edje_object_signal_emit(handle_, "edje,cursor,handle,show", "edje");
      else
        edje_object_signal_emit(handle_, "elm,state,bottom,reversed", "elm");
      break;
    case DirectionTopNormal:
      if (handle_type_ == HANDLE_TYPE_INPUT)
        edje_object_signal_emit(handle_, "edje,cursor,handle,top", "edje");
      else
        edje_object_signal_emit(handle_, "elm,state,top", "elm");
      break;
    case DirectionTopReverse:
      if (handle_type_ == HANDLE_TYPE_INPUT)
        edje_object_signal_emit(handle_, "edje,cursor,handle,top", "edje");
      else
        edje_object_signal_emit(handle_, "elm,state,top,reversed", "elm");
      break;
  }

  switch (handle_type_) {
    case HANDLE_TYPE_LEFT:
      evas_object_smart_callback_call(controller_.rwhv()->ewk_view(),
                                      "selection,handle,left,direction",
                                      &direction);
      break;
    case HANDLE_TYPE_RIGHT:
      evas_object_smart_callback_call(controller_.rwhv()->ewk_view(),
                                      "selection,handle,right,direction",
                                      &direction);
      break;
    case HANDLE_TYPE_INPUT:
      evas_object_smart_callback_call(controller_.rwhv()->ewk_view(),
                                      "selection,handle,large,direction",
                                      &direction);
      break;
  }
}

gfx::Rect SelectionHandleEfl::GetSelectionRect() const {
  gfx::Rect rect;

  switch(handle_type_) {
  case HANDLE_TYPE_RIGHT:
    return controller_.GetRightRect();
  case HANDLE_TYPE_LEFT:
    return controller_.GetLeftRect();
  case HANDLE_TYPE_INPUT:
    // no rect for this type of handle
    break;
  }

  return gfx::Rect();
}

void SelectionHandleEfl::MoveObject(const gfx::Point& point, bool ignore_parent_view_offset) {
  gfx::Rect view_bounds = controller_.rwhv()->GetViewBoundsInPix();
  int handle_x = point.x() + view_bounds.x();
  int handle_y = point.y() + view_bounds.y();

  if (IsTop()) {
    if (handle_type_ == HANDLE_TYPE_RIGHT)
      handle_y -= controller_.GetRightRect().height();
    else
      handle_y -= controller_.GetLeftRect().height();
  }

  // Prevent selection handles from moving out of visible webview.
  if (handle_type_ != HANDLE_TYPE_INPUT) {
    const gfx::Rect& viewport_rect = controller_.GetVisibleViewportRect();
    int viewport_rect_x = viewport_rect.x();
    int viewport_rect_y = viewport_rect.y();
    int viewport_rect_w = viewport_rect.width();
    int viewport_rect_h = viewport_rect.height();

    if (handle_y < viewport_rect_y)
      handle_y = viewport_rect_y;
    else if (handle_y > viewport_rect_h + viewport_rect_y)
      handle_y = viewport_rect_h + viewport_rect_y;

    if (handle_x < viewport_rect_x)
      handle_x = viewport_rect_x;
    else if (handle_x > viewport_rect_w + viewport_rect_x)
      handle_x = viewport_rect_w + viewport_rect_x;
  }

  evas_object_move(handle_, handle_x, handle_y);
}

} // namespace content
