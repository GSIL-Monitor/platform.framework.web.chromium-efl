// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <Eina.h>
#include <Elementary.h>
#include "ecore_x_wayland_wrapper.h"

#include "base/logging.h"
#include "popup_controller_efl.h"
#include "ui/base/clipboard/clipboard_helper_efl.h"

#if defined(OS_TIZEN)
// TODO: Webview and Rendering TG will implement following for Wayland.
#if !defined(WAYLAND_BRINGUP)
#include <ui-gadget.h>
#endif
#include <app_control.h>
#else
#include <gio/gio.h>
#endif // OS_TIZEN

#ifdef OS_TIZEN
#include <efl_extension.h>
#endif

namespace content {

std::vector<PopupControllerEfl::Context_Option> PopupControllerEfl::options_;

static void close_cb(void *data, Evas_Object *obj, void *event_info) {
  PopupControllerEfl* controller = static_cast<PopupControllerEfl*>(data);
  if (!controller)
    return;

  controller->closePopup();
}

void PopupControllerEfl::itemSelected(
    void *data,
    Evas_Object *obj,
    void *event_info) {
  PopupControllerEfl* controller = static_cast<PopupControllerEfl*>(data);

  LOG(INFO) << __PRETTY_FUNCTION__ << " content: "
      << controller->fullContent().c_str() << " type: "
      << controller->popupContentType();

  Elm_Object_Item* selected = static_cast<Elm_Object_Item*>(event_info);
  int index = elm_genlist_item_index_get(selected);
  // elm_genlist_item_index_get iterrate starting from 1
  index--;

  controller->closePopup();

  if (index < 0 || (unsigned int)index >= controller->options_.size()) {
    LOG(ERROR) << __PRETTY_FUNCTION__ << " : " << "invalid index.";
    return;
  }

#if defined(OS_TIZEN)
  app_control_h svcHandle = 0;
  if (app_control_create(&svcHandle) < 0 || !svcHandle) {
    LOG(ERROR) << __PRETTY_FUNCTION__ << " : " << "could not create service.";
    return;
  }

  if (controller->options_[index] == COPY) {
    ClipboardHelperEfl::GetInstance()->SetData(
        controller->content(), ClipboardDataTypeEfl::PLAIN_TEXT);
  } else if (controller->popupContentType() == PHONE) {
    if (controller->options_[index] == CALL) {
      app_control_set_operation(svcHandle, APP_CONTROL_OPERATION_DIAL);
      app_control_set_uri(svcHandle, controller->fullContent().c_str());
    } else if (controller->options_[index] == SEND_MESSAGE) {
      app_control_set_operation(svcHandle, APP_CONTROL_OPERATION_COMPOSE);
      app_control_add_extra_data(
          svcHandle,
          "http://tizen.org/appcontrol/data/messagetype",
          "sms");
      app_control_add_extra_data(
          svcHandle,
          APP_CONTROL_DATA_TO,
          controller->content().c_str());
      app_control_add_extra_data(
          svcHandle,
          "http://tizen.org/appcontrol/data/return_result",
          "false");
      app_control_set_app_id(svcHandle, "msg-composer-efl");
    } else if (controller->options_[index] == ADD_CONTACT) {
      app_control_set_operation(
          svcHandle,
          "http://tizen.org/appcontrol/operation/social/add");
      app_control_add_extra_data(
          svcHandle,
          "http://tizen.org/appcontrol/data/social/item_type",
          "contact");
      app_control_add_extra_data(
          svcHandle,
          "http://tizen.org/appcontrol/data/social/phone",
          controller->content().c_str());
      app_control_set_app_id(svcHandle, "contacts-details-efl");
    }
  } else if (controller->popupContentType() == EMAIL) {
    if (controller->options_[index] == SEND_EMAIL) {
      app_control_set_operation(svcHandle, APP_CONTROL_OPERATION_COMPOSE);
      app_control_add_extra_data(
          svcHandle,
          APP_CONTROL_DATA_TO,
          controller->content().c_str());
      app_control_set_app_id(svcHandle, "email-composer-efl");
    } else if (controller->options_[index] == ADD_CONTACT) {
      app_control_set_operation(
          svcHandle,
          "http://tizen.org/appcontrol/operation/social/add");
      app_control_add_extra_data(
          svcHandle,
          "http://tizen.org/appcontrol/data/social/item_type",
          "contact");
      app_control_add_extra_data(
          svcHandle,
          "http://tizen.org/appcontrol/data/social/email",
          controller->content().c_str());
      app_control_set_app_id(svcHandle, "contacts-details-efl");
    }
  }

  int ret = app_control_send_launch_request(svcHandle, 0, 0);
  if (ret != APP_CONTROL_ERROR_NONE)
    LOG(ERROR) << __PRETTY_FUNCTION__ << " : "
        << "could not launch application.";

  app_control_destroy(svcHandle);
#endif
}

PopupControllerEfl::PopupControllerEfl(EWebView* webView)
  : web_view_(webView),
    top_widget_(NULL),
    popup_(NULL),
    content_type_(UNKNOWN) {
}

char* PopupControllerEfl::contextMenuGenlistTextSet(
    void* data,
    Evas_Object* obj,
    const char* part) {
  size_t index = reinterpret_cast<size_t>(data);

  if (index >= options_.size())
    return 0;

  char* label = NULL;
  if (options_[index] == CALL)
    label = strdup(dgettext("WebKit", "IDS_BR_OPT_CALL"));
  else if (options_[index] == SEND_EMAIL)
    label = strdup(dgettext("WebKit", "IDS_BR_OPT_SEND_EMAIL"));
  else if (options_[index] == SEND_MESSAGE)
    label = strdup(dgettext("WebKit", "IDS_COM_BODY_SEND_MESSAGE"));
  else if (options_[index] == ADD_CONTACT)
    label = strdup(dgettext("WebKit", "IDS_BR_BODY_ADD_TO_CONTACT"));
  else if (options_[index] == COPY)
    label = strdup(dgettext("WebKit","IDS_WEBVIEW_OPT_COPY"));

  return label;
}

void PopupControllerEfl::openPopup(const char* message) {
  if (!message)
    return;

  full_content_ = std::string(message);
  content_ = std::string(message);
#if defined(OS_TIZEN)
  if (content_.find("mailto:") != std::string::npos) {
    content_.erase(0, 7);
    content_type_ = EMAIL;
  } else if (content_.find("tel:") != std::string::npos) {
    content_.erase(0, 4);
    content_type_ = PHONE;
  } else {
    LOG(INFO) << "Unknown content type: " << message;
    return;
  }

  addOptions();

  if (!createPopup())
    return;

  appendItems();
  addCallbacks();
  showPopup();
#endif
}

void PopupControllerEfl::addOptions() {
  options_.clear();

  if (content_type_ == EMAIL) {
    options_.push_back(SEND_EMAIL);
    options_.push_back(ADD_CONTACT);
    options_.push_back(COPY);
  } else if (content_type_ == PHONE) {
    options_.push_back(CALL);
    options_.push_back(SEND_MESSAGE);
    options_.push_back(ADD_CONTACT);
    options_.push_back(COPY);
  }
}

bool PopupControllerEfl::createPopup() {
  top_widget_ = web_view_->evas_object();

  if (!top_widget_)
    return false;

  popup_ = elm_popup_add(top_widget_);

  if (!popup_)
    return false;

  evas_object_data_set(popup_, "PopupControllerEfl", this);
  elm_object_part_text_set(
      popup_, "title,text", const_cast<char*>(content_.c_str()));

  evas_object_size_hint_weight_set(popup_, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

  return true;
}

void PopupControllerEfl::appendItems() {
  Evas_Object* genlist = elm_genlist_add(popup_);
  evas_object_data_set(genlist, "PopupControllerEfl", this);
  evas_object_size_hint_align_set(genlist, EVAS_HINT_FILL, EVAS_HINT_FILL);
  elm_scroller_content_min_limit(genlist, EINA_FALSE, EINA_TRUE);

  static Elm_Genlist_Item_Class _context_menu_list_item;
  _context_menu_list_item.item_style = "default";
  _context_menu_list_item.func.text_get = contextMenuGenlistTextSet;
  _context_menu_list_item.func.content_get = 0;
  _context_menu_list_item.func.state_get = 0;
  _context_menu_list_item.func.del = 0;

  for (unsigned i = 0; i < options_.size(); ++i) {
    elm_genlist_item_append(genlist,
                            &_context_menu_list_item,
                            reinterpret_cast<const void*>(i),
                            0,
                            ELM_GENLIST_ITEM_NONE,
                            itemSelected,
                            this);
  }

  evas_object_show(genlist);
  elm_object_content_set(popup_, genlist);
}

void PopupControllerEfl::addCallbacks() {
  // Close popup after tapping on outside of it
  evas_object_smart_callback_add(popup_, "block,clicked", close_cb, this);
#ifdef OS_TIZEN
  eext_object_event_callback_add(popup_, EEXT_CALLBACK_BACK, close_cb, this);
  evas_object_focus_set(popup_, EINA_TRUE);
#endif
}

void PopupControllerEfl::showPopup() {
  evas_object_show(popup_);
}

void PopupControllerEfl::closePopup() {
  if(top_widget_)
    evas_object_data_set(top_widget_, "ContextMenuContollerEfl", 0);

  if (popup_) {
    evas_object_data_set(popup_, "ContextMenuContollerEfl", 0);
    evas_object_del(popup_);
    popup_ = NULL;
  }
}

void PopupControllerEfl::SetPopupSize(int width, int height) {
  if (!popup_)
    return;

  evas_object_resize(popup_, width, height);
  evas_object_move(popup_, 0, 0);
}

} // namespace
