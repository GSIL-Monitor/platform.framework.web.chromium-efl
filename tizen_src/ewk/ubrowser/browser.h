// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _BROWSER_H_
#define _BROWSER_H_

#include <map>
#include <Ecore_Evas.h>
#include <Elementary.h>

#include "window.h"

#if defined(OS_TIZEN)
#include <device/haptic.h>
#endif

#if defined(OS_TIZEN) && !defined(OS_TIZEN_TV_PRODUCT)
#include <privacy-privilege-manager/privacy_privilege_manager.h>
#endif

class Browser {
 public:
  enum GUILevel {
    UBROWSER_GUI_LEVEL_NONE = 0,
    UBROWSER_GUI_LEVEL_URL_ONLY,
    UBROWSER_GUI_LEVEL_MINIMAL,
    UBROWSER_GUI_LEVEL_ALL,
  };

  Browser(bool desktop, GUILevel gui_level, std::string ua, std::string tizen_version);
  ~Browser();

  Window& CreateWindow(bool = false);
  void ShowInspectorWindow();
  void StartInspectorServer();
  void ActivateToolboxWindow();
  bool IsDesktop() const { return desktop_; }
  bool IsTracingEnabled() const { return tracing_enabled_; }
  std::string GetUserAgent() const { return user_agent_; }
  std::string GetTizenVersion() const { return tizen_version_; }
  GUILevel GetGUILevel() const { return gui_level_; }
  void StartTracing();
  void StopTracing();
  void StartVibration(uint64_t);
  void StopVibration();

  void OnWindowDestroyed(Window::IdType id);
  void OnWindowURLChanged(Window::IdType id, const char*);
  void OnWindowTitleChanged(Window::IdType id, const char*);
  void OnWindowLoadStarted(Window::IdType id);
  void OnWindowLoadFinished(Window::IdType id);

  void Exit() const;
#if defined(TIZEN_VIDEO_HOLE)
  void SetEnableVideoHole(bool enable) { video_hole_ = enable; }
  bool IsVideoHoleEnabled() const { return video_hole_; }
#endif
  void SetDefaultZoomFactor(double zoom);
  double DefaultZoomFactor();

 private:
  void AddButton(Evas_Object*, const char*, const char*,
                 const char*, Evas_Smart_Cb);
  Evas_Object* CreateControlButtons(Evas_Object*);
  void DidStartInspectorServer(unsigned port);

#if defined(OS_TIZEN) && !defined(OS_TIZEN_TV_PRODUCT)
  void CheckApplicationPermission(const char* privilege);
  static void OnPermissionCheckResult(ppm_call_cause_e cause,
                                      ppm_request_result_e result,
                                      const char* privilege,
                                      void* user_data);
#endif

  static void OnToolboxWindowDelRequest(void* data, Evas_Object*, void*);
  static void OnWindowElementClicked(void* data, Evas_Object*, void*);
  static void OnWindowClose(void* data, Evas_Object*, void*);
  static void OnNewWindowRequest(void* data, Evas_Object*, void*);
  static void OnNewIncognitoWindowRequest(void* data, Evas_Object*, void*);
  static void OnVibrationStart(uint64_t vibration_time, void*);
  static void OnVibrationStop(void*);
  static Eina_Bool OnVibrationTimeout(void*);
  static void OnSignalHandler(int);

  typedef struct _WindowMapData {
    Window*          window;
    Elm_Object_Item* list_elm;
    Evas_Object*     loading_indicator;
  } WindowMapData;

  typedef std::map<Window::IdType, WindowMapData> WindowMap;

  WindowMap window_map_;
  static Window* selected_window_;

  Evas_Object* toolbox_window_;
  Evas_Object* window_list_;

  bool desktop_;
  bool inspector_started_;
  bool tracing_enabled_;
  std::string user_agent_;
  std::string tizen_version_;
#if defined(TIZEN_VIDEO_HOLE)
  bool video_hole_;
#endif
#if defined(OS_TIZEN)
  Ecore_Timer* haptic_timer_id_;
  haptic_device_h haptic_handle_;
  haptic_effect_h haptic_effect_;
#endif
  GUILevel gui_level_;
};

#endif // _BROWSER_H_
