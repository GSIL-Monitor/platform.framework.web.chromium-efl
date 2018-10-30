// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "window_ui.h"

#include <assert.h>
#include <Elementary.h>
#include <EWebKit.h>
#include <EWebKit_internal.h>
#include <EWebKit_product.h>

#include "browser.h"
#include "logger.h"
#include "window.h"

#include "tizen/system_info.h"

namespace {
static std::string kDefaultNewWindowURL = "http://www.google.com";
static double kMinViewScale = 1.0;
static double kMaxViewScale = 4.0;
static int kNotificationTimeoutSec = 3;
}

WindowUI::WindowUI(Window& window, Browser& browser)
    : window_(window),
      browser_(browser),
      navbar_(nullptr),
      urlbar_(nullptr),
      url_entry_(nullptr),
      stop_reload_button_(nullptr),
      progress_bar_(nullptr),
      forward_button_(nullptr),
      back_button_(nullptr),
      active_popup_(nullptr),
      menu_(nullptr),
#if defined(OS_TIZEN_TV_PRODUCT)
      should_show_label_(true),
#else
      should_show_label_(browser_.IsDesktop()),
#endif
      is_loading_(false),
      url_entry_color_() {
  if (browser_.GetGUILevel() >= Browser::UBROWSER_GUI_LEVEL_URL_ONLY)
    CreateTopBar();
  if (browser_.GetGUILevel() >= Browser::UBROWSER_GUI_LEVEL_MINIMAL)
    CreateBottomBar();

  // TODO: Workaround for http://suprem.sec.samsung.net/jira/browse/TWF-843
  if (IsTvProfile()) {
    should_show_label_ = true;
    Elm_Theme* theme = elm_theme_new();
    elm_theme_set(theme, "tizen-HD-light");
    elm_object_theme_set(navbar_, theme);
    elm_theme_free(theme);
  }
}

WindowUI::~WindowUI() {
  if (active_popup_)
    evas_object_del(active_popup_);
}

void WindowUI::OnURLChanged(const char* url) {
  log_trace("%s: %s", __PRETTY_FUNCTION__, url);
  if (url_entry_)
    elm_object_text_set(url_entry_, url);
}

void WindowUI::OnLoadingStarted() {
  log_trace("%s", __PRETTY_FUNCTION__);
  if (progress_bar_)
    elm_progressbar_pulse(progress_bar_, EINA_TRUE);
  if (stop_reload_button_) {
    Evas_Object* icon = elm_object_part_content_get(stop_reload_button_,
                                                    "icon");
    elm_icon_standard_set(icon, "close");
    if (should_show_label_)
      elm_object_text_set(stop_reload_button_, "Stop");
  }
  is_loading_ = true;
}

void WindowUI::OnLoadingFinished() {
  log_trace("%s", __PRETTY_FUNCTION__);
  if (progress_bar_)
    elm_progressbar_pulse(progress_bar_, EINA_FALSE);
  if (stop_reload_button_) {
    Evas_Object* icon = elm_object_part_content_get(stop_reload_button_,
                                                    "icon");
    elm_icon_standard_set(icon, "refresh");
    if (should_show_label_)
      elm_object_text_set(stop_reload_button_, "Reload");
  }
  is_loading_ = false;
}

void WindowUI::EnableBackButton(bool enable) {
  if (back_button_)
    elm_object_disabled_set(back_button_, !enable);
}

void WindowUI::EnableForwardButton(bool enable) {
  if (forward_button_)
    elm_object_disabled_set(forward_button_, !enable);
}

void WindowUI::ShowNotification(const char* text) {
  log_trace("%s : %s", __PRETTY_FUNCTION__, text);
  Evas_Object* notification = elm_notify_add(window_.GetEvasObject());
  elm_notify_timeout_set(notification, kNotificationTimeoutSec);
  Evas_Object* content = elm_label_add(notification);
  elm_object_text_set(content, text);
  elm_object_content_set(notification, content);
  elm_notify_align_set(notification, 0.5, 0.5);
  evas_object_show(notification);
}

