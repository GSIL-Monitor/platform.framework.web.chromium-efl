// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/select_picker/form_navigable_picker.h"

#include <Elementary.h>

#include "base/files/file_path.h"
#include "base/path_service.h"
#include "eweb_view.h"
#include "common/render_messages_ewk.h"
#include "common/web_contents_utils.h"
#include "content/common/paths_efl.h"

#if defined(OS_TIZEN)
#include <efl_extension.h>
#endif

namespace {
const char* const kMouseClicked1 = "mouse,clicked,1";
const char* const kImagePrevBgObj = "elm.image.prev_bg";
const char* const kImageNextBgObj = "elm.image.next_bg";
const char* const kImageDoneBgObj = "elm.image.done_bg";
const char* const kDimmObj = "dimm";
}

FormNavigablePicker::FormNavigablePicker(
    EWebView* web_view,
    int selected_index,
    bool is_multi_select,
    Elm_Genlist_Item_Scrollto_Type scrollto_type)
    : SelectPickerBase(web_view,
                       selected_index,
                       is_multi_select,
                       scrollto_type),
      form_navigator_info_{0, 0, true, true} {}

FormNavigablePicker::~FormNavigablePicker() {
  auto edje_obj = elm_layout_edje_get(layout_);
  edje_object_signal_callback_del(edje_obj, kMouseClicked1, kImagePrevBgObj,
                                  NavigateToPrevCallback);
  edje_object_signal_callback_del(edje_obj, kMouseClicked1, kImageNextBgObj,
                                  NavigateToNextCallback);
  edje_object_signal_callback_del(edje_obj, kMouseClicked1, kImageDoneBgObj,
                                  ListClosedCallback);
  edje_object_signal_callback_del(edje_obj, kMouseClicked1, kDimmObj,
                                  ListClosedCallback);
#if defined(OS_TIZEN)
  eext_object_event_callback_del(layout_, EEXT_CALLBACK_BACK,
                                 HWBackKeyCallback);
#endif
  evas_object_event_callback_del(web_view_->evas_object(), EVAS_CALLBACK_KEY_UP,
                                 KeyUpCallback);
}

void FormNavigablePicker::RegisterCallbacks() {
  auto edje_obj = elm_layout_edje_get(layout_);
  edje_object_signal_callback_add(edje_obj, kMouseClicked1, kImagePrevBgObj,
                                  NavigateToPrevCallback, this);
  edje_object_signal_callback_add(edje_obj, kMouseClicked1, kImageNextBgObj,
                                  NavigateToNextCallback, this);
  edje_object_signal_callback_add(edje_obj, kMouseClicked1, kImageDoneBgObj,
                                  ListClosedCallback, this);
  edje_object_signal_callback_add(edje_obj, kMouseClicked1, kDimmObj,
                                  ListClosedCallback, this);
#if defined(OS_TIZEN)
  eext_object_event_callback_add(layout_, EEXT_CALLBACK_BACK, HWBackKeyCallback,
                                 this);
#endif
  evas_object_event_callback_add(web_view_->evas_object(), EVAS_CALLBACK_KEY_UP,
                                 KeyUpCallback, this);
  evas_object_propagate_events_set(layout_, false);
}

void FormNavigablePicker::ShowButtons() {
  auto edje_obj = elm_layout_edje_get(layout_);
  edje_object_signal_emit(edje_obj, "show,prev_button,signal", "");
  edje_object_signal_emit(edje_obj, "show,next_button,signal", "");
  edje_object_signal_emit(edje_obj, "show,picker,signal", "");
}

void FormNavigablePicker::UpdateFormNavigation(int form_element_count,
                                               int current_node_index) {
  form_navigator_info_ = {form_element_count, current_node_index};
  UpdateNavigationButtons();
}

void FormNavigablePicker::UpdateNavigationButtons() {
  auto edje_obj = elm_layout_edje_get(layout_);
  if (form_navigator_info_.index > 0 && !form_navigator_info_.is_prev) {
    edje_object_signal_emit(edje_obj, "enable,prev_button,signal", "");
    form_navigator_info_.is_prev = true;
  } else if (form_navigator_info_.index == 0) {
    edje_object_signal_emit(edje_obj, "disable,prev_button,signal", "");
    form_navigator_info_.is_prev = false;
  }
  if (form_navigator_info_.index < form_navigator_info_.count - 1 &&
      !form_navigator_info_.is_next) {
    edje_object_signal_emit(edje_obj, "enable,next_button,signal", "");
    form_navigator_info_.is_next = true;
  } else if (form_navigator_info_.index == form_navigator_info_.count - 1) {
    edje_object_signal_emit(edje_obj, "disable,next_button,signal", "");
    form_navigator_info_.is_next = false;
  }
}

