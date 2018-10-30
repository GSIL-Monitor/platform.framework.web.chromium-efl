// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

#include <string>

#include "utc_blink_api_result_locked.h"

static const char* const kTitleChangedCb = "title,changed";

class utc_blink_ewk_intercept_request_ignore : public utc_blink_ewk_base {
 protected:
  static const char* kInterceptURL;

  void LoadFinished(Evas_Object* webview) override {
    // URL loads normally if intercept request is ignored in callback
    const char* url = ewk_view_url_get(webview);
    if (url)
      url_ = url;
    if (!title_.empty())
      EventLoopStop(Success);
  }

  void PostSetUp() override {
    evas_object_smart_callback_add(GetEwkWebView(), kTitleChangedCb,
                                   TitleChanged, this);
  }

  void PreTearDown() override {
    evas_object_smart_callback_del(GetEwkWebView(), kTitleChangedCb,
                                   TitleChanged);
  }

  static void TitleChanged(void* data, Evas_Object*, void* event_info) {
    if (!data) {
      utc_message("TitleChanged :: data is null");
      return;
    }
    auto owner = static_cast<utc_blink_ewk_intercept_request_ignore*>(data);
    if (!event_info) {
      utc_message("TitleChanged :: event_info is null");
      return;
    }
    auto title = static_cast<char*>(event_info);
    owner->title_ = title;
    if (!owner->url_.empty())
      owner->EventLoopStop(Success);
  }

  static void intercept_request_callback_ignore(
      Ewk_Context* /*ctx*/,
      Ewk_Intercept_Request* intercept_request,
      void* user_data) {
    auto* owner =
        static_cast<utc_blink_ewk_intercept_request_ignore*>(user_data);
    owner->api_result_ = ewk_intercept_request_ignore(intercept_request);
  }

  static void intercept_request_callback_ignore_null(
      Ewk_Context* /*ctx*/,
      Ewk_Intercept_Request* /*intercept_request*/,
      void* user_data) {
    auto* owner =
        static_cast<utc_blink_ewk_intercept_request_ignore*>(user_data);
    owner->api_result_ = ewk_intercept_request_ignore(nullptr);
  }

  std::string url_;
  std::string title_;
  ApiResultLocked<Eina_Bool> api_result_;
};

const char* utc_blink_ewk_intercept_request_ignore::kInterceptURL =
    "http://www.google.com/";

/**
 * @brief Tests if url loads if intercept request is ignored.
 */
TEST_F(utc_blink_ewk_intercept_request_ignore, POS_TEST_IGNORE) {
  ewk_context_intercept_request_callback_set(
      ewk_view_context_get(GetEwkWebView()), intercept_request_callback_ignore,
      this);

  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), kInterceptURL));
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_TRUE(api_result_.IsSet());
  EXPECT_TRUE(api_result_.Get());
  EXPECT_NE(std::string::npos, url_.find("google"))
      << "Substring \"google\" not in "
      << "\"" << url_ << "\"." << std::endl;
  EXPECT_NE(std::string::npos, title_.find("Google"))
      << "Substring \"Google\" not in "
      << "\"" << title_ << "\"." << std::endl;
}

/**
 * @brief Tests if EINA_FALSE is returned for null intercept request.
 */
TEST_F(utc_blink_ewk_intercept_request_ignore, NEG_TEST_IGNORE_NULL) {
  ewk_context_intercept_request_callback_set(
      ewk_view_context_get(GetEwkWebView()),
      intercept_request_callback_ignore_null, this);

  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), kInterceptURL));
  ASSERT_EQ(Timeout, EventLoopStart(3.0));
  ASSERT_TRUE(api_result_.IsSet());
  ASSERT_FALSE(api_result_.Get());
}
