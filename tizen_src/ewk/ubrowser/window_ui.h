// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _WINDOW_UI_H_
#define _WINDOW_UI_H_

#include <Evas.h>

class Window;
class Browser;

typedef void (* PermissionCallback)(bool decision, void* data);

typedef struct {
  void* callbackData;
  PermissionCallback callback;
  void* popupData;
} PermissionPopupButtonData;

class WindowUI {
 public:
  WindowUI(Window&, Browser&);
  ~WindowUI();

  Evas_Object* GetNavBar() const { return navbar_; }
  Evas_Object* GetURLBar() const { return urlbar_; }

  void OnURLChanged(const char* url);
  void OnLoadingStarted();
  void OnLoadingFinished();

  void EnableBackButton(bool);
  void EnableForwardButton(bool);

  void ShowPermissionPopup(const char* title, const char* description,
                           PermissionCallback, void* data);
  void ShowInspectorURLPopup(const char* url);

  void OnThemeColorChanged(int r, int g, int b, int a);
  void ClosePopup();

 private:
  void CreateTopBar();
  void CreateBottomBar();
  Evas_Object* AddButton(Evas_Object*, const char*,
                         const char*, Evas_Smart_Cb);
  Evas_Object* CreateExtraActionsMenu(Evas_Object*);
  void ShowTextEntryPopup(const char* title,
                          const char* initial_content, Evas_Smart_Cb);
  void ShowNotification(const char* text);
#if defined(PWE_SUPPORT)
  void ShowTextEntriesPopup(const char* title,
                            const std::vector<const char*>& keys,
                            Evas_Smart_Cb cb);
#endif

  void CloseMenu();

  // Nav bar callbacks
  static void OnCreateWindow(void* data, Evas_Object*, void*);
  static void OnCreateIncognitoWindow(void* data, Evas_Object*, void*);
  static void OnBack(void* data, Evas_Object*, void*);
  static void OnForward(void* data, Evas_Object*, void*);
  static void OnStopOrReload(void* data, Evas_Object*, void*);
  static void OnShowExtraActionsMenu(void* data, Evas_Object*, void*);

  // URL bar callbacks
  static void OnURLEntered(void* data, Evas_Object*, void*);

  // Extra actions callbacks
  static void OnUAOverride(void* data, Evas_Object*, void*);
  static void OnUAOverrideEntered(void* data, Evas_Object*, void*);
  static void OnInspectorShow(void* data, Evas_Object*, void*);
  static void OnInspectorServerStart(void* data, Evas_Object* obj, void*);
  static void OnUseGoogleDataProxy(void* data, Evas_Object* obj, void*);
#if !defined(OS_TIZEN)
  static void OnRotate(void* data, Evas_Object*, void*);
#endif
  static void OnSelectMouseInput(void* data, Evas_Object*, void*);
  static void OnSelectTouchInput(void* data, Evas_Object*, void*);
  static void OnStartTracing(void* data, Evas_Object*, void*);
  static void OnStopTracing(void* data, Evas_Object*, void*);
  static void OnAutoFittingEnabled(void* data, Evas_Object*, void*);
  static void OnAutoFittingDisabled(void* data, Evas_Object*, void*);
  static void OnAutoFillEnabled(void* data, Evas_Object*, void*);
  static void OnAutoFillDisabled(void* data, Evas_Object*, void*);

  static void OnScriptWindowOpenEnabled(void* data, Evas_Object* obj, void*);
  static void OnScriptWindowOpenDisabled(void* data, Evas_Object* obj, void*);

  static void ClosePopup(void* data, Evas_Object*, void*);
  static void OnShowZoomPopup(void* data, Evas_Object*, void*);
  static void OnZoomChanged(void* data, Evas_Object*, void*);
  static void OnRememberFormDataChange(void* data, Evas_Object*, void*);
  static void OnRememberPasswordChange(void* data, Evas_Object*, void*);
  static void OnFormProfileChange(void* data, Evas_Object*, void*);

  // Permission popup buttons callbacks
  static void PermissionDeclined(void* data, Evas_Object*, void*);
  static void PermissionGranted(void* data, Evas_Object*, void*);

  static void Exit(void* data, Evas_Object*, void*);

  Window& window_;
  Browser& browser_;
  Evas_Object* navbar_;
  Evas_Object* urlbar_;
  Evas_Object* url_entry_;
  Evas_Object* stop_reload_button_;
  Evas_Object* progress_bar_;
  Evas_Object* forward_button_;
  Evas_Object* back_button_;
  Evas_Object* active_popup_;
  Evas_Object* menu_;

  bool should_show_label_;
  bool is_loading_;

  struct Color {
    int r;
    int g;
    int b;
    int a;
  } url_entry_color_;
};

#endif // _WINDOW_UI_H_
