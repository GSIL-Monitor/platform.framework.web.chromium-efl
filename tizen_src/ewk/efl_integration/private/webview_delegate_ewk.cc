// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webview_delegate_ewk.h"

#include <cassert>
#include "eweb_view.h"
#include "public/ewk_view.h"
#include "private/ewk_view_private.h"

// static
const WebViewDelegateEwk& WebViewDelegateEwk::GetInstance() {
  static WebViewDelegateEwk instance;
  return instance;
}

EWebView* WebViewDelegateEwk::GetWebViewFromEvasObject(
    Evas_Object* evas_object) const {
  return ::GetWebViewFromEvasObject(evas_object);
}

/* LCOV_EXCL_START */
bool WebViewDelegateEwk::IsWebViewEvasObject(Evas_Object* evas_object) const {
  return ::IsWebViewObject(evas_object);
}

Evas_Object_Smart_Clipped_Data* WebViewDelegateEwk::GetSmartClippedData(
    Evas_Object* evas_object) {
  if (!WebViewDelegateEwk::IsWebViewEvasObject(evas_object)) {
    return NULL;
  }
  return &GetEwkViewSmartDataFromEvasObject(evas_object)->base;
}

Eina_Rectangle WebViewDelegateEwk::GetLastUsedViewPortArea(
    Evas_Object* evas_object) const {
  Eina_Rectangle result = {-1, -1, -1, -1};
  if (WebViewDelegateEwk::IsWebViewEvasObject(evas_object)) {
    Ewk_View_Smart_Data* sd = GetEwkViewSmartDataFromEvasObject(evas_object);
    assert(sd);
    result.x = sd->view.x;
    result.y = sd->view.y;
    result.w = sd->view.w;
    result.h = sd->view.h;
  }
  return result;
}
/* LCOV_EXCL_STOP */

bool WebViewDelegateEwk::RequestHandleEvent_FocusIn(EWebView* wv) const {
  Ewk_View_Smart_Data* sd = GetEwkViewSmartDataFromEvasObject(wv->evas_object());
  if (sd && sd->api && sd->api->focus_in) {
    // XXX: is it what we want, or do we want to return false if api->focus_in
    // returns false?
    sd->api->focus_in(sd);  // LCOV_EXCL_LINE
    return true;            // LCOV_EXCL_LINE
  }
  return false;
}

/* LCOV_EXCL_START */
bool WebViewDelegateEwk::RequestHandleEvent_FocusOut(EWebView* wv) const {
  Ewk_View_Smart_Data* sd = GetEwkViewSmartDataFromEvasObject(wv->evas_object());
  if (sd && sd->api && sd->api->focus_out) {
    // XXX: is it what we want, or do we want to return false if api->focus_out
    // returns false?
    sd->api->focus_out(sd);
    return true;
  }
  return false;
}

bool WebViewDelegateEwk::RequestHandleEvent_MouseUp(EWebView* wv,
    const Evas_Event_Mouse_Up* event_info) const {
  Ewk_View_Smart_Data* sd = GetEwkViewSmartDataFromEvasObject(wv->evas_object());
  if (sd && sd->api && sd->api->mouse_up) {
    // XXX: is it what we want, or do we want to return false if api->mouse_up
    // returns false?
    sd->api->mouse_up(sd, event_info);
    return true;
  }
  return false;
}

bool WebViewDelegateEwk::RequestHandleEvent_MouseDown(EWebView* wv,
    const Evas_Event_Mouse_Down* event_info) const {
  Ewk_View_Smart_Data* sd = GetEwkViewSmartDataFromEvasObject(wv->evas_object());
  if (sd && sd->api && sd->api->mouse_down) {
    // XXX: is it what we want, or do we want to return false if api->mouse_down
    // returns false?
    sd->api->mouse_down(sd, event_info);
    return true;
  }
  return false;
}

bool WebViewDelegateEwk::RequestHandleEvent_MouseMove(EWebView* wv,
    const Evas_Event_Mouse_Move* event_info) const {
  Ewk_View_Smart_Data* sd = GetEwkViewSmartDataFromEvasObject(wv->evas_object());
  if (sd && sd->api && sd->api->mouse_move) {
    // XXX: is it what we want, or do we want to return false if api->mouse_move
    // returns false?
    sd->api->mouse_move(sd, event_info);
    return true;
  }
  return false;
}

bool WebViewDelegateEwk::RequestHandleEvent_MouseWheel(EWebView* wv,
    const Evas_Event_Mouse_Wheel* event_info) const {
  Ewk_View_Smart_Data* sd = GetEwkViewSmartDataFromEvasObject(wv->evas_object());
  if (sd && sd->api && sd->api->mouse_wheel) {
    // XXX: is it what we want, or do we want to return false if api->mouse_wheel
    // returns false?
    sd->api->mouse_wheel(sd, event_info);
    return true;
  }
  return false;
}

bool WebViewDelegateEwk::RequestHandleEvent_KeyUp(EWebView* wv,
    const Evas_Event_Key_Up* event_info) const {
  Ewk_View_Smart_Data* sd = GetEwkViewSmartDataFromEvasObject(wv->evas_object());
  if (sd && sd->api && sd->api->key_up) {
    // XXX: is it what we want, or do we want to return false if api->key_up
    // returns false?
    sd->api->key_up(sd, event_info);
    return true;
  }
  return false;
}

bool WebViewDelegateEwk::RequestHandleEvent_KeyDown(EWebView* wv,
    const Evas_Event_Key_Down* event_info) const {
  Ewk_View_Smart_Data* sd = GetEwkViewSmartDataFromEvasObject(wv->evas_object());
  if (sd && sd->api && sd->api->key_down) {
    // XXX: is it what we want, or do we want to return false if api->key_down
    // returns false?
    sd->api->key_down(sd, event_info);
    return true;
  }
  return false;
}

void WebViewDelegateEwk::RequestHandleOrientationLock(EWebView* wv,
    int orientation) const {
  Ewk_View_Smart_Data* sd = GetEwkViewSmartDataFromEvasObject(wv->evas_object());
  if (sd && sd->api && sd->api->orientation_lock) {
    sd->api->orientation_lock(sd, orientation);
  }
}

void WebViewDelegateEwk::RequestHandleOrientationUnlock(EWebView* wv) const {
  Ewk_View_Smart_Data* sd = GetEwkViewSmartDataFromEvasObject(wv->evas_object());
  if (sd && sd->api && sd->api->orientation_unlock) {
    sd->api->orientation_unlock(sd);
  }
}
/* LCOV_EXCL_STOP */
