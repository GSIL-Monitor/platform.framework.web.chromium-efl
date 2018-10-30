// Copyright 2016-2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ewk/efl_integration/authentication_challenge_popup.h"

#if defined(OS_TIZEN)
#include <efl_extension.h>
#endif

#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "content/common/paths_efl.h"
#include "ewk/efl_integration/eweb_view.h"
#include "public/ewk_view.h"

// Color of text_box border
static const int kBorderColorRed = 10;
static const int kBorderColorGreen = 10;
static const int kBorderColorBlue = 10;

// Length of text_box border
static const int kBorderWidth = 200;
static const int kBorderHeight = 30;
static const int kBorderThickness = 3;

AuthenticationChallengePopup::AuthenticationChallengePopup(
    Ewk_Auth_Challenge* auth_challenge,
    Evas_Object* ewk_view)
    : conformant_(nullptr),
      ewk_view_(ewk_view),
      login_button_(nullptr),
      layout_(nullptr),
      password_entry_(nullptr),
      popup_(nullptr),
      user_name_entry_(nullptr),
      auth_challenge_(auth_challenge) {}

AuthenticationChallengePopup::~AuthenticationChallengePopup() {
  if (login_button_)
    evas_object_smart_callback_del(login_button_, "clicked",
                                   AuthenticationLoginCallback);
#if defined(OS_TIZEN)
  if (popup_) {
    eext_object_event_callback_del(popup_, EEXT_CALLBACK_BACK,
                                   HwBackKeyCallback);
  }
#endif
  if (conformant_) {
    evas_object_del(conformant_);
    conformant_ = nullptr;
  }
}

void AuthenticationChallengePopup::AuthenticationLoginCallback(
    void* data,
    Evas_Object* obj,
    void* event_info) {
  auto auth_challenge_popup = static_cast<AuthenticationChallengePopup*>(data);
  if (!auth_challenge_popup) {
    LOG(ERROR) << "Showing authentication popup failed";
    return;
  }

  const char* username =
      elm_entry_entry_get(auth_challenge_popup->user_name_entry_);
  const char* password =
      elm_entry_entry_get(auth_challenge_popup->password_entry_);
  ewk_auth_challenge_credential_use(auth_challenge_popup->auth_challenge_,
                                    username, password);
  delete auth_challenge_popup;
}

#if defined(OS_TIZEN)
void AuthenticationChallengePopup::HwBackKeyCallback(void* data,
                                                     Evas_Object* obj,
                                                     void* event_info) {
  auto* auth_challenge_popup = static_cast<AuthenticationChallengePopup*>(data);
  if (!auth_challenge_popup) {
    LOG(ERROR) << "Showing authentication popup failed";
    return;
  }
  ewk_auth_challenge_credential_cancel(auth_challenge_popup->auth_challenge_);
  delete auth_challenge_popup;
}
#endif

Evas_Object* AuthenticationChallengePopup::CreateBorder(
    const std::string& text,
    Evas_Object* entry_box) {
  Evas_Object* border = elm_bg_add(popup_);
  if (!border)
    return nullptr;

  elm_bg_color_set(border, kBorderColorRed, kBorderColorGreen,
                   kBorderColorBlue);
  if (!text.compare("border_up") || !text.compare("border_down"))
    evas_object_size_hint_min_set(border, kBorderWidth, kBorderThickness);
  else
    evas_object_size_hint_min_set(border, kBorderThickness, kBorderHeight);

  evas_object_show(border);
  elm_object_part_content_set(entry_box, text.c_str(), border);

  return border;
}

Evas_Object* AuthenticationChallengePopup::CreateTextBox(
    const char* edj_path,
    Evas_Object* container_box) {
  Evas_Object* entry_box = elm_layout_add(popup_);
  if (!entry_box)
    return nullptr;
  elm_layout_file_set(entry_box, edj_path, "authentication_challenge");

  Evas_Object* text_box = elm_entry_add(entry_box);
  if (!text_box)
    return nullptr;
  elm_entry_single_line_set(text_box, EINA_TRUE);
  elm_object_focus_set(text_box, EINA_TRUE);
  elm_entry_scrollable_set(text_box, true);
  evas_object_show(text_box);

  elm_object_part_content_set(entry_box, "elm.swallow.entry", text_box);

  CreateBorder("border_up", entry_box);
  CreateBorder("border_down", entry_box);
  CreateBorder("border_left", entry_box);
  CreateBorder("border_right", entry_box);

  elm_box_pack_end(container_box, entry_box);
  evas_object_show(entry_box);

  return text_box;
}

