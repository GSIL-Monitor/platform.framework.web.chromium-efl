// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TIZEN_WEBVIEW_EVENT_TW_WEBVIEW_EVAS_EVENT_HANDLER_H_
#define TIZEN_WEBVIEW_EVENT_TW_WEBVIEW_EVAS_EVENT_HANDLER_H_

#include "content/browser/renderer_host/evas_event_handler.h"

class EWebView;

class WebViewEvasEventHandler: public content::EvasEventHandler {
 public:
  explicit WebViewEvasEventHandler(EWebView*); // LCOV_EXCL_LINE

  // ---- event handlers
  bool HandleEvent_FocusIn() override;
  bool HandleEvent_FocusOut() override;
  bool HandleEvent_KeyDown (const Evas_Event_Key_Down*) override;
  bool HandleEvent_KeyUp (const Evas_Event_Key_Up*) override;
  bool HandleEvent_MouseDown (const Evas_Event_Mouse_Down*) override;
  bool HandleEvent_MouseUp   (const Evas_Event_Mouse_Up*) override;
  bool HandleEvent_MouseMove (const Evas_Event_Mouse_Move*) override;
  bool HandleEvent_MouseWheel(const Evas_Event_Mouse_Wheel*) override;

 private:
  EWebView* webview_;
};

#endif  // TIZEN_WEBVIEW_PUBLIC_TW_WEBVIEW_EVAS_EVENT_HANDLER_H_
