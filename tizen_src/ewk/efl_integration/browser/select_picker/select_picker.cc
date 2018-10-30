// Copyright 2014-17 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/select_picker/select_picker.h"

#include "base/logging.h"
#include "browser/select_picker/select_picker_util.h"
#include "eweb_view.h"
#include "tizen/system_info.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/dip_util.h"

namespace {

const char* const kChangedCbName = "changed";
const char* const kShowPickerDone = "show,picker,done";
#if defined(TIZEN_ATK_SUPPORT)
const char* const kImagePrevBgObj = "elm.image.prev_bg";
const char* const kImageNextBgObj = "elm.image.next_bg";
const char* const kImageDoneBgObj = "elm.image.done_bg";
#endif

void RadioIconChangedCallback(void* data, Evas_Object* obj, void* event_info) {
  auto callback_data = static_cast<GenlistCallbackData*>(data);
  callback_data->SetSelection(true);
}

} // anonymous namespace

SelectPickerEfl::SelectPickerEfl(EWebView* web_view,
                                 int selected_index,
                                 bool is_multiple_selection)
    : FormNavigablePicker(web_view, selected_index, is_multiple_selection),
      radio_main_(nullptr)
#if defined(TIZEN_ATK_SUPPORT)
      ,
      prev_button_(nullptr),
      next_button_(nullptr),
      done_button_(nullptr),
      ao_prev_button_(nullptr),
      ao_next_button_(nullptr),
      ao_done_button_(nullptr)
#endif
{
  evas_object_size_hint_weight_set(layout_, EVAS_HINT_EXPAND,
                                   EVAS_HINT_EXPAND);

  elm_win_resize_object_add(window_, layout_);
  evas_object_propagate_events_set(layout_, false);

  ecore_events_filter_ =
      ecore_event_filter_add(nullptr, EcoreEventFilterCallback, nullptr, this);
  if (!ecore_events_filter_)
    LOG(ERROR) << "Unable to create ecore events filter";
}

SelectPickerEfl::~SelectPickerEfl() {
  edje_object_signal_callback_del(elm_layout_edje_get(layout_), kShowPickerDone,
                                  "", ShowPickerDoneCallback);
#if defined(TIZEN_ATK_SUPPORT)
  RemoveAtkObject();
#endif

  if (ecore_events_filter_)
    ecore_event_filter_del(ecore_events_filter_);

  Hide();
  DestroyRadioList();
}

#if defined(TIZEN_ATK_SUPPORT)
Eina_Bool SelectPickerEfl::AccessNavigateToPrevCallback(
    void* data,
    Evas_Object* obj,
    Elm_Access_Action_Info* action_info) {
  auto picker = static_cast<SelectPickerEfl*>(data);
  picker->FormNavigate(false);
  return EINA_TRUE;
}

Eina_Bool SelectPickerEfl::AccessNavigateToNextCallback(
    void* data,
    Evas_Object* obj,
    Elm_Access_Action_Info* action_info) {
  auto picker = static_cast<SelectPickerEfl*>(data);
  picker->FormNavigate(true);
  return EINA_TRUE;
}

Eina_Bool SelectPickerEfl::AccessDoneCallback(
    void* data,
    Evas_Object* obj,
    Elm_Access_Action_Info* action_info) {
  auto picker = static_cast<SelectPickerEfl*>(data);
  picker->web_view_->HidePopupMenu();
  return EINA_TRUE;
}