bool AuthenticationChallengePopup::CreateTextLabel(const char* text,
                                                   Evas_Object* container_box) {
  Evas_Object* text_label = elm_label_add(popup_);
  if (!text_label)
    return false;
  elm_object_text_set(text_label, text);
  elm_box_pack_end(container_box, text_label);
  evas_object_show(text_label);
  return true;
}

Evas_Object* AuthenticationChallengePopup::CreateContainerBox() {
  Evas_Object* box = elm_box_add(popup_);
  if (!box)
    return nullptr;
  evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
  evas_object_show(box);

  if (!CreateTextLabel(dgettext("WebKit", "IDS_WEBVIEW_BODY_USERNAME"), box))
    return nullptr;
  base::FilePath edj_dir, authentication_edj;
  PathService::Get(PathsEfl::EDJE_RESOURCE_DIR, &edj_dir);
  authentication_edj =
      edj_dir.Append(FILE_PATH_LITERAL("AuthenticationPopup.edj"));

  user_name_entry_ =
      CreateTextBox(authentication_edj.AsUTF8Unsafe().c_str(), box);
  if (!user_name_entry_)
    return nullptr;

  if (!CreateTextLabel(dgettext("WebKit", "IDS_WEBVIEW_BODY_PASSWORD"), box))
    return nullptr;

  password_entry_ =
      CreateTextBox(authentication_edj.AsUTF8Unsafe().c_str(), box);
  if (!password_entry_)
    return nullptr;
  elm_entry_password_set(password_entry_, EINA_TRUE);
  elm_entry_input_panel_layout_set(password_entry_,
                                   ELM_INPUT_PANEL_LAYOUT_PASSWORD);

  return box;
}

bool AuthenticationChallengePopup::ShowAuthenticationPopup(const char* url) {
  Evas_Object* parent =
      elm_object_top_widget_get(elm_object_parent_widget_get(ewk_view_));
  if (!parent)
    parent = ewk_view_;

  conformant_ = elm_conformant_add(parent);
  if (!conformant_)
    return false;

  evas_object_size_hint_weight_set(conformant_, EVAS_HINT_EXPAND,
                                   EVAS_HINT_EXPAND);
  elm_win_resize_object_add(parent, conformant_);

  layout_ = elm_layout_add(conformant_);
  if (!layout_)
    return false;
  elm_layout_theme_set(layout_, "layout", "application", "default");
  evas_object_size_hint_weight_set(layout_, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
  elm_object_content_set(conformant_, layout_);
  evas_object_show(layout_);

  popup_ = elm_popup_add(layout_);
  if (!popup_)
    return false;
  elm_object_part_text_set(popup_, "title,text", url);

  Evas_Object* box = CreateContainerBox();
  if (!box)
    return false;
  elm_object_content_set(popup_, box);
  login_button_ = elm_button_add(popup_);
  if (!login_button_)
    return false;
  elm_object_style_set(login_button_, "popup");
  evas_object_size_hint_weight_set(login_button_, EVAS_HINT_EXPAND,
                                   EVAS_HINT_EXPAND);
  evas_object_size_hint_align_set(login_button_, EVAS_HINT_FILL,
                                  EVAS_HINT_FILL);
  elm_object_text_set(login_button_, dgettext("WebKit", "IDS_BR_BODY_LOG_IN"));
  elm_box_pack_end(box, login_button_);
  elm_object_part_content_set(popup_, "button1", login_button_);
  evas_object_show(login_button_);

  evas_object_smart_callback_add(login_button_, "clicked",
                                 AuthenticationLoginCallback, this);
#if defined(OS_TIZEN)
  eext_object_event_callback_add(popup_, EEXT_CALLBACK_BACK, HwBackKeyCallback,
                                 this);
#endif
  evas_object_show(popup_);

  return true;
}

void AuthenticationChallengePopup::CreateAndShow(
    Ewk_Auth_Challenge* auth_challenge,
    Evas_Object* ewk_view) {
  if (!auth_challenge)
    return;
  auto popup = new AuthenticationChallengePopup(auth_challenge, ewk_view);

  const char* url = ewk_auth_challenge_url_get(auth_challenge);

  ewk_auth_challenge_suspend(auth_challenge);
  if (!popup->ShowAuthenticationPopup(url)) {
    LOG(ERROR) << "Showing authentication popup is failed";
    ewk_auth_challenge_credential_cancel(auth_challenge);
    delete popup;
  }
}
