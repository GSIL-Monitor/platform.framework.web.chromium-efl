// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

class utc_blink_ewk_view_main_frame_scrollbar_visible_set
    : public utc_blink_ewk_base {
 protected:
  void PostSetUp() override {
    evas_object_smart_callback_add(GetEwkWebView(), "contents,size,changed",
                                   ContentsSizeChanged, this);
  }

  void PreTearDown() override {
    evas_object_smart_callback_del(GetEwkWebView(), "contents,size,changed",
                                   ContentsSizeChanged);
  }

  static void ContentsSizeChanged(void* data,
                                  Evas_Object* eObject,
                                  void* dataFinished) {
    if (data) {
      static_cast<utc_blink_ewk_view_main_frame_scrollbar_visible_set*>(data)
          ->EventLoopStop(Success);
    }
  }

  static void ScrollbarVisibleGetCallback(Evas_Object* eObject,
                                          Eina_Bool visible,
                                          void* data) {
    utc_check_ne(data, nullptr);
    utc_blink_ewk_view_main_frame_scrollbar_visible_set* owner = nullptr;
    OwnerFromVoid(data, &owner);
    utc_message("[ScrollbarVisibleGetCallback] :: visible = %s\n",
                visible == EINA_TRUE ? "EINA_TRUE" : "EINA_FALSE");
    utc_check_eq(owner->expected_visible_, visible);
    owner->EventLoopStop(Success);
  }

 protected:
  Eina_Bool expected_visible_;
};

TEST_F(utc_blink_ewk_view_main_frame_scrollbar_visible_set, TURN_OFF) {
  Eina_Bool ret = EINA_FALSE;

  // Loading the page triggers a call to ContentsSizeChanged callback.
  utc_check_eq(EINA_TRUE, ewk_view_url_set(
                              GetEwkWebView(),
                              GetResourceUrl("common/sample_2.html").c_str()));
  utc_check_eq(Success, EventLoopStart());

  // Initially the scrollbar is visible.
  expected_visible_ = EINA_TRUE;
  ret = ewk_view_main_frame_scrollbar_visible_get(
      GetEwkWebView(), ScrollbarVisibleGetCallback, this);
  utc_check_eq(ret, EINA_TRUE);
  utc_check_eq(Success, EventLoopStart());

  // Check that changing scrollbar visiblity triggers "contents,size,changed"
  // callback. Indeed at this point only ContentsSizeChanged callback can stop
  // the next call to EventLoopStart.
  ret = ewk_view_main_frame_scrollbar_visible_set(GetEwkWebView(), false);
  utc_check_eq(ret, EINA_TRUE);
  utc_check_eq(Success, EventLoopStart());

  // Check that the scrollbar is not visible.
  expected_visible_ = EINA_FALSE;
  ret = ewk_view_main_frame_scrollbar_visible_get(
      GetEwkWebView(), ScrollbarVisibleGetCallback, this);
  utc_check_eq(ret, EINA_TRUE);
  utc_check_eq(Success, EventLoopStart());

  // Make the scrollbar visible again.
  ret = ewk_view_main_frame_scrollbar_visible_set(GetEwkWebView(), true);
  utc_check_eq(ret, EINA_TRUE);
  utc_check_eq(Success, EventLoopStart());
  expected_visible_ = EINA_TRUE;
  ret = ewk_view_main_frame_scrollbar_visible_get(
      GetEwkWebView(), ScrollbarVisibleGetCallback, this);
  utc_check_eq(ret, EINA_TRUE);
  utc_check_eq(Success, EventLoopStart());
}