void WindowUI::CreateTopBar() {
  urlbar_ = elm_box_add(window_.GetEvasObject());
  elm_box_horizontal_set(urlbar_, EINA_TRUE);
  evas_object_size_hint_weight_set(urlbar_, EVAS_HINT_EXPAND, 0.0f);
  evas_object_size_hint_align_set(urlbar_, EVAS_HINT_FILL, EVAS_HINT_FILL);

  if (browser_.GetGUILevel() >= Browser::UBROWSER_GUI_LEVEL_ALL) {
    progress_bar_ = elm_progressbar_add(window_.GetEvasObject());
    elm_object_style_set(progress_bar_, "wheel");
    elm_progressbar_pulse_set(progress_bar_, EINA_TRUE);
    elm_progressbar_pulse(progress_bar_, EINA_FALSE);
    elm_box_pack_end(urlbar_, progress_bar_);
    evas_object_show(progress_bar_);
  }

  url_entry_ = elm_entry_add(urlbar_);
  elm_entry_single_line_set(url_entry_, EINA_TRUE);
  elm_entry_scrollable_set(url_entry_, EINA_TRUE);
  elm_entry_input_panel_layout_set(url_entry_, ELM_INPUT_PANEL_LAYOUT_URL);
  evas_object_size_hint_weight_set(url_entry_, EVAS_HINT_EXPAND, 1.0f);
  evas_object_size_hint_align_set(url_entry_, EVAS_HINT_FILL, EVAS_HINT_FILL);
  evas_object_smart_callback_add(url_entry_, "activated", OnURLEntered, this);
  elm_box_pack_end(urlbar_, url_entry_);
  evas_object_color_get(url_entry_, &url_entry_color_.r, &url_entry_color_.g,
                        &url_entry_color_.b, &url_entry_color_.a);
  evas_object_show(url_entry_);

  if (browser_.GetGUILevel() >= Browser::UBROWSER_GUI_LEVEL_ALL) {
    Evas_Object* separator = elm_separator_add(urlbar_);
    elm_separator_horizontal_set(separator, EINA_TRUE);
    elm_box_pack_end(urlbar_, separator);
    evas_object_show(separator);
  }
}

void WindowUI::CreateBottomBar() {
  navbar_ = elm_box_add(window_.GetEvasObject());
  elm_box_horizontal_set(navbar_, EINA_TRUE);
  elm_box_homogeneous_set(navbar_, EINA_TRUE);
  evas_object_size_hint_weight_set(navbar_, EVAS_HINT_EXPAND, 0.0f);
  evas_object_size_hint_align_set(navbar_, EVAS_HINT_FILL, EVAS_HINT_FILL);

  if (browser_.GetGUILevel() >= Browser::UBROWSER_GUI_LEVEL_ALL) {
    AddButton(navbar_, "file", "New Window", &WindowUI::OnCreateWindow);
    if (browser_.IsDesktop())
      AddButton(navbar_, "file", "New Incognito Window",
                &WindowUI::OnCreateIncognitoWindow);
  }

  back_button_ = AddButton(navbar_, "arrow_left", "Back", &WindowUI::OnBack);
  forward_button_ = AddButton(navbar_, "arrow_right", "Forward",
                              &WindowUI::OnForward);
  stop_reload_button_ = AddButton(navbar_, "refresh", "Reload",
                                  &WindowUI::OnStopOrReload);
  if (browser_.GetGUILevel() >= Browser::UBROWSER_GUI_LEVEL_ALL)
    AddButton(navbar_, "arrow_up", "More Actions",
              &WindowUI::OnShowExtraActionsMenu);

  EnableBackButton(false);
  EnableForwardButton(false);
}