void SelectPickerEfl::AddAtkObject() {
  auto edje_obj = elm_layout_edje_get(layout_);

  prev_button_ =
      (Evas_Object*)edje_object_part_object_get(edje_obj, kImagePrevBgObj);
  ao_prev_button_ = elm_access_object_register(prev_button_, layout_);
  elm_atspi_accessible_name_set(
      ao_prev_button_, dgettext("WebKit", "IDS_WEBVIEW_BUTTON_PREV_ABB"));
  elm_atspi_accessible_reading_info_type_set(
      ao_prev_button_, ELM_ACCESSIBLE_READING_INFO_TYPE_NAME);

  next_button_ =
      (Evas_Object*)edje_object_part_object_get(edje_obj, kImageNextBgObj);
  ao_next_button_ = elm_access_object_register(next_button_, layout_);
  elm_atspi_accessible_name_set(
      ao_next_button_, dgettext("WebKit", "IDS_WEBVIEW_BUTTON_NEXT_ABB3"));
  elm_atspi_accessible_reading_info_type_set(
      ao_next_button_, ELM_ACCESSIBLE_READING_INFO_TYPE_NAME);

  done_button_ =
      (Evas_Object*)edje_object_part_object_get(edje_obj, kImageDoneBgObj);
  ao_done_button_ = elm_access_object_register(done_button_, layout_);
  elm_atspi_accessible_name_set(ao_done_button_,
                                dgettext("WebKit", "IDS_WEBVIEW_BUTTON_DONE"));
  elm_atspi_accessible_reading_info_type_set(
      ao_done_button_, ELM_ACCESSIBLE_READING_INFO_TYPE_NAME);

  elm_access_action_cb_set(ao_prev_button_, ELM_ACCESS_ACTION_ACTIVATE,
                           AccessNavigateToPrevCallback, this);
  elm_access_action_cb_set(ao_next_button_, ELM_ACCESS_ACTION_ACTIVATE,
                           AccessNavigateToNextCallback, this);
  elm_access_action_cb_set(ao_done_button_, ELM_ACCESS_ACTION_ACTIVATE,
                           AccessDoneCallback, this);

  elm_atspi_accessible_relationship_append(
      ao_done_button_, ELM_ATSPI_RELATION_FLOWS_TO, first_item_);
  elm_atspi_accessible_relationship_append(
      first_item_, ELM_ATSPI_RELATION_FLOWS_FROM, ao_done_button_);
}

void SelectPickerEfl::RemoveAtkObject() {
  if (ao_done_button_) {
    elm_atspi_accessible_relationship_remove(
        ao_done_button_, ELM_ATSPI_RELATION_FLOWS_TO, first_item_);
    elm_atspi_accessible_relationship_remove(
        first_item_, ELM_ATSPI_RELATION_FLOWS_FROM, ao_done_button_);
  }

  if (prev_button_)
    elm_access_object_unregister(prev_button_);
  if (next_button_)
    elm_access_object_unregister(next_button_);
  if (done_button_)
    elm_access_object_unregister(done_button_);
}
#endif

void SelectPickerEfl::Init(const std::vector<content::MenuItem>& items,
                           const gfx::Rect& bounds) {
  // Request form navigation information as early as possible,
  // given that is renderer will ping-back with actual requested data.
  RequestFormNavigationInformation();

  RegisterCallbacks("control.edj", false);

  // Adds extra callbacks for viewport adjustment.
  edje_object_signal_callback_add(elm_layout_edje_get(layout_), kShowPickerDone,
                                  "", ShowPickerDoneCallback, this);

  CreateAndPopulatePopupList(items);
  AddBackground();
  AddDoneButton();

  // Need to update evas objects here in order to get proper geometry
  // at EWebView::AdjustViewPortHeightToPopupMenu.
  evas_norender(evas_object_evas_get(window_));
}

void SelectPickerEfl::DestroyRadioList() {
  if (!radio_main_)
    return;
  if (is_multiple_selection_)
    evas_object_smart_callback_del(popup_list_, kChangedCbName,
                                   RadioIconChangedCallback);
  elm_genlist_clear(radio_main_);
  evas_object_del(radio_main_);
}

void SelectPickerEfl::ClearData() {
  DestroyRadioList();
  FormNavigablePicker::ClearData();
}

