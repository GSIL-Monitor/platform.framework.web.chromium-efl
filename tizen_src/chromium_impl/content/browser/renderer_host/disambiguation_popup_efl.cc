// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/disambiguation_popup_efl.h"

#include <Elementary.h>

#include "base/path_service.h"
#include "base/files/file_path.h"
#include "content/browser/renderer_host/render_widget_host_view_efl.h"
#include "content/browser/renderer_host/web_event_factory_efl.h"
#include "content/common/paths_efl.h"
#include "tizen/system_info.h"
#include "ui/events/event.h"
#include "ui/display/screen.h"

#if defined(OS_TIZEN)
#include <efl_extension.h>
#endif

namespace content {

namespace {
#if defined(OS_TIZEN)
void DisambiguationPopupHWBackKey(void* data, Evas_Object* obj, void* event_info) {
  if (IsMobileProfile() || IsWearableProfile()) {
    DisambiguationPopupEfl* instance = static_cast<DisambiguationPopupEfl*>(data);
    instance->Dismiss();
  }
}
#endif

void OnMouseUp(void* data, Evas*, Evas_Object* image, void* event_info) {
  Evas_Event_Mouse_Up* up_event =
      static_cast<Evas_Event_Mouse_Up*>(event_info);
  DisambiguationPopupEfl* disambiguation_popup =
      static_cast<DisambiguationPopupEfl*>(data);
  disambiguation_popup->HandleMouseEventUp(up_event);
}

void OnMouseDown(void* data, Evas*, Evas_Object* image, void* event_info) {
  Evas_Event_Mouse_Down* down_event =
      static_cast<Evas_Event_Mouse_Down*>(event_info);
  DisambiguationPopupEfl* disambiguation_popup =
      static_cast<DisambiguationPopupEfl*>(data);
  disambiguation_popup->HandleMouseEventDown(down_event);
}

void TouchedOnOutterArea(void* data, Evas*, Evas_Object* image, void* event_info) {
  DisambiguationPopupEfl* disambiguation_popup =
      static_cast<DisambiguationPopupEfl*>(data);
  disambiguation_popup->Dismiss();
}

} // namespace

DisambiguationPopupEfl::DisambiguationPopupEfl(Evas_Object* parent_view,
                                               RenderWidgetHostViewEfl* rwhv)
    : rwhv_(rwhv),
      parent_view_(parent_view),
      content_image_(nullptr),
      popup_(nullptr),
      height_(0),
      width_(0) {
  CHECK(parent_view);
  CHECK(rwhv);

  popup_ = elm_ctxpopup_add(parent_view);
  Elm_Theme* theme = elm_theme_new();
  base::FilePath edje_path;
  base::FilePath link_magnifier_edj;
  PathService::Get(PathsEfl::EDJE_RESOURCE_DIR, &edje_path);
  link_magnifier_edj = edje_path.Append(FILE_PATH_LITERAL("DisambiguationPopup.edj"));
  elm_theme_ref_set(theme, elm_theme_default_get());
  elm_theme_extension_add(theme, link_magnifier_edj.AsUTF8Unsafe().c_str());
  elm_object_theme_set(popup_, theme);
  elm_theme_free(theme);

  elm_ctxpopup_hover_parent_set(popup_, parent_view);
  elm_ctxpopup_direction_priority_set(popup_, ELM_CTXPOPUP_DIRECTION_UP,
      ELM_CTXPOPUP_DIRECTION_DOWN, ELM_CTXPOPUP_DIRECTION_UP,
      ELM_CTXPOPUP_DIRECTION_DOWN);
  evas_object_smart_callback_add(popup_, "dismissed", Dismissed, this);
}

DisambiguationPopupEfl::~DisambiguationPopupEfl() {
  Clear();
  evas_object_del(popup_);
  popup_ = NULL;
}

void DisambiguationPopupEfl::Show(const gfx::Rect& target,
                                  const SkBitmap& display_image) {
  DCHECK(!IsVisible());
  target_rect_ = target;

  if (content_image_)
    evas_object_del(content_image_);
  content_image_ = evas_object_image_add(evas_object_evas_get(parent_view_));
  elm_object_content_set(popup_, content_image_);

  UpdateImage(display_image);

  evas_object_event_callback_add(content_image_,
      EVAS_CALLBACK_MOUSE_UP, OnMouseUp, this);
  evas_object_event_callback_add(content_image_,
      EVAS_CALLBACK_MOUSE_DOWN, OnMouseDown, this);

  evas_object_event_callback_add(popup_,
      EVAS_CALLBACK_MOUSE_UP, TouchedOnOutterArea, this);
  evas_object_event_callback_add(popup_,
      EVAS_CALLBACK_MULTI_DOWN, TouchedOnOutterArea, this);

#if defined(OS_TIZEN)
  if (IsMobileProfile() || IsWearableProfile()) {
    eext_object_event_callback_add(
        popup_, EEXT_CALLBACK_BACK, DisambiguationPopupHWBackKey, this);
  }
#endif

  int parent_view_x = 0, parent_view_y = 0;
  evas_object_geometry_get(parent_view_, &parent_view_x, &parent_view_y, 0, 0);

  const gfx::Point position = target_rect_.CenterPoint();
  evas_object_move(popup_, position.x() + parent_view_x,
      position.y() + parent_view_y);

  evas_object_show(popup_);
}

void DisambiguationPopupEfl::Dismiss() {
  DCHECK(IsVisible());
  rwhv_->DisambiguationPopupDismissed();

  target_rect_ = gfx::Rect();
  elm_ctxpopup_dismiss(popup_);
  Clear();
}

void DisambiguationPopupEfl::UpdateImage(const SkBitmap& display_image) {
  DCHECK(content_image_);

  width_ = display_image.width();
  height_ = display_image.height();

  // TODO: should be set to same as display_image colorspace
  evas_object_image_colorspace_set(content_image_, EVAS_COLORSPACE_ARGB8888);

  evas_object_image_alpha_set(content_image_, EINA_TRUE);
  evas_object_image_filled_set(content_image_, EINA_FALSE);

  evas_object_image_size_set(content_image_, width_, height_);
  evas_object_image_data_copy_set(content_image_, display_image.getPixels());

  evas_object_image_fill_set(content_image_, 0, 0, width_, height_);
  evas_object_size_hint_min_set(content_image_, width_, height_);
  evas_object_size_hint_max_set(content_image_, width_, height_);
  evas_object_size_hint_padding_set(content_image_, 0, 0, 0, 0);
}

void DisambiguationPopupEfl::Clear() {
  evas_object_event_callback_del(popup_, EVAS_CALLBACK_MOUSE_UP,
                                 TouchedOnOutterArea);
  evas_object_event_callback_del(popup_, EVAS_CALLBACK_MULTI_DOWN,
                                 TouchedOnOutterArea);

#if defined(OS_TIZEN)
  if (IsMobileProfile() || IsWearableProfile()) {
    eext_object_event_callback_del(popup_, EEXT_CALLBACK_BACK,
                                   DisambiguationPopupHWBackKey);
  }
#endif

  evas_object_hide(popup_);
  elm_object_content_unset(popup_);
  if (content_image_) {
    evas_object_event_callback_del(content_image_, EVAS_CALLBACK_MOUSE_UP,
                                   OnMouseUp);
    evas_object_event_callback_del(content_image_, EVAS_CALLBACK_MOUSE_DOWN,
                                   OnMouseDown);
    evas_object_del(content_image_);
    content_image_ = NULL;
  }
}

bool DisambiguationPopupEfl::IsVisible() {
  return evas_object_visible_get(popup_);
}

void DisambiguationPopupEfl::HandleMouseEventUp(Evas_Event_Mouse_Up* up_event) {
  PrepareMouseEventForForward(up_event);
  rwhv_->HandleDisambiguationPopupMouseUpEvent(up_event);
  Dismiss();
}

void DisambiguationPopupEfl::HandleMouseEventDown(Evas_Event_Mouse_Down* down_event) {
  PrepareMouseEventForForward(down_event);
  rwhv_->HandleDisambiguationPopupMouseDownEvent(down_event);
}

template<class T>
void DisambiguationPopupEfl::PrepareMouseEventForForward(T* event) {

  int image_x = 0, image_y = 0;
  evas_object_geometry_get(content_image_, &image_x, &image_y, 0, 0);

  int image_cord_x = event->canvas.x - image_x;
  int image_cord_y = event->canvas.y - image_y;

  // We have image cords, now we need to translate them to target_rect_.
  float ratio = static_cast<float>(width_) / target_rect_.width();

  int target_rect_x = target_rect_.x() + (image_cord_x / ratio);
  int target_rect_y = target_rect_.y() + (image_cord_y / ratio);

  int parent_view_x = 0, parent_view_y = 0;
  evas_object_geometry_get(parent_view_, &parent_view_x, &parent_view_y, 0, 0);
  float device_scale_factor = display::Screen::GetScreen()->
      GetPrimaryDisplay().device_scale_factor();
  event->canvas.x = target_rect_x + (parent_view_x * device_scale_factor);
  event->canvas.y = target_rect_y + (parent_view_y * device_scale_factor);
}

void DisambiguationPopupEfl::Dismissed(void* data, Evas_Object*, void*) {
  DisambiguationPopupEfl* disambiguation_popup =
      static_cast<DisambiguationPopupEfl*>(data);
  disambiguation_popup->Clear();
}

}