Evas_Object* WindowUI::AddButton(Evas_Object* parent,
    const char* icon, const char* label, Evas_Smart_Cb cb) {
  Evas_Object* bt = elm_button_add(parent);
  evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
  evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, 0.5);
  if (label && should_show_label_)
    elm_object_text_set(bt, label);
  if (icon) {
    Evas_Object* icon_elm = elm_icon_add(bt);
    elm_icon_standard_set(icon_elm, icon);
    elm_object_part_content_set(bt, "icon", icon_elm);
  }
  evas_object_smart_callback_add(bt, "clicked", cb, this);
  elm_box_pack_end(parent, bt);
  evas_object_show(bt);

  return bt;
}

namespace {

void _HideCtxPopup(void* data, Evas_Object* obj, void*) {
  evas_object_hide(obj);
}

} // namespace

void WindowUI::CloseMenu() {
  evas_object_hide(menu_);
  evas_object_del(menu_);
  menu_ = 0;
}

Evas_Object* WindowUI::CreateExtraActionsMenu(Evas_Object* parent) {
  if (menu_) {
    CloseMenu();
    return 0;
  }
  menu_ =  elm_ctxpopup_add(parent);

  if ((IsMobileProfile() || IsWearableProfile())) {
    int width;
    evas_object_geometry_get(parent, 0, 0, &width, 0);
    evas_object_size_hint_min_set(menu_, width, 0);
  }
  evas_object_smart_callback_add(menu_, "dismissed", _HideCtxPopup, NULL);

  if (window_.IsFormProfileEnabled()) {
    elm_ctxpopup_item_append(menu_, "Disable Form Profile", NULL,
                             &WindowUI::OnFormProfileChange, this);
  } else {
    elm_ctxpopup_item_append(menu_, "Enable Form Profile", NULL,
                             &WindowUI::OnFormProfileChange, this);
  }

  if (window_.IsRememberFormDataEnabled()) {
    elm_ctxpopup_item_append(menu_, "Disable Remember Form Data", NULL,
                             &WindowUI::OnRememberFormDataChange, this);
  } else {
    elm_ctxpopup_item_append(menu_, "Enable Remember Form Data", NULL,
                             &WindowUI::OnRememberFormDataChange, this);
  }

  if (window_.IsRememberPasswordEnabled()) {
    elm_ctxpopup_item_append(menu_, "Disable Remember Password", NULL,
                             &WindowUI::OnRememberPasswordChange, this);
  } else {
    elm_ctxpopup_item_append(menu_, "Enable Remember Password", NULL,
                             &WindowUI::OnRememberPasswordChange, this);
  }

  elm_ctxpopup_item_append(menu_, "Override User Agent String", NULL,
                           &WindowUI::OnUAOverride, this);

  if (browser_.IsDesktop()) {
    elm_ctxpopup_item_append(menu_, "Show Web Inspector", NULL,
                             &WindowUI::OnInspectorShow, this);
  } else {
    elm_ctxpopup_item_append(menu_, "Start Web Inspector server", NULL,
                             &WindowUI::OnInspectorServerStart, this);
  }

  elm_ctxpopup_item_append(menu_, "Use Google data proxy", NULL,
                           &WindowUI::OnUseGoogleDataProxy, this);
#if !defined(OS_TIZEN)
  elm_ctxpopup_item_append(menu_, "Simulate screen rotation", NULL,
      &WindowUI::OnRotate, this);
#endif

  if (window_.AreTouchEventsEnabled()) {
    elm_ctxpopup_item_append(menu_, "Enable mouse events", NULL,
                             &WindowUI::OnSelectMouseInput, this);
  } else {
    elm_ctxpopup_item_append(menu_, "Enable touch events", NULL,
                             &WindowUI::OnSelectTouchInput, this);
  }

  if (!browser_.IsTracingEnabled()) {
    elm_ctxpopup_item_append(menu_, "Start tracing", NULL,
                             &WindowUI::OnStartTracing, this);
  } else {
    elm_ctxpopup_item_append(menu_, "Stop tracing", NULL,
                             &WindowUI::OnStopTracing, this);
  }

  Ewk_Settings* settings = window_.GetEwkSettings();
  if (!ewk_settings_auto_fitting_get(settings)) {
    elm_ctxpopup_item_append(menu_, "Enable auto fitting", NULL,
                             &WindowUI::OnAutoFittingEnabled, this);
  } else {
    elm_ctxpopup_item_append(menu_, "Disable auto fitting", NULL,
                             &WindowUI::OnAutoFittingDisabled, this);
  }

  if (!ewk_settings_scripts_can_open_windows_get(settings)) {
    elm_ctxpopup_item_append(menu_, "Enable js window.open", NULL,
                             &WindowUI::OnScriptWindowOpenEnabled, this);
  } else {
    elm_ctxpopup_item_append(menu_, "Disable js window.open", NULL,
                             &WindowUI::OnScriptWindowOpenDisabled, this);
  }

  if (!ewk_settings_form_profile_data_enabled_get(settings) ||
     !ewk_settings_form_candidate_data_enabled_get(settings)) {
    elm_ctxpopup_item_append(menu_, "Enable autofill", NULL,
                             &WindowUI::OnAutoFillEnabled, this);
  } else {
    elm_ctxpopup_item_append(menu_, "Disable autofill", NULL,
                             &WindowUI::OnAutoFillDisabled, this);
  }

  elm_ctxpopup_item_append(menu_, "Change page zoom level", NULL,
                           &WindowUI::OnShowZoomPopup, this);

  elm_ctxpopup_item_append(menu_, "Quit", NULL, &WindowUI::Exit, this);

  return menu_;
}

