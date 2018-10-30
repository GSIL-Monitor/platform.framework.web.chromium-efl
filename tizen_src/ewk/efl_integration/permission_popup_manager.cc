// Copyright 2015-2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "permission_popup_manager.h"

#if defined(OS_TIZEN)
#include <efl_extension.h>
#endif

#include "base/strings/utf_string_conversions.h"
#include "permission_popup.h"

/* LCOV_EXCL_START */
PermissionPopupManager::PermissionPopupManager(Evas_Object* evas_object)
    : permission_popups_(nullptr),
      evas_object_(evas_object),
      popup_(nullptr),
      top_widget_(nullptr) {}

PermissionPopupManager::~PermissionPopupManager() {
  CancelPermissionRequest();
}

void PermissionPopupManager::AddPermissionRequest(
    PermissionPopup* permission_popup) {
  permission_popups_ = eina_list_append(permission_popups_, permission_popup);
  ShowPermissionPopup(static_cast<PermissionPopup*>(
      eina_list_data_get(permission_popups_)));
}
/* LCOV_EXCL_STOP */

void PermissionPopupManager::CancelPermissionRequest() {
  DeletePopup();
  DeleteAllPermissionRequests();
}

void PermissionPopupManager::DeleteAllPermissionRequests() {
  if (!permission_popups_)
    return;

  void* data;
  /* LCOV_EXCL_START */
  EINA_LIST_FREE(permission_popups_, data) {
    auto permission_popup = static_cast<PermissionPopup*>(data);
    permission_popup->SendDecidedPermission(evas_object_, false);
    delete permission_popup;
  }
  /* LCOV_EXCL_STOP */
}

/* LCOV_EXCL_START */
void PermissionPopupManager::DeletePermissionRequest(
    PermissionPopup* permission_popup) {
  permission_popups_ = eina_list_remove(permission_popups_, permission_popup);
  delete permission_popup;
}

void PermissionPopupManager::ShowPermissionPopup(
    PermissionPopup* permission_popup) {
  if (popup_)
    return;
  if (!CreatePopup())
    return;
  if (!SetupPopup(permission_popup)) {
    DeletePopup();
    return;
  }

  evas_object_show(popup_);
}

void PermissionPopupManager::Decide(bool decision) {
  DeletePopup();

  auto permission_popup = static_cast<PermissionPopup*>(
      eina_list_data_get(permission_popups_));
  permission_popup->SendDecidedPermission(evas_object_, decision);
  DeletePermissionRequest(permission_popup);
  permission_popup = static_cast<PermissionPopup*>(
      eina_list_data_get(permission_popups_));
  if (permission_popup)
    ShowPermissionPopup(permission_popup);
}

void PermissionPopupManager::PermissionOkCallback(void* data, Evas_Object* obj,
                                                  void* event_info) {
  PermissionPopupManager* manager = static_cast<PermissionPopupManager*>(data);
  manager->Decide(true);
}

void PermissionPopupManager::PermissionCancelCallback(void* data,
                                                      Evas_Object* obj,
                                                      void* event_info) {
  PermissionPopupManager* manager = static_cast<PermissionPopupManager*>(data);
  manager->Decide(false);
}

bool PermissionPopupManager::CreatePopup() {
  top_widget_ = elm_object_top_widget_get(
      elm_object_parent_widget_get(evas_object_));
  if (!top_widget_)
    return false;

  Evas_Object* conformant = elm_conformant_add(top_widget_);
  if (!conformant)
    return false;

  evas_object_size_hint_weight_set(conformant, EVAS_HINT_EXPAND,
                                   EVAS_HINT_EXPAND);
  elm_win_resize_object_add(top_widget_, conformant);
  evas_object_show(conformant);

  Evas_Object* layout = elm_layout_add(conformant);
  if (!layout)
    return false;

  elm_layout_theme_set(layout, "layout", "application", "default");
  evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
  evas_object_show(layout);

  elm_object_content_set(conformant, layout);

  popup_ = elm_popup_add(layout);
  return !!popup_;
}
/* LCOV_EXCL_STOP */

void PermissionPopupManager::DeletePopup() {
  if (!popup_)
    return;
  evas_object_del(popup_); // LCOV_EXCL_LINE
  popup_ = nullptr; // LCOV_EXCL_LINE
}

/* LCOV_EXCL_START */
bool PermissionPopupManager::SetupPopup(PermissionPopup* permission_popup) {
  elm_popup_align_set(popup_, ELM_NOTIFY_ALIGN_FILL, 1.0);

  if (!SetPopupText(permission_popup->GetMessage()) ||
      !SetPopupTitle(permission_popup->GetOrigin()))
    return false;

#if defined(OS_TIZEN)
  eext_object_event_callback_add(
      popup_, EEXT_CALLBACK_BACK,
      &PermissionPopupManager::PermissionCancelCallback, this);
#endif

  AddButtonToPopup("button1", "IDS_WEBVIEW_BUTTON_CANCEL_ABB4",
                   &PermissionPopupManager::PermissionCancelCallback, false);
  AddButtonToPopup("button2", "IDS_WEBVIEW_BUTTON_OK_ABB4",
                   &PermissionPopupManager::PermissionOkCallback, true);

  return true;
}

bool PermissionPopupManager::SetPopupTitle(const std::string& origin) {
  // Empty origin is improper one.
  if (origin.empty())
    return false;
  std::string title =
      dgettext("WebKit","IDS_WEBVIEW_HEADER_MESSAGE_FROM_PS_M_WEBSITE");

  const std::string replaceStr("%s");
  size_t pos = title.find(replaceStr);
  if (pos != std::string::npos)
    title.replace(pos, replaceStr.length(), origin);
  else
    LOG(ERROR) << "Translation issue. " << title << " not found";
  elm_object_part_text_set(popup_, "title,text", title.c_str());
  return true;
}

bool PermissionPopupManager::SetPopupText(const std::string& message) {
  if (message.empty())
    return false;
  std::string text = message;
  std::string replaceStr = std::string("\n");
  size_t pos = text.find(replaceStr.c_str());
  if (pos != std::string::npos)
    text.replace(pos, replaceStr.length(), "</br>");
  elm_object_text_set(popup_, text.c_str());
  return true;
}

void PermissionPopupManager::AddButtonToPopup(const char* name,
                                              const char* text,
                                              Evas_Smart_Cb callback,
                                              bool focus) {
  Evas_Object* button = elm_button_add(popup_);
  elm_object_style_set(button, "popup");
  elm_object_domain_translatable_part_text_set(button, NULL, "WebKit", text);
  elm_object_part_content_set(popup_, name, button);
  evas_object_smart_callback_add(button, "clicked", callback, this);
  if (focus)
    evas_object_focus_set(button, true);
}
/* LCOV_EXCL_STOP */