void FormNavigablePicker::RegisterCallbacks(const char* edj, bool overlay) {
  base::FilePath edj_dir;
  base::FilePath control_edj;
  PathService::Get(PathsEfl::EDJE_RESOURCE_DIR, &edj_dir);
  control_edj = edj_dir.Append(FILE_PATH_LITERAL(edj));
  if (!elm_layout_file_set(layout_, control_edj.AsUTF8Unsafe().c_str(),
                           "elm/picker")) {
    LOG(ERROR) << "error elm_layout_file_set, " << edj;
  } else {
    if (overlay)
      elm_theme_overlay_add(nullptr, control_edj.AsUTF8Unsafe().c_str());
    RegisterCallbacks();
  }
  ShowButtons();
}

Evas_Object* FormNavigablePicker::AddBackground() {
  Evas_Object* bg = elm_bg_add(layout_);
  elm_object_part_content_set(layout_, "bg", bg);
  return bg;
}

void FormNavigablePicker::AddDoneButton() {
  edje_object_part_text_set(elm_layout_edje_get(layout_), "elm.text.done",
                            dgettext("WebKit", "IDS_WEBVIEW_BUTTON_DONE"));
}

void FormNavigablePicker::UpdatePickerData(
    int selected_index,
    const std::vector<content::MenuItem>& items,
    bool is_multiple_selection) {
  RequestFormNavigationInformation();
  SelectPickerBase::UpdatePickerData(selected_index, items,
                                     is_multiple_selection);
}

void FormNavigablePicker::FormNavigate(bool direction) {
  auto render_frame_host = static_cast<content::RenderFrameHostImpl*>(
      web_view_->web_contents().GetFocusedFrame());
  if (!render_frame_host)
    return;
  render_frame_host->DidCancelPopupMenu();
  render_frame_host->Send(new EwkFrameMsg_MoveToNextOrPreviousSelectElement(
      render_frame_host->GetRoutingID(), direction));
}

void FormNavigablePicker::RequestFormNavigationInformation() {
  auto rfh = static_cast<content::RenderFrameHostImpl*>(
      web_view_->web_contents().GetFocusedFrame());
  if (rfh) {
    rfh->Send(new EwkFrameMsg_RequestSelectCollectionInformation(
        rfh->GetRoutingID()));
  }
}

void FormNavigablePicker::NavigateToNextCallback(void* data,
                                                 Evas_Object* obj,
                                                 const char* emission,
                                                 const char* source) {
  auto picker = static_cast<FormNavigablePicker*>(data);
  picker->FormNavigate(true);
}

void FormNavigablePicker::NavigateToPrevCallback(void* data,
                                                 Evas_Object* obj,
                                                 const char* emission,
                                                 const char* source) {
  auto picker = static_cast<FormNavigablePicker*>(data);
  picker->FormNavigate(false);
}

void FormNavigablePicker::CloseList() {
  if (is_multiple_selection_)
    DidMultipleSelectPopupMenuItem();
  else
    DidSelectPopupMenuItem();

  web_view_->HidePopupMenu();
}

void FormNavigablePicker::KeyUpCallback(void* data,
                                        Evas* e,
                                        Evas_Object* obj,
                                        void* event_info) {
  auto key_struct = static_cast<Evas_Event_Key_Up*>(event_info);
  if (!web_contents_utils::MapsToHWBackKey(key_struct->keyname))
    return;
#if defined(OS_TIZEN)
  HWBackKeyCallback(data, obj, event_info);
#endif
}

#if defined(OS_TIZEN)
void FormNavigablePicker::HWBackKeyCallback(void* data,
                                            Evas_Object* obj,
                                            void* event_info) {
  ListClosedCallback(data, obj, 0, 0);
}
#endif

void FormNavigablePicker::ListClosedCallback(void* data,
                                             Evas_Object* obj,
                                             const char* emission,
                                             const char* source) {
  auto picker = static_cast<FormNavigablePicker*>(data);
  picker->CloseList();
}