void WindowUI::ShowPermissionPopup(const char* title,
    const char* description, PermissionCallback cb, void* data) {
  log_trace("%s: (%s, %s)", __PRETTY_FUNCTION__, title, description);

  Evas_Object* popup = elm_popup_add(window_.GetEvasObject());
  elm_object_part_text_set(popup, "title,text", title);
  elm_object_part_text_set(popup, NULL, description);
  elm_popup_orient_set(popup, ELM_POPUP_ORIENT_CENTER);
  active_popup_ = popup;

  Evas_Object* btn = elm_button_add(popup);
  elm_object_text_set(btn, "Yes");
  PermissionPopupButtonData* buttonData = new PermissionPopupButtonData;
  buttonData->callbackData=data;
  buttonData->callback=cb;
  buttonData->popupData=this;

  evas_object_smart_callback_add(btn, "clicked", PermissionGranted,
                                 buttonData);
  elm_object_part_content_set(popup, "button1", btn);

  btn = elm_button_add(popup);
  elm_object_text_set(btn, "No");
  evas_object_smart_callback_add(btn, "clicked", PermissionDeclined,
                                 buttonData);
  elm_object_part_content_set(popup, "button2", btn);

  evas_object_show(popup);
}

void WindowUI::OnThemeColorChanged(int r, int g, int b, int a) {
  log_trace("%s: rgba(%d, %d, %d, %d)", __PRETTY_FUNCTION__, r, g, b, a);
  if (url_entry_) {
    // FIXME: For darker colors font color is to close to background color in
    // entry. It is style dependent, so style modification or change would be
    // needed to solve that issue.
    if (a != 0)
      evas_object_color_set(url_entry_, r, g, b, a);
    else
      evas_object_color_set(url_entry_, url_entry_color_.r, url_entry_color_.g,
                            url_entry_color_.b, url_entry_color_.a);
  }
}

void WindowUI::PermissionGranted(void* data, Evas_Object* button, void*) {
  log_trace("%s", __PRETTY_FUNCTION__);

  PermissionPopupButtonData* buttonData =(PermissionPopupButtonData*) data;
  ClosePopup(buttonData->popupData, NULL, NULL);
  buttonData->callback(true, buttonData->callbackData);
  delete buttonData;
}

void WindowUI::PermissionDeclined(void* data, Evas_Object* button, void*) {
  log_trace("%s", __PRETTY_FUNCTION__);

  PermissionPopupButtonData* buttonData =(PermissionPopupButtonData*) data;
  ClosePopup(buttonData->popupData, NULL, NULL);
  buttonData->callback(false, buttonData->callbackData);
  delete buttonData;
}

