+  // Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_intercept_request_get_base.h"

    const std::string utc_blink_ewk_intercept_request_get_base::kInterceptURL =
        "http://www.google.com/";

utc_blink_ewk_intercept_request_get_base::
    utc_blink_ewk_intercept_request_get_base()
    : api_executed_(false) {}

utc_blink_ewk_intercept_request_get_base::
    ~utc_blink_ewk_intercept_request_get_base() {}

void utc_blink_ewk_intercept_request_get_base::LoadFinished(
    Evas_Object* webview) {
  // Should not load without decision about interception.
  EventLoopStop(Failure);
}

void utc_blink_ewk_intercept_request_get_base::callback_positive(
    Ewk_Context* /*ctx*/,
    Ewk_Intercept_Request* intercept_request,
    void* user_data) {
  auto* owner =
      static_cast<utc_blink_ewk_intercept_request_get_base*>(user_data);
  owner->test_func(intercept_request);
  owner->api_executed_.store(true);
}

void utc_blink_ewk_intercept_request_get_base::callback_negative(
    Ewk_Context* /*ctx*/,
    Ewk_Intercept_Request* /*intercept_request*/,
    void* user_data) {
  auto* owner =
      static_cast<utc_blink_ewk_intercept_request_get_base*>(user_data);
  owner->test_func(nullptr);
  owner->api_executed_.store(true);
}

void utc_blink_ewk_intercept_request_get_base::pos_test() {
  ewk_context_intercept_request_callback_set(
      ewk_view_context_get(GetEwkWebView()), callback_positive, this);

  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), kInterceptURL.c_str()));
  ASSERT_EQ(Timeout, EventLoopStart(3.0));
  ASSERT_TRUE(api_executed_.load());
}

void utc_blink_ewk_intercept_request_get_base::neg_test() {
  ewk_context_intercept_request_callback_set(
      ewk_view_context_get(GetEwkWebView()), callback_negative, this);

  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), kInterceptURL.c_str()));
  ASSERT_EQ(Timeout, EventLoopStart(3.0));
  ASSERT_TRUE(api_executed_.load());
}
