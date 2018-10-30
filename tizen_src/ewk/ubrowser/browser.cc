// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser.h"

#include <arpa/inet.h>
#include <assert.h>
#include <ifaddrs.h>
#include <net/if.h>

#include <EWebKit.h>
#include <EWebKit_internal.h>
#include <EWebKit_product.h>

#include "logger.h"
#include "tizen/system_info.h"

static std::string kHomePage = "http://www.google.com";
static int kDefaultMobileWindowWidth = 360;
static int kDefaultMobileWindowHeight = 640;
#if defined(OS_TIZEN_TV_PRODUCT)
static std::string kDefaultCookiePath = "/home/owner/.cookie";
static std::string kDefaultCertificatePath = "/opt/data/cert/vdca.pem";
static double kDefaultZoomFactor = 1.5;
static int kDefaultDesktopWindowWidth = 1920;
static int kDefaultDesktopWindowHeight = 1048;
#else
static int kDefaultDesktopWindowWidth = 1600;
static int kDefaultDesktopWindowHeight = 900;
#endif
static int kToolboxWindowWidth = 640;
static int kToolboxWindowHeight = 400;
static unsigned kInspectorServerPort = 7777;

#if defined(OS_TIZEN) && !defined(OS_TIZEN_TV_PRODUCT)
static const char* kLocationPrivilege = "http://tizen.org/privilege/location";
#endif

Window* Browser::selected_window_ = NULL;

static std::string GetConfigPath() {
#if defined(OS_TIZEN_TV_PRODUCT)
  return kDefaultCookiePath;
#elif defined(OS_TIZEN) && defined(TIZEN_APP)
  // Since ubrowser is not being created in Tizen specific way
  // app_get_data_path() returns null.
  // TODO: determine proper path using tizen API once TIZEN_APP
  // gets enabled.
  log_error("GetConfigPath is not implemented");
  return std::string();
#else
  std::string path;
  char* config_path = getenv("XDG_CONFIG_HOME");
  if (config_path) {
    path = config_path;
  } else {
    // Fallback code, on Mobile XDG_CONFIG_HOME is not set.
    char* home_path = getenv("HOME");
    if (!home_path) {
      log_error("Could not determine home directory");
      return std::string();
    }
    path = home_path + std::string("/.config");
  }
  return path + "/ubrowser";
#endif
}

Browser::Browser(bool desktop,
                 GUILevel gui_level,
                 std::string ua,
                 std::string tizen_version)
    : toolbox_window_(nullptr),
      window_list_(nullptr),
      desktop_(desktop),
      inspector_started_(false),
      tracing_enabled_(false),
      user_agent_(ua),
      tizen_version_(tizen_version),
#if defined(TIZEN_VIDEO_HOLE)
      video_hole_(false),
#endif
#if defined(OS_TIZEN)
      haptic_timer_id_(nullptr),
      haptic_handle_(nullptr),
      haptic_effect_(nullptr),
