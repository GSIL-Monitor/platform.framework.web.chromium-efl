// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef webview_delegate_ewk_h
#define webview_delegate_ewk_h

#include <Evas.h>
#include "base/macros.h"

class EWebView;

struct WebViewDelegateEwk {
 public:
  static const WebViewDelegateEwk& GetInstance();
  EWebView* GetWebViewFromEvasObject(Evas_Object* evas_object) const;
  bool IsWebViewEvasObject(Evas_Object* evas_object) const;
  Evas_Object_Smart_Clipped_Data* GetSmartClippedData(Evas_Object* evas_object);
  Eina_Rectangle GetLastUsedViewPortArea(Evas_Object* evas_object) const;

  // Event handlers
  bool RequestHandleEvent_FocusIn(EWebView* wv) const;
  bool RequestHandleEvent_FocusOut(EWebView* wv) const;
  bool RequestHandleEvent_MouseUp(EWebView* wv, const Evas_Event_Mouse_Up* event_info) const;
  bool RequestHandleEvent_MouseDown(EWebView* wv, const Evas_Event_Mouse_Down* event_info) const;
  bool RequestHandleEvent_MouseMove(EWebView* wv, const Evas_Event_Mouse_Move* event_info) const;
  bool RequestHandleEvent_MouseWheel(EWebView* wv, const Evas_Event_Mouse_Wheel* event_info) const;
  bool RequestHandleEvent_KeyUp(EWebView* wv, const Evas_Event_Key_Up* event_info) const;
  bool RequestHandleEvent_KeyDown(EWebView* wv, const Evas_Event_Key_Down* event_info) const;
  void RequestHandleOrientationLock(EWebView* wv, int orientation) const;
  void RequestHandleOrientationUnlock(EWebView* wv) const;
 private:
  WebViewDelegateEwk() {}
  DISALLOW_COPY_AND_ASSIGN(WebViewDelegateEwk);
};


#endif // webview_delegate_ewk_h