void WindowUI::ShowInspectorURLPopup(const char* url) {
  log_trace("%s : %s", __PRETTY_FUNCTION__, url);
  Evas_Object* popup = elm_popup_add(window_.GetEvasObject());
  elm_object_part_text_set(popup, "title,text", url);
  elm_popup_orient_set(popup, ELM_POPUP_ORIENT_CENTER);
  active_popup_ = popup;

  Evas_Object* btn = elm_button_add(popup);
  elm_object_text_set(btn, "Close");
  evas_object_smart_callback_add(btn, "clicked", ClosePopup, this);
  elm_object_part_content_set(popup, "button1", btn);

  evas_object_show(popup);
}

void WindowUI::ShowTextEntryPopup(const char* title,
                                  const char* initial_content,
                                  Evas_Smart_Cb cb) {
  log_trace("%s: (%s, %s)", __PRETTY_FUNCTION__, title, initial_content);

  Evas_Object* popup = elm_popup_add(window_.GetEvasObject());
  elm_object_part_text_set(popup, "title,text", title);
  elm_popup_orient_set(popup, ELM_POPUP_ORIENT_CENTER);
  active_popup_ = popup;

  Evas_Object* entry = elm_entry_add(popup);
  elm_entry_single_line_set(entry, EINA_TRUE);
  elm_entry_scrollable_set(entry, EINA_TRUE);
  elm_entry_input_panel_layout_set(entry, ELM_INPUT_PANEL_LAYOUT_URL);
  evas_object_size_hint_weight_set(entry, EVAS_HINT_EXPAND, 0.0f);
  evas_object_size_hint_align_set(entry, EVAS_HINT_FILL, EVAS_HINT_FILL);
  evas_object_smart_callback_add(
      entry, "activated", cb,
      new std::pair<Evas_Object*, WindowUI*>(entry, this));

  elm_object_part_content_set(popup, "default", entry);
  if (initial_content) {
    elm_object_text_set(entry, initial_content);
    elm_entry_cursor_end_set(entry);
  }

  Evas_Object* btn = elm_button_add(popup);
  elm_object_text_set(btn, "OK");
  evas_object_smart_callback_add(
      btn, "clicked", cb, new std::pair<Evas_Object*, WindowUI*>(entry, this));
  elm_object_part_content_set(popup, "button1", btn);

  btn = elm_button_add(popup);
  elm_object_text_set(btn, "Cancel");
  evas_object_smart_callback_add(btn, "clicked", ClosePopup, this);
  elm_object_part_content_set(popup, "button2", btn);

  evas_object_show(popup);
}

void WindowUI::OnCreateWindow(void* data, Evas_Object*, void*) {
  log_trace("%s", __PRETTY_FUNCTION__);
  WindowUI* thiz = static_cast<WindowUI*>(data);
  if (thiz->browser_.IsDesktop()) {
    thiz->browser_.CreateWindow().LoadURL(kDefaultNewWindowURL);
  } else {
    thiz->browser_.ActivateToolboxWindow();
  }
}

void WindowUI::OnCreateIncognitoWindow(void* data, Evas_Object*, void*) {
  log_trace("%s", __PRETTY_FUNCTION__);
  WindowUI* thiz = static_cast<WindowUI*>(data);
  thiz->browser_.CreateWindow(true).LoadURL(kDefaultNewWindowURL);
}

void WindowUI::OnBack(void* data, Evas_Object*, void*) {
  log_trace("%s", __PRETTY_FUNCTION__);
  static_cast<WindowUI*>(data)->window_.Back();
}

void WindowUI::OnForward(void* data, Evas_Object*, void*) {
  log_trace("%s", __PRETTY_FUNCTION__);
  static_cast<WindowUI*>(data)->window_.Forward();
}

void WindowUI::OnStopOrReload(void* data, Evas_Object*, void*) {
  log_trace("%s", __PRETTY_FUNCTION__);
  WindowUI* thiz = static_cast<WindowUI*>(data);
  if (thiz->is_loading_)
    thiz->window_.Stop();
  else
    thiz->window_.Reload();
}

