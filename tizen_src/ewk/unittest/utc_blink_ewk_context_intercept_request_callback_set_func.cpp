// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

#include <atomic>

class utc_blink_ewk_context_intercept_request_callback_set
    : public utc_blink_ewk_base {
 protected:
  utc_blink_ewk_context_intercept_request_callback_set()
      : callback_called_(false) {}

  static const char* kInterceptURL;

  void LoadFinished(Evas_Object* webview) override {
    // Should not load without decision about interception.
    EventLoopStop(Failure);
  }

  static void intercept_request_callback(
      Ewk_Context* /*ctx*/,
      Ewk_Intercept_Request* intercept_request,
      void* user_data) {
    auto* owner =
        static_cast<utc_blink_ewk_context_intercept_request_callback_set*>(
            user_data);
    owner->callback_called_.store(true);
  }

  std::atomic<bool> callback_called_;
};

const char*
    utc_blink_ewk_context_intercept_request_callback_set::kInterceptURL =
        "http://www.google.com/";

/**
 * @brief Tests if intercept request callback is called.
 */
TEST_F(utc_blink_ewk_context_intercept_request_callback_set,
       POS_TEST_CALLBACK_SET) {
  ewk_context_intercept_request_callback_set(
      ewk_view_context_get(GetEwkWebView()), intercept_request_callback, this);

  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), kInterceptURL));
  ASSERT_EQ(Timeout, EventLoopStart(3.0));
  ASSERT_TRUE(callback_called_.load());
}

/**
 * @brief Tests if function resets callback in case of NULL of a callback.
 */
TEST_F(utc_blink_ewk_context_intercept_request_callback_set,
       POS_TEST_CALLBACK_RESET) {
  ewk_context_intercept_request_callback_set(
      ewk_view_context_get(GetEwkWebView()), intercept_request_callback, this);
  ewk_context_intercept_request_callback_set(
      ewk_view_context_get(GetEwkWebView()), nullptr, nullptr);

  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), kInterceptURL));

  // Expected value Failure, because LoadFinished sets this status
  // if url loads normally.
  ASSERT_EQ(Failure, EventLoopStart());
  ASSERT_FALSE(callback_called_.load());
}

/**
 * @brief Tests if function works properly in case of NULL of a context.
 */
TEST_F(utc_blink_ewk_context_intercept_request_callback_set,
       NEG_TEST_NULL_CONTEXT) {
  ewk_context_intercept_request_callback_set(
      nullptr, intercept_request_callback, nullptr);
}