Evas_Object* SelectPickerEfl::IconGetCallback(void* data, Evas_Object* obj,
                                              const char* part) {

  auto callback_data = static_cast<GenlistCallbackData*>(data);
  auto picker = static_cast<SelectPickerEfl*>(callback_data->GetSelectPicker());
  if (callback_data->IsEnabled() && !strcmp(part, "elm.swallow.end")) {
    Evas_Object* radio = elm_radio_add(obj);
    elm_object_focus_allow_set(radio, false);
    elm_radio_state_value_set(radio, callback_data->GetIndex());
    elm_radio_group_add(radio, picker->radio_main_);

    evas_object_size_hint_weight_set(radio, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(radio, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_propagate_events_set(radio, EINA_FALSE);

    elm_radio_value_set(picker->radio_main_, picker->selected_index_);
    evas_object_smart_callback_add(radio, kChangedCbName,
                                   RadioIconChangedCallback, (void*)data);
    return radio;
  }
  return nullptr;
}

// static
void SelectPickerEfl::ShowPickerDoneCallback(void* data,
                                             Evas_Object*,
                                             const char*,
                                             const char*) {
  auto picker = static_cast<SelectPickerEfl*>(data);
  if (!picker || !picker->web_view_)
    return;

  picker->web_view_->AdjustViewPortHeightToPopupMenu(true);
  picker->web_view_->ScrollFocusedNodeIntoView();
}

void SelectPickerEfl::ItemSelected(GenlistCallbackData* data,
                                   void* event_info) {
  // Efl highlights item by default. We only want radio button to be checked.
  Evas_Object* radio_icon = elm_object_item_part_content_get(
      (Elm_Object_Item*)event_info, "elm.swallow.end");
  data->SetSelection(false);

  if (data->IsEnabled()) {
    elm_radio_value_set(radio_icon, data->GetIndex());
    selected_index_ = data->GetIndex();
    DidSelectPopupMenuItem();
  }
#if defined(TIZEN_ATK_SUPPORT)
  if (web_view_->GetAtkStatus())
    web_view_->HidePopupMenu();
#endif
}

void SelectPickerEfl::CreateAndPopulatePopupList(
    const std::vector<content::MenuItem>& items) {
  popup_list_ = elm_genlist_add(layout_);
  elm_genlist_homogeneous_set(popup_list_, EINA_TRUE);
  elm_genlist_mode_set(popup_list_, ELM_LIST_COMPRESS);
  elm_object_style_set(popup_list_, "solid/default");
  evas_object_size_hint_weight_set(popup_list_, EVAS_HINT_EXPAND,
                                   EVAS_HINT_EXPAND);
  evas_object_size_hint_align_set(popup_list_, EVAS_HINT_FILL, EVAS_HINT_FILL);

  elm_object_focus_allow_set(popup_list_, false);

#if defined(TIZEN_ATK_SUPPORT)
  if (web_view_->GetAtkStatus())
    item_class_->func.content_get = nullptr;
#else
  item_class_->func.content_get =
      is_multiple_selection_ ? nullptr : IconGetCallback;
#endif

  elm_genlist_multi_select_set(popup_list_, is_multiple_selection_);

  if (!is_multiple_selection_) {
    radio_main_ = elm_radio_add(popup_list_);
    if (!radio_main_) {
      LOG(ERROR) << "elm_radio_add failed.";
      return;
    }

    elm_radio_state_value_set(radio_main_, 0);
    elm_radio_value_set(radio_main_, 0);

    InitializeSelectedPickerData(items);
    if (selected_index_ >= 0) {
      elm_radio_value_set(radio_main_, selected_index_);
    }

    evas_object_smart_callback_add(popup_list_, kChangedCbName,
                                   RadioIconChangedCallback, this);
  } else {
    InitializeSelectedPickerData(items);
  }

  elm_object_part_content_set(layout_, "elm.swallow.content", popup_list_);
#if defined(TIZEN_ATK_SUPPORT)
  AddAtkObject();
#endif
}

Eina_Bool SelectPickerEfl::EcoreEventFilterCallback(void* user_data,
                                                    void* /*loop_data*/,
                                                    int type,
                                                    void* event) {
  auto picker = static_cast<SelectPickerEfl*>(user_data);
  if (type == ECORE_EVENT_KEY_DOWN && picker->IsVisible()) {
    std::string key_name = static_cast<Ecore_Event_Key*>(event)->keyname;
    if (!key_name.compare("Up") || !key_name.compare("Left")) {
      picker->FormNavigate(false);
      return ECORE_CALLBACK_CANCEL;
    } else if (!key_name.compare("Right") || !key_name.compare("Down")) {
      picker->FormNavigate(true);
      return ECORE_CALLBACK_CANCEL;
    }
  }
  return ECORE_CALLBACK_PASS_ON;
}

gfx::Rect SelectPickerEfl::GetGeometryDIP() const {
  if (IsMobileProfile()) {
    int x, y, w, h;
    edje_object_part_geometry_get(elm_layout_edje_get(layout_), "bg", &x, &y, &w,
                                  &h);
    return ConvertRectToDIP(display::Screen::GetScreen()->
                                GetPrimaryDisplay().device_scale_factor(),
                            gfx::Rect(x, y, w, h));
  } else {
    return gfx::Rect();
  }
 }