void WindowUI::OnShowExtraActionsMenu(void* data, Evas_Object* obj, void*) {
  log_trace("%s", __PRETTY_FUNCTION__);
  WindowUI* thiz = static_cast<WindowUI*>(data);
  Evas_Object* popup = thiz->CreateExtraActionsMenu(
      thiz->window_.GetEvasObject());

  if (!popup)
    return;

  int x, y;
  evas_object_geometry_get(obj, &x, &y, 0, 0);
  evas_object_move(popup, x, y);
  evas_object_show(popup);
}

void WindowUI::OnURLEntered(void* data, Evas_Object*, void*) {
  log_trace("%s", __PRETTY_FUNCTION__);
  WindowUI* thiz = static_cast<WindowUI*>(data);
  thiz->window_.LoadURL(elm_object_text_get(thiz->url_entry_));
  elm_object_focus_set(thiz->url_entry_, EINA_FALSE);
}

void WindowUI::OnUAOverride(void* data, Evas_Object* obj, void*) {
  log_trace("%s", __PRETTY_FUNCTION__);
  WindowUI* thiz = static_cast<WindowUI*>(data);
  const char* ua = thiz->window_.GetUserAgent();
  thiz->ShowTextEntryPopup("User Agent Override", ua,
                           &WindowUI::OnUAOverrideEntered);
  thiz->CloseMenu();
}

void WindowUI::OnUAOverrideEntered(void* data, Evas_Object*, void*) {
  std::pair<Evas_Object*, WindowUI*>* p =
      static_cast<std::pair<Evas_Object*, WindowUI*>*>(data);
  p->second->window_.SetUserAgent(elm_object_text_get(p->first));
  evas_object_del(p->second->active_popup_);
  p->second->active_popup_ = NULL;
  delete p;
}

void WindowUI::OnInspectorShow(void* data, Evas_Object* obj, void*) {
  log_trace("%s", __PRETTY_FUNCTION__);
  WindowUI* thiz = static_cast<WindowUI*>(data);
  thiz->browser_.ShowInspectorWindow();
  thiz->CloseMenu();
}

void WindowUI::OnInspectorServerStart(void* data, Evas_Object* obj, void*) {
    log_trace("%s", __PRETTY_FUNCTION__);
    WindowUI* thiz = static_cast<WindowUI*>(data);
    thiz->browser_.StartInspectorServer();
    thiz->CloseMenu();
}

void WindowUI::OnUseGoogleDataProxy(void* data, Evas_Object* obj, void*) {
  log_trace("%s", __PRETTY_FUNCTION__);
  WindowUI* thiz = static_cast<WindowUI*>(data);
  ewk_context_proxy_set(ewk_context_default_get(), "compress.googlezip.net:80",
                        "<local>");
  thiz->window_.SetGoogleDataProxyHeaders();
  thiz->CloseMenu();
}

#if !defined(OS_TIZEN)
void WindowUI::OnRotate(void* data, Evas_Object* obj, void*) {
  log_trace("%s", __PRETTY_FUNCTION__);
  WindowUI* thiz = static_cast<WindowUI*>(data);
  thiz->window_.FakeRotate();
  thiz->CloseMenu();
}
#endif

void WindowUI::OnSelectMouseInput(void* data, Evas_Object* obj, void*) {
  log_trace("%s", __PRETTY_FUNCTION__);
  WindowUI* thiz = static_cast<WindowUI*>(data);
  thiz->window_.EnableTouchEvents(false);
  thiz->window_.EnableMouseEvents(true);
  thiz->ShowNotification("Mouse events enabled");
  thiz->CloseMenu();
}

void WindowUI::OnSelectTouchInput(void* data, Evas_Object* obj, void*) {
  log_trace("%s", __PRETTY_FUNCTION__);
  WindowUI* thiz = static_cast<WindowUI*>(data);
  thiz->window_.EnableTouchEvents(true);
  thiz->window_.EnableMouseEvents(false);
  thiz->ShowNotification("Touch events enabled");
  thiz->CloseMenu();
}

