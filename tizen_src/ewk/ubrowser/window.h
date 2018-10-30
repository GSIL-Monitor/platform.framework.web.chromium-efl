// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _WINDOW_H_
#define _WINDOW_H_

#include <string>
#include <Ecore_Evas.h>
#include <ewk_settings_internal.h>
#include <ewk_quota_permission_request_internal.h>

class Browser;
class WindowUI;

class Window {
 public:
  typedef void* IdType;

  Window(Browser&, int width, int height, bool incognito);
  ~Window();

  Evas_Object* GetEvasObject() const { return window_; };
  Ewk_Settings* GetEwkSettings() const;

  void LoadURL(std::string url);
  const char* GetURL() const;
  void Activate();
  void Close();
  void Show();
  void Hide();
  void Back();
  void Forward();
  void Reload();
  void Stop();
  void SetUserAgent(const char* new_ua);
  const char* GetUserAgent() const;
  void FakeRotate();
  void Resize(int width, int height);
  void EnableTouchEvents(bool);
  void EnableMouseEvents(bool);
  bool AreTouchEventsEnabled() const;
  bool IsRememberFormDataEnabled() const;
  bool IsRememberPasswordEnabled() const;
  bool IsFormProfileEnabled() const;
  double GetScale() const;
  void GetScaleRange(double* minScale, double* maxScale) const;
  void SetScale(double);
  void ShowInspectorURL(const char* url);
  void SetGoogleDataProxyHeaders() const;
  void SetAutoRotate();
  void Exit() const;

  IdType Id() const;

 private:
  static void OnWindowDelRequest(void* data, Evas_Object*, void*);
  static void OnNewWindowRequest(void *data, Evas_Object*, void*);
  static void OnTitleChanged(void*, Evas_Object*, void*);
  static void OnURLChanged(void*, Evas_Object*, void*);
  static void OnLoadStarted(void*, Evas_Object*, void*);
  static void OnLoadFinished(void*, Evas_Object*, void*);
  static void OnConsoleMessage(void*, Evas_Object*, void*);
  static void OnOrientationChanged(void*, Evas_Object*, void*);
  static void OnNewWindowPolicyDecide(void*, Evas_Object*, void*);
  static void OnBackForwardListChanged(void*, Evas_Object*, void*);
  static void OnQuotaPermissionRequest(Evas_Object*, const Ewk_Quota_Permission_Request*, void*);
  static void OnUserMediaPermissionRequest(void* data, Evas_Object*, void* event_info);
  static void OnUserMediaPermissionDecisionTaken(bool decision, void* data);
  static void OnEnterFullScreenRequest(void*, Evas_Object*, void*);
  static void OnExitFullScreenRequest(void*, Evas_Object*, void*);
  static void OnHostFocusedIn(void* data, Evas_Object*, void*);
  static void OnHostFocusedOut(void* data, Evas_Object*, void*);
  static void OnBeforeFormRepostWarningShow(void*, Evas_Object*, void*);
  static void OnFormRepostWarningShow(void*, Evas_Object*, void*);
  static void OnUserFormRepostDecisionTaken(bool decision, void* data);
  static void OnThemeColorChanged(Evas_Object* o,
                                  int r,
                                  int g,
                                  int b,
                                  int a,
                                  void* data);
#if defined(OS_TIZEN)
  static void OnHWBack(void*, Evas_Object*, void*);
#endif

#if defined(PWE_SUPPORT)
  static void OnServiceWorkerRegistrationResult(Ewk_Context*, const char*, const char*, Eina_Bool, void*);
  static void OnServiceWorkerUnregistrationResult(Ewk_Context*, const char*, Eina_Bool, void*);
#endif

#if defined(OS_TIZEN)
  static void OnKeyDown(void*, Evas*, Evas_Object*, void*);
#endif
#if defined(OS_TIZEN_TV_PRODUCT)
  static void On_Notify_Subtitle_Play(void*, Evas_Object*, void*);
  static void On_Notify_Subtitle_Stop(void*, Evas_Object*, void*);
  static void On_Notify_Subtitle_Pause(void*, Evas_Object*, void*);
  static void On_Notify_Subtitle_Resume(void*, Evas_Object*, void*);
  static void On_Notify_Subtitle_Seek_Start(void*, Evas_Object*, void*);
  static void On_Notify_Subtitle_Seek_Completed(void*, Evas_Object*, void*);
  static void On_Subtitle_Data(void*, Evas_Object*, void*);
  static void On_Video_Playback_Load(void*, Evas_Object*, void*);
  static void On_Video_Playback_Ready(void*, Evas_Object*, void*);
  static void On_Video_Playback_Start(void*, Evas_Object*, void*);
  static void On_Video_Playback_Finish(void*, Evas_Object*, void*);
  static void On_Video_Playback_Stop(void*, Evas_Object*, void*);
#if defined(TIZEN_TBM_SUPPORT)
  static void OnVideotexturing(void*, Evas_Object*, void*);
#endif  // defined(TIZEN_TBM_SUPPORT)
#endif
#if defined(OS_TIZEN_TV_PRODUCT)
  void CreateMouseCursor();
#endif
  Browser& browser_;
  WindowUI* ui_;
  Evas_Object* window_;
  Evas_Object* web_view_;
  Evas_Object* web_view_elm_host_;
  bool is_fullscreen_;
};

#endif // _WINDOW_H_