#endif
      gui_level_(gui_level) {
  log_info("UI type: %s", desktop_ ? "desktop" : "mobile");

  if (IsDesktopProfile()) {
    log_info("Runtime Profile : DESKTOP : ", IsDesktopProfile());
  } else if (IsMobileProfile()) {
    log_info("Runtime Profile : MOBILE : ", IsMobileProfile());
  } else if (IsTvProfile()) {
    log_info("Runtime Profile : TV : ", IsTvProfile());
  } else if (IsWearableProfile()) {
    log_info("Runtime Profile : WEARABLE : ", IsWearableProfile());
  } else if (IsIviProfile()) {
    log_info("Runtime Profile : IVI : ", IsIviProfile());
  } else if (IsCommonProfile()) {
    log_info("Runtime Profile : COMMON : ", IsCommonProfile());
  } else {
    log_info("Runtime Profile : UNKNOWN : ");
  }

  if (IsEmulatorArch()) {
    log_info("Runtime Architecture : EMULATOR : ", IsEmulatorArch());
  }

  // If we don't call ewk_context_default_get here, ubrowser crashes in desktop
  // mode. This is a hack.
  // FIXME: find a real fix
  Ewk_Context* ctx = ewk_context_default_get();

  // Developers usually kill ubrowser by Ctrl+C which ends
  // ecore main loop resulting in not call ewk_shutdown and
  // all jobs that saves something on disk do not take place.
  // Add SIGINT handler to properly close ubrowser app.
  signal(SIGINT, Browser::OnSignalHandler);

  std::string config_path = GetConfigPath();
  if (!config_path.empty()) {
    ewk_cookie_manager_persistent_storage_set(
        ewk_context_cookie_manager_get(ctx), config_path.c_str(),
        EWK_COOKIE_PERSISTENT_STORAGE_SQLITE);
  }

  ewk_context_cache_model_set(ctx, EWK_CACHE_MODEL_PRIMARY_WEBBROWSER);

#if defined(OS_TIZEN_TV_PRODUCT)
  ewk_context_default_zoom_factor_set(ctx, kDefaultZoomFactor);
  ewk_context_certificate_file_set(ctx, kDefaultCertificatePath.c_str());
  ewk_context_cache_disabled_set(ctx, EINA_FALSE);
#endif

  if (desktop_)
    return;

  ewk_context_vibration_client_callbacks_set(
    ctx, &Browser::OnVibrationStart, &Browser::OnVibrationStop, this);

  if (GetGUILevel() == UBROWSER_GUI_LEVEL_ALL) {
    toolbox_window_ = elm_win_util_standard_add(
        "ubrowser-toolbox", "uBrowser ToolBox");

    evas_object_resize(
        toolbox_window_, kToolboxWindowWidth, kToolboxWindowHeight);

    elm_win_autodel_set(toolbox_window_, EINA_TRUE);

    Evas_Object* box = elm_box_add(toolbox_window_);
    evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(toolbox_window_, box);
    evas_object_show(box);

    window_list_ = elm_list_add(toolbox_window_);
    elm_list_select_mode_set(window_list_, ELM_OBJECT_SELECT_MODE_ALWAYS);

    evas_object_size_hint_weight_set(
        window_list_, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

    evas_object_size_hint_align_set(
        window_list_, EVAS_HINT_FILL, EVAS_HINT_FILL);

    elm_box_pack_end(box, window_list_);
    evas_object_show(window_list_);

    Evas_Object* bbox = elm_box_add(toolbox_window_);
    elm_box_horizontal_set(bbox, EINA_TRUE);
    evas_object_size_hint_weight_set(bbox, EVAS_HINT_EXPAND, 0);
    evas_object_size_hint_align_set(bbox, EVAS_HINT_FILL, 0);
    elm_box_pack_end(box, bbox);
    evas_object_show(bbox);

    AddButton(bbox, "New Window", "add", NULL, OnNewWindowRequest);
    AddButton(
        bbox, "New Incognito Window", "add", NULL, OnNewIncognitoWindowRequest);

    if (elm_win_wm_rotation_supported_get(toolbox_window_)) {
      static const int rots[] = {0, 90, 270};
      elm_win_wm_rotation_available_rotations_set(
          toolbox_window_, rots, (sizeof(rots) / sizeof(int)));
    }

    evas_object_smart_callback_add(toolbox_window_, "delete,request",
                                   &Browser::OnToolboxWindowDelRequest, this);
    evas_object_show(toolbox_window_);
  }

#if defined(OS_TIZEN) && !defined(OS_TIZEN_TV_PRODUCT)
  CheckApplicationPermission(kLocationPrivilege);
#endif
}

Browser::~Browser() {
  while (!window_map_.empty())
    window_map_.begin()->second.window->Close();
  Ewk_Context* ctx = ewk_context_default_get();
  if (inspector_started_)
    ewk_context_inspector_server_stop(ctx);
}

Window& Browser::CreateWindow(bool incognito) {
  WindowMapData data;
  memset(&data, 0, sizeof(data));

  if (desktop_)
    data.window = new Window(*this, kDefaultDesktopWindowWidth,
                             kDefaultDesktopWindowHeight, incognito);
  else
    data.window = new Window(*this, kDefaultMobileWindowWidth,
                             kDefaultMobileWindowHeight, incognito);
  selected_window_ = data.window;

  if (toolbox_window_) {
    Evas_Object* pb = elm_progressbar_add(toolbox_window_);
    elm_object_style_set(pb, "wheel");
    elm_progressbar_pulse_set(pb, EINA_TRUE);
    evas_object_size_hint_align_set(pb, EVAS_HINT_FILL, 0);
    evas_object_size_hint_weight_set(pb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    data.loading_indicator = pb;
    data.list_elm = elm_list_item_append(window_list_,
                                         "New Window",
                                         pb,
                                         CreateControlButtons(window_list_),
                                         &Browser::OnWindowElementClicked,
                                         data.window);
  }

  window_map_[data.window->Id()] = data;

  if (window_list_)
    elm_list_go(window_list_);

  log_trace("%s: %x", __PRETTY_FUNCTION__, data.window->Id());

  return *data.window;
}

void Browser::ShowInspectorWindow() {
  Ewk_Context* ctx = ewk_context_default_get();
  unsigned int port = ewk_context_inspector_server_start(ctx, 0);
  assert(port != 0);
  char url[1024];
  snprintf(url, sizeof(url), "http://localhost:%u/", port);
  Window& win = CreateWindow();
  win.EnableMouseEvents(true);
  win.EnableTouchEvents(false);
  win.LoadURL(url);
  inspector_started_ = true;
}

void Browser::StartInspectorServer() {
  Ewk_Context* ctx = ewk_context_default_get();
  unsigned port = ewk_context_inspector_server_start(ctx, kInspectorServerPort);
  if (port)
    DidStartInspectorServer(port);
  else
    log_error("Could not start Web Inspector Server at %u",
              kInspectorServerPort);
}

void Browser::DidStartInspectorServer(unsigned port) {
  // Get IP address.
  struct ifaddrs* addrs;
  int rc = getifaddrs(&addrs);
  if (rc == -1) {
    log_error("getifaddrs() failed");
    return;
  }
  char url[32] = {0};
  // Find non local, up IP address.
  for (struct ifaddrs* iter = addrs; iter && iter->ifa_addr;
       iter = iter->ifa_next) {
    if (iter->ifa_addr->sa_family == AF_INET &&
        (iter->ifa_flags & IFF_LOOPBACK) == 0 &&
        (iter->ifa_flags & IFF_RUNNING) != 0) {
      struct sockaddr_in* p_addr =
          reinterpret_cast<struct sockaddr_in*>(iter->ifa_addr);
      snprintf(url, sizeof(url) / sizeof(url[0]), "http://%s:%u",
               inet_ntoa(p_addr->sin_addr), port);
      break;
    }
  }
  freeifaddrs(addrs);

  if (strlen(url)) {
    if (selected_window_)
      selected_window_->ShowInspectorURL(url);
  } else
    log_error("Web Inspector has started at %u port but its IP is unknown",
              kInspectorServerPort);
}

void Browser::ActivateToolboxWindow() {
  log_trace("%s", __PRETTY_FUNCTION__);
  assert(toolbox_window_);
  elm_win_activate(toolbox_window_);
}

void Browser::StartTracing() {
  log_trace("%s", __PRETTY_FUNCTION__);
  ewk_start_tracing("*, disabled-by-default-toplevel.flow", "", "");
  tracing_enabled_ = true;
}

void Browser::StopTracing() {
  log_trace("%s", __PRETTY_FUNCTION__);
  ewk_stop_tracing();
  tracing_enabled_ = false;
}

void Browser::StartVibration(uint64_t duration) {
  log_trace("%s: %d", __PRETTY_FUNCTION__, duration);
#if defined(OS_TIZEN)
  if (IsMobileProfile() || IsWearableProfile()) {
    if (haptic_timer_id_) {
      ecore_timer_del(haptic_timer_id_);
      haptic_timer_id_ = NULL;
    }

    if (haptic_handle_) {
      device_haptic_stop(haptic_handle_, haptic_effect_);
      device_haptic_close(haptic_handle_);
      haptic_handle_ = NULL;
    }

    if (device_haptic_open(0, &haptic_handle_) != DEVICE_ERROR_NONE) {
      log_error("__vibration_on_cb:device_haptic_open failed");
      return;
    }

    device_haptic_vibrate(haptic_handle_, duration, 100, &haptic_effect_);
    double in = (double)((double)(duration) / (double)(1000));
    haptic_timer_id_ = ecore_timer_add(in, &Browser::OnVibrationTimeout, this);
  }
#endif
}

void Browser::StopVibration() {
  log_trace("%s", __PRETTY_FUNCTION__);
#if defined(OS_TIZEN)
  if (IsMobileProfile() || IsWearableProfile()) {
    if (haptic_timer_id_) {
      ecore_timer_del(haptic_timer_id_);
      haptic_timer_id_ = NULL;
    }

    if (haptic_handle_) {
      device_haptic_stop(haptic_handle_, haptic_effect_);
      device_haptic_close(haptic_handle_);
      haptic_handle_ = NULL;
    }
  }
#endif
}

void Browser::OnWindowDestroyed(Window::IdType id) {
  log_trace("%s: %x", __PRETTY_FUNCTION__, id);
  assert(window_map_.find(id) != window_map_.end());
  WindowMapData window_map_data = window_map_[id];
  if (window_map_data.list_elm)
    elm_object_item_del(window_map_data.list_elm);
  if (window_map_data.window == selected_window_)
    selected_window_ = NULL;
  if (window_list_)
    elm_list_go(window_list_);
  delete window_map_data.window;
  window_map_.erase(id);
}

void Browser::OnWindowTitleChanged(Window::IdType id, const char* title) {
  if (toolbox_window_)
    elm_object_item_part_text_set(window_map_[id].list_elm, "default", title);
}

void Browser::OnWindowLoadStarted(Window::IdType id) {
  assert(window_map_.find(id) != window_map_.end());
  if (window_map_[id].loading_indicator)
    elm_progressbar_pulse(window_map_[id].loading_indicator, EINA_TRUE);
}

void Browser::OnWindowLoadFinished(Window::IdType id) {
  assert(window_map_.find(id) != window_map_.end());
  if (window_map_[id].loading_indicator)
    elm_progressbar_pulse(window_map_[id].loading_indicator, EINA_FALSE);
}

void Browser::AddButton(Evas_Object* parent,
                        const char* label,
                        const char* icon,
                        const char* tooltip,
                        Evas_Smart_Cb cb) {
  Evas_Object* bt = elm_button_add(parent);
  evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
  evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, 0.5);
  if (label)
    elm_object_text_set(bt, label);
  if (tooltip)
    elm_object_tooltip_text_set(bt, tooltip);
  if (icon) {
    Evas_Object* icon_elm = elm_icon_add(bt);
    elm_icon_standard_set(icon_elm, icon);
    elm_object_part_content_set(bt, "icon", icon_elm);
  }
  evas_object_smart_callback_add(bt, "clicked", cb, this);
  elm_box_pack_end(parent, bt);
  evas_object_show(bt);
}

Evas_Object* Browser::CreateControlButtons(Evas_Object* parent) {
  Evas_Object* box = elm_box_add(parent);
  elm_box_horizontal_set(box, EINA_TRUE);
  evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
  evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
  AddButton(box, NULL, "close", "Close", &Browser::OnWindowClose);
  evas_object_show(box);
  return box;
}

void Browser::OnToolboxWindowDelRequest(void* data, Evas_Object*, void*) {
  Browser* thiz = static_cast<Browser*>(data);
  while (!thiz->window_map_.empty())
    thiz->window_map_.begin()->second.window->Close();
  thiz->toolbox_window_ = NULL;
}

void Browser::OnWindowElementClicked(void* data, Evas_Object*, void*) {
  selected_window_ = static_cast<Window*>(data);
  selected_window_->Activate();
}

void Browser::OnWindowClose(void* data, Evas_Object*, void*) {
  if (selected_window_) {
    selected_window_->Close();
    selected_window_ = NULL;
  }
}

void Browser::OnNewWindowRequest(void* data, Evas_Object*, void*) {
  Browser* thiz = static_cast<Browser*>(data);
  Window& win = thiz->CreateWindow();
  win.LoadURL(kHomePage);
}

void Browser::OnNewIncognitoWindowRequest(void* data, Evas_Object*, void*) {
  Browser* thiz = static_cast<Browser*>(data);
  Window& win = thiz->CreateWindow(true);
  win.LoadURL(kHomePage);
}

void Browser::OnVibrationStart(uint64_t vibration_time, void* data) {
  Browser* thiz = static_cast<Browser*>(data);
  thiz->StartVibration(vibration_time);
}

void Browser::OnVibrationStop(void* data) {
  Browser* thiz = static_cast<Browser*>(data);
  thiz->StopVibration();
}

Eina_Bool Browser::OnVibrationTimeout(void* data) {
  Browser::OnVibrationStop(data);
  return ECORE_CALLBACK_CANCEL;
}

void Browser::OnSignalHandler(int sig) {
  if (sig == SIGINT) {
    if (selected_window_)
      selected_window_->Exit();
  }
}

void Browser::Exit() const {
#if defined(OS_TIZEN) && defined(TIZEN_APP)
  ui_app_exit();
#else
  elm_exit();
#endif
}

void Browser::SetDefaultZoomFactor(double zoom) {
  Ewk_Context* ctx = ewk_context_default_get();
  ewk_context_default_zoom_factor_set(ctx, zoom);
}

double Browser::DefaultZoomFactor() {
  Ewk_Context* ctx = ewk_context_default_get();
  return ewk_context_default_zoom_factor_get(ctx);
}

#if defined(OS_TIZEN) && !defined(OS_TIZEN_TV_PRODUCT)
void Browser::CheckApplicationPermission(const char* privilege) {
  ppm_check_result_e result;
  int ret = ppm_check_permission(privilege, &result);
  if (ret != PRIVACY_PRIVILEGE_MANAGER_ERROR_NONE) {
    log_error("ppm_check_permission failed.");
    return;
  }

  switch (result) {
    case PRIVACY_PRIVILEGE_MANAGER_CHECK_RESULT_ALLOW:
      log_info("%s privilege was allowed.", privilege);
      break;
    case PRIVACY_PRIVILEGE_MANAGER_CHECK_RESULT_DENY:
      log_info("%s privilege was denied.", privilege);
      break;
    case PRIVACY_PRIVILEGE_MANAGER_CHECK_RESULT_ASK:
      ppm_request_permission(privilege, OnPermissionCheckResult, nullptr);
      break;
  }
}

void Browser::OnPermissionCheckResult(ppm_call_cause_e cause,
                                      ppm_request_result_e result,
                                      const char* privilege,
                                      void* user_data) {
  if (cause == PRIVACY_PRIVILEGE_MANAGER_CALL_CAUSE_ERROR) {
    log_error("ppm_request_permission failed.");
    return;
  }

  switch (result) {
    case PRIVACY_PRIVILEGE_MANAGER_REQUEST_RESULT_ALLOW_FOREVER:
      log_info("%s privilege was allowed forever.", privilege);
      break;
    case PRIVACY_PRIVILEGE_MANAGER_REQUEST_RESULT_DENY_FOREVER:
      log_info("%s privilege was denied forever.", privilege);
      break;
    case PRIVACY_PRIVILEGE_MANAGER_REQUEST_RESULT_DENY_ONCE:
      log_info("%s privilege was denied.", privilege);
      break;
  }
}
#endif
