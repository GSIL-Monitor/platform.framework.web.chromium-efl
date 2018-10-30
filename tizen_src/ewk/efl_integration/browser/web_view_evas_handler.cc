// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/web_view_evas_handler.h"

#include "private/webview_delegate_ewk.h"

#include "eweb_view.h"
#include "tizen/system_info.h"

using content::SelectionControllerEfl;

WebViewEvasEventHandler::WebViewEvasEventHandler(EWebView* wv)
    : webview_ (wv) {
}

bool WebViewEvasEventHandler::HandleEvent_FocusIn() {
  return WebViewDelegateEwk::GetInstance().
      RequestHandleEvent_FocusIn(webview_);
}

/* LCOV_EXCL_START */
bool WebViewEvasEventHandler::HandleEvent_FocusOut() {
  return WebViewDelegateEwk::GetInstance().
      RequestHandleEvent_FocusOut(webview_);
}

bool WebViewEvasEventHandler::HandleEvent_KeyDown(const Evas_Event_Key_Down* event_info) {
  return WebViewDelegateEwk::GetInstance().
      RequestHandleEvent_KeyDown(webview_, event_info);
}

bool WebViewEvasEventHandler::HandleEvent_KeyUp(const Evas_Event_Key_Up* event_info) {
  return WebViewDelegateEwk::GetInstance().
      RequestHandleEvent_KeyUp(webview_, event_info);
}

bool WebViewEvasEventHandler::HandleEvent_MouseDown(const Evas_Event_Mouse_Down* event_info) {
  return WebViewDelegateEwk::GetInstance().
      RequestHandleEvent_MouseDown(webview_, event_info);
}

bool WebViewEvasEventHandler::HandleEvent_MouseUp(const Evas_Event_Mouse_Up* event_info) {
  return WebViewDelegateEwk::GetInstance().
      RequestHandleEvent_MouseUp(webview_, event_info);
}

bool WebViewEvasEventHandler::HandleEvent_MouseMove(const Evas_Event_Mouse_Move* event_info) {
  return WebViewDelegateEwk::GetInstance().
      RequestHandleEvent_MouseMove(webview_, event_info);
}

bool WebViewEvasEventHandler::HandleEvent_MouseWheel(const Evas_Event_Mouse_Wheel* event_info) {
  return WebViewDelegateEwk::GetInstance().
      RequestHandleEvent_MouseWheel(webview_, event_info);
}
/* LCOV_EXCL_STOP */