void WindowUI::OnStartTracing(void* data, Evas_Object* obj, void*) {
  log_trace("%s", __PRETTY_FUNCTION__);
  WindowUI* thiz = static_cast<WindowUI*>(data);
  thiz->browser_.StartTracing();
  thiz->CloseMenu();
  thiz->ShowNotification("Tracing started");
}

void WindowUI::OnStopTracing(void* data, Evas_Object* obj, void*) {
  log_trace("%s", __PRETTY_FUNCTION__);
  WindowUI* thiz = static_cast<WindowUI*>(data);
  thiz->browser_.StopTracing();
  thiz->CloseMenu();
  thiz->ShowNotification("Tracing finished");
}

void WindowUI::OnAutoFittingEnabled(void* data, Evas_Object* obj, void*) {
  log_trace("%s", __PRETTY_FUNCTION__);
  WindowUI* thiz = static_cast<WindowUI*>(data);
  Ewk_Settings* settings = thiz->window_.GetEwkSettings();
  ewk_settings_auto_fitting_set(settings, true);
  thiz->CloseMenu();
  thiz->ShowNotification("Auto fitting enabled");
}

void WindowUI::OnAutoFittingDisabled(void* data, Evas_Object* obj, void*) {
  log_trace("%s", __PRETTY_FUNCTION__);
  WindowUI* thiz = static_cast<WindowUI*>(data);
  Ewk_Settings* settings = thiz->window_.GetEwkSettings();
  ewk_settings_auto_fitting_set(settings, false);
  thiz->CloseMenu();
  thiz->ShowNotification("Auto fitting disabled");
}

void WindowUI::OnAutoFillEnabled(void* data, Evas_Object* obj, void*) {
  log_trace("%s", __PRETTY_FUNCTION__);
  WindowUI* thiz = static_cast<WindowUI*>(data);
  Ewk_Settings* settings = thiz->window_.GetEwkSettings();
  ewk_settings_form_profile_data_enabled_set(settings, true);
  ewk_settings_form_candidate_data_enabled_set(settings, true);
  thiz->CloseMenu();
  thiz->ShowNotification("Autofill enabled");
}

void WindowUI::OnAutoFillDisabled(void* data, Evas_Object* obj, void*) {
  log_trace("%s", __PRETTY_FUNCTION__);
  WindowUI* thiz = static_cast<WindowUI*>(data);
  Ewk_Settings* settings = thiz->window_.GetEwkSettings();
  ewk_settings_form_profile_data_enabled_set(settings, false);
  ewk_settings_form_candidate_data_enabled_set(settings, false);
  thiz->CloseMenu();
  thiz->ShowNotification("Autofill disabled");
}

void WindowUI::OnScriptWindowOpenEnabled(void* data, Evas_Object* obj, void*) {
  log_trace("%s", __PRETTY_FUNCTION__);
  WindowUI* thiz = static_cast<WindowUI*>(data);
  Ewk_Settings* settings = thiz->window_.GetEwkSettings();
  ewk_settings_scripts_can_open_windows_set(settings, true);
  thiz->CloseMenu();
  thiz->ShowNotification("JS window.open enabled");
}

void WindowUI::OnScriptWindowOpenDisabled(void* data, Evas_Object* obj, void*) {
  log_trace("%s", __PRETTY_FUNCTION__);
  WindowUI* thiz = static_cast<WindowUI*>(data);
  Ewk_Settings* settings = thiz->window_.GetEwkSettings();
  ewk_settings_scripts_can_open_windows_set(settings, false);
  thiz->CloseMenu();
  thiz->ShowNotification("JS window.open disabled");
}

void WindowUI::ClosePopup(void* data, Evas_Object*, void*) {
  WindowUI* thiz = static_cast<WindowUI*>(data);
  evas_object_del(thiz->active_popup_);
  thiz->active_popup_ = NULL;
}

