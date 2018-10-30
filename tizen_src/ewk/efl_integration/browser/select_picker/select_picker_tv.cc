// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/select_picker/select_picker_tv.h"
#include "common/application_type.h"
#include "eweb_view.h"
#include "private/webview_delegate_ewk.h"

// Size of <select> picker
static const int kBoxWidth = 400;
static const int kBoxHeight = 500;
static const float kBoxRatio = 0.4;

SelectPickerEflTv::SelectPickerEflTv(EWebView* web_view,
                                     int selected_index,
                                     bool is_multiple_selection)
    : FormNavigablePicker(web_view,
                          selected_index,
                          is_multiple_selection,
                          ELM_GENLIST_ITEM_SCROLLTO_MIDDLE) {
  evas_object_size_hint_weight_set(layout_, kBoxWidth, kBoxHeight);
  evas_object_resize(layout_, kBoxWidth, kBoxHeight);
  ecore_events_filter_ =
      ecore_event_filter_add(nullptr, EcoreEventFilterCallback, nullptr, this);
  if (!ecore_events_filter_)
    LOG(ERROR) << "Unable to create ecore events filter";
}

SelectPickerEflTv::~SelectPickerEflTv() {
  Hide();
  if (ecore_events_filter_)
    ecore_event_filter_del(ecore_events_filter_);
}

const char* SelectPickerEflTv::GetItemStyle() const {
  return "APP_STYLE";
}

void SelectPickerEflTv::Init(const std::vector<content::MenuItem>& items,
                           const gfx::Rect& bounds) {
  // Request form navigation information as early as possible,
  // given that is renderer will ping-back with actual requested data.
  RequestFormNavigationInformation();
  RegisterCallbacks("controlTV.edj", true);
  CoordinatePopupPosition(bounds.origin());
  CreateAndPopulatePopupList(items);
  elm_bg_color_set(AddBackground(), 231, 234, 242);
  AddDoneButton();
}

void SelectPickerEflTv::CreateAndPopulatePopupList(
    const std::vector<content::MenuItem>& items) {
  popup_list_ = elm_genlist_add(layout_);
  elm_genlist_homogeneous_set(popup_list_, EINA_TRUE);
  elm_genlist_mode_set(popup_list_, ELM_LIST_COMPRESS);
  elm_object_style_set(popup_list_, "styleA");
  evas_object_size_hint_weight_set(popup_list_, EVAS_HINT_EXPAND,
                                   EVAS_HINT_EXPAND);
  evas_object_size_hint_align_set(popup_list_, EVAS_HINT_FILL, EVAS_HINT_FILL);
  elm_object_focus_allow_set(popup_list_, false);
  item_class_->func.content_get = nullptr;
  elm_genlist_multi_select_set(popup_list_, is_multiple_selection_);
  InitializeSelectedPickerData(items);
#if defined(TIZEN_ATK_SUPPORT)
  elm_genlist_item_selected_set(selected_item_, EINA_TRUE);
#endif
  elm_object_part_content_set(layout_, "elm.swallow.content", popup_list_);
  elm_object_scale_set(popup_list_, kBoxRatio);
}

void SelectPickerEflTv::CoordinatePopupPosition(const gfx::Point& origin) {
  Eina_Rectangle webview_bounds =
      WebViewDelegateEwk::GetInstance().GetLastUsedViewPortArea(
          web_view_->evas_object());
  int target_x = origin.x();
  int target_y = origin.y() + webview_bounds.y;

  if (target_x + kBoxWidth > webview_bounds.w)
    target_x = webview_bounds.w - kBoxWidth;
  if (target_y + kBoxHeight > webview_bounds.h)
    target_y = webview_bounds.h - kBoxHeight;

  evas_object_move(layout_, target_x, target_y);
}

void SelectPickerEflTv::NextItem(bool foward) {
  Elm_Object_Item* next_item = nullptr;
  Elm_Object_Item* current_item = elm_genlist_selected_item_get(popup_list_);

  if (!current_item)
    return;

  if (foward)
    next_item = elm_genlist_item_next_get(current_item);
  else
    next_item = elm_genlist_item_prev_get(current_item);

  if (next_item)
    elm_genlist_item_selected_set(next_item, true);

  ScrollSelectedItemTo();
}

Eina_Bool SelectPickerEflTv::EcoreEventFilterCallback(void* user_data,
                                                      void* /*loop_data*/,
                                                      int type,
                                                      void* event) {
  auto picker = static_cast<SelectPickerEflTv*>(user_data);
  bool spatial_navigation_enabled = picker->web_view_->GetSettings()
                                        ->getPreferences()
                                        .spatial_navigation_enabled;
  if (type != ECORE_EVENT_KEY_DOWN && type != ECORE_EVENT_KEY_UP)
    return ECORE_CALLBACK_PASS_ON;

  Ecore_Event_Key* key_event = static_cast<Ecore_Event_Key*>(event);
#if defined(OS_TIZEN_TV_PRODUCT)
  const char* input_device_name = ecore_device_name_get(key_event->dev);
  bool is_keyboard =
      !!input_device_name && (strstr(input_device_name, "keyboard") ||
                              strstr(input_device_name, "Keyboard") ||
                              strstr(input_device_name, "key board"));

  // In web browser, direction keys of RC are used to move pointer if
  // spatial navigation is not enabled.
  if (content::IsWebBrowser() && !spatial_navigation_enabled && !is_keyboard)
    return ECORE_CALLBACK_PASS_ON;
#endif

  std::string key_name = key_event->keyname;
  if (type == ECORE_EVENT_KEY_DOWN && picker->IsVisible()) {
    if (!key_name.compare("Left")) {
      picker->FormNavigate(false);
      return ECORE_CALLBACK_CANCEL;
    } else if (!key_name.compare("Right")) {
      picker->FormNavigate(true);
      return ECORE_CALLBACK_CANCEL;
    } else if (!key_name.compare("Up")) {
      picker->NextItem(false);
      return ECORE_CALLBACK_CANCEL;
    } else if (!key_name.compare("Down")) {
      picker->NextItem(true);
      return ECORE_CALLBACK_CANCEL;
    } else if (!key_name.compare("Return") || !key_name.compare("XF86Return")) {
      picker->CloseList();
      return ECORE_CALLBACK_CANCEL;
    }
  } else if (type == ECORE_EVENT_KEY_UP && picker->IsVisible()) {
    if (!key_name.compare("Left") || !key_name.compare("Right") ||
        !key_name.compare("Up") || !key_name.compare("Down") ||
        !key_name.compare("Return") || !key_name.compare("XF86Return"))
      return ECORE_CALLBACK_CANCEL;
  }
  return ECORE_CALLBACK_PASS_ON;
}