void WindowUI::ClosePopup() {
  if (active_popup_) {
    evas_object_del(active_popup_);
    active_popup_ = NULL;
  }
}

void WindowUI::OnShowZoomPopup(void* data, Evas_Object* obj, void*) {
  log_trace("%s", __PRETTY_FUNCTION__);
  WindowUI* thiz = static_cast<WindowUI*>(data);

  Evas_Object* popup = elm_popup_add(thiz->window_.GetEvasObject());
  elm_object_part_text_set(popup, "title,text", "Change page zoom level");
  elm_popup_orient_set(popup, ELM_POPUP_ORIENT_CENTER);
  thiz->active_popup_ = popup;
  Evas_Object* btn = elm_button_add(popup);

  Evas_Object* slider = elm_slider_add(popup);
  thiz->window_.GetScaleRange(&kMinViewScale, &kMaxViewScale);
  elm_slider_min_max_set(slider, kMinViewScale, kMaxViewScale);
  elm_slider_value_set(slider, thiz->window_.GetScale());
  elm_slider_indicator_format_set(slider, "%1.2f");
  elm_slider_indicator_show_set(slider, EINA_TRUE);
  evas_object_smart_callback_add(slider, "changed", &OnZoomChanged, thiz);
  elm_object_part_content_set(popup, "default", slider);

  elm_object_text_set(btn, "Close");
  evas_object_smart_callback_add(btn, "clicked", ClosePopup, thiz);
  elm_object_part_content_set(popup, "button1", btn);

  evas_object_show(popup);
  thiz->CloseMenu();
}

void WindowUI::OnZoomChanged(void* data, Evas_Object* obj, void*) {
  log_trace("%s", __PRETTY_FUNCTION__);
  WindowUI* thiz = static_cast<WindowUI*>(data);
  thiz->window_.SetScale(elm_slider_value_get(obj));
}

void WindowUI::OnRememberFormDataChange(void* data, Evas_Object* obj, void*) {
  log_trace("%s", __PRETTY_FUNCTION__);
  WindowUI* thiz = static_cast<WindowUI*>(data);
  Ewk_Settings* settings = thiz->window_.GetEwkSettings();
  if (thiz->window_.IsRememberFormDataEnabled()) {
    ewk_settings_form_candidate_data_enabled_set(settings, false);
    thiz->ShowNotification("Remember Form Data disabled");
  } else {
    ewk_settings_form_candidate_data_enabled_set(settings, true);
    thiz->ShowNotification("Remember Form Data enabled");
  }
  thiz->CloseMenu();
}

void WindowUI::OnRememberPasswordChange(void* data, Evas_Object* obj, void*) {
  log_trace("%s", __PRETTY_FUNCTION__);
  WindowUI* thiz = static_cast<WindowUI*>(data);
  Ewk_Settings* settings = thiz->window_.GetEwkSettings();
  if (thiz->window_.IsRememberPasswordEnabled()) {
    ewk_settings_autofill_password_form_enabled_set(settings, false);
    thiz->ShowNotification("Remember Password disabled");
  } else {
    ewk_settings_autofill_password_form_enabled_set(settings, true);
    thiz->ShowNotification("Remember Password enabled");
  }
  thiz->CloseMenu();
}

void WindowUI::OnFormProfileChange(void* data, Evas_Object* obj, void*) {
  log_trace("%s", __PRETTY_FUNCTION__);
  WindowUI* thiz = static_cast<WindowUI*>(data);
  Ewk_Settings* settings = thiz->window_.GetEwkSettings();
  if (thiz->window_.IsFormProfileEnabled()) {
    ewk_settings_form_profile_data_enabled_set(settings, false);
    thiz->ShowNotification("Form Profile disabled");
  } else {
    ewk_settings_form_profile_data_enabled_set(settings, true);
    thiz->ShowNotification("Form Profile enabled");
  }
  thiz->CloseMenu();
}

void WindowUI::Exit(void* data, Evas_Object*, void*) {
  WindowUI* thiz = static_cast<WindowUI*>(data);
  thiz->window_.Exit();
}
