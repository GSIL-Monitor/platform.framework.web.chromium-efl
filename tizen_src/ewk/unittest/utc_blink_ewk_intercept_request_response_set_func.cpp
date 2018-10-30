// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

#include <cstring>
#include <string>

#include "utc_blink_api_result_locked.h"

class utc_blink_ewk_intercept_request_response_set : public utc_blink_ewk_base {
 public:
  utc_blink_ewk_intercept_request_response_set() {
    body_length_ = strlen(kBody);
    int headers_length =
        snprintf(nullptr, 0, kHeadersTemplate, body_length_) + 1;  // for '\0'
    headers_ = new char[headers_length];
    snprintf(headers_, headers_length, kHeadersTemplate, body_length_);

    body_ajax_length_ = strlen(kBodyAjax);
    int headers_ajax_length =
        snprintf(nullptr, 0, kHeadersTemplate, body_ajax_length_) +
        1;  // for '\0'
    headers_ajax_ = new char[headers_ajax_length];
    snprintf(headers_ajax_, headers_ajax_length, kHeadersTemplate,
             body_ajax_length_);

    body_ajax_response_length_ = strlen(kBodyAjaxResponse);
    int headers_ajax_response_length =
        snprintf(nullptr, 0, kHeadersTemplate, body_ajax_response_length_) +
        1;  // for '\0'
    headers_ajax_response_ = new char[headers_ajax_response_length];
    snprintf(headers_ajax_response_, headers_ajax_response_length,
             kHeadersTemplate, body_ajax_response_length_);
  }

  ~utc_blink_ewk_intercept_request_response_set() override {
    delete[] headers_;
    delete[] headers_ajax_;
    delete[] headers_ajax_response_;
  }

 protected:
  static const char* kInterceptURL;
  static const char* kHeadersTemplate;
  static const char* kBody;
  static const char* kBodyAjax;
  static const char* kBodyAjaxResponse;

  void PostSetUp() override {
    evas_object_smart_callback_add(GetEwkWebView(), "title,changed",
                                   title_changed, this);
  }

  void PreTearDown() override {
    evas_object_smart_callback_del(GetEwkWebView(), "title,changed",
                                   title_changed);
  }

  void LoadFinished(Evas_Object* webview) override {
    // different title for first intercepted request
    first_title_ = ewk_view_title_get(webview);
  }

  static void title_changed(void* data,
                            Evas_Object* webview,
                            void* event_info) {
    auto* owner =
        static_cast<utc_blink_ewk_intercept_request_response_set*>(data);
    const char* title = ewk_view_title_get(owner->GetEwkWebView());
    if (strcmp(title, "first intercepted") != 0) {
      owner->EventLoopStop(Success);
    }
  }

  static void intercept_request_callback_response_set_ajax(
      Ewk_Context* /*ctx*/,
      Ewk_Intercept_Request* intercept_request,
      void* user_data) {
    auto* owner =
        static_cast<utc_blink_ewk_intercept_request_response_set*>(user_data);

    const char* url = ewk_intercept_request_url_get(intercept_request);
    if (strcmp(url, "http://www.google.com/ewk-test/") == 0) {
      // intercept ajax request
      owner->api_result_ajax_ = ewk_intercept_request_response_set(
          intercept_request, owner->headers_ajax_response_, kBodyAjaxResponse,
          owner->body_ajax_response_length_);
    } else {
      // intercept first page request
      owner->api_result_page_ = ewk_intercept_request_response_set(
          intercept_request, owner->headers_ajax_, kBodyAjax,
          owner->body_ajax_length_);
    }
  }

  static void intercept_request_callback_response_set_null_request_intercept(
      Ewk_Context* /*ctx*/,
      Ewk_Intercept_Request* /*intercept_request*/,
      void* user_data) {
    auto* owner =
        static_cast<utc_blink_ewk_intercept_request_response_set*>(user_data);
    owner->api_result_ = ewk_intercept_request_response_set(
        nullptr, owner->headers_, kBody, owner->body_length_);
  }

  static void intercept_request_callback_response_set_null_headers(
      Ewk_Context* /*ctx*/,
      Ewk_Intercept_Request* intercept_request,
      void* user_data) {
    auto* owner =
        static_cast<utc_blink_ewk_intercept_request_response_set*>(user_data);
    owner->api_result_ = ewk_intercept_request_response_set(
        intercept_request, nullptr, kBody, owner->body_length_);
  }

  static void intercept_request_callback_response_set_null_body(
      Ewk_Context* /*ctx*/,
      Ewk_Intercept_Request* intercept_request,
      void* user_data) {
    auto* owner =
        static_cast<utc_blink_ewk_intercept_request_response_set*>(user_data);
    owner->api_result_ = ewk_intercept_request_response_set(
        intercept_request, owner->headers_, nullptr, owner->body_length_)
  }

  static void intercept_request_callback_response_set_negative_length(
      Evas_Object* o,
      Ewk_Intercept_Request* intercept_request,
      void* user_data) {
    auto* owner =
        static_cast<utc_blink_ewk_intercept_request_response_set*>(user_data);
    if (ewk_intercept_request_response_set(intercept_request, owner->headers_,
                                           kBody, -1))
      owner->EventLoopStop(Failure);
    else
      owner->EventLoopStop(Success);
  }

  int body_length_;
  int body_ajax_length_;
  int body_ajax_response_length_;
  char* headers_;
  char* headers_ajax_;
  char* headers_ajax_response_;
  std::string first_title_;
  ApiResultLocked<Eina_Bool> api_result_;
  ApiResultLocked<Eina_Bool> api_result_ajax_;
  ApiResultLocked<Eina_Bool> api_result_page_;
};

const char* utc_blink_ewk_intercept_request_response_set::kInterceptURL =
    "http://www.google.com/";

const char* utc_blink_ewk_intercept_request_response_set::kHeadersTemplate =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html; charset=UTF-8\r\n"
    "Content-Length: %d\r\n"
    "Access-Control-Allow-Origin: *\r\n"
    "Server: TEST\r\n\r\n";

const char* utc_blink_ewk_intercept_request_response_set::kBody =
    "<html><head><title>intercepted</title></head>"
    "<body>Hello, Request Intercept!</body></html>";

const char* utc_blink_ewk_intercept_request_response_set::kBodyAjax =
    "<html><head><title>first intercepted</title></head>"
    "<body><script>"
    "function ajax_listener() {"
    "document.title = this.responseText + ' ' + this.status + ' ' + "
    "this.statusText + ' ' + this.getResponseHeader('Server');"
    "}"
    "var ajax = new XMLHttpRequest();"
    "ajax.onload = ajax_listener;"
    "ajax.open('GET', 'http://www.google.com/ewk-test/', true);"
    "ajax.send();"
    "</script>"
    "</body></html>";

const char* utc_blink_ewk_intercept_request_response_set::kBodyAjaxResponse =
    "AJAX";

/**
 * @brief Tests if body and custom headers are set.
 */
TEST_F(utc_blink_ewk_intercept_request_response_set, POS_TEST_RESPONSE_SET) {
  ewk_context_intercept_request_callback_set(
      ewk_view_context_get(GetEwkWebView()),
      intercept_request_callback_response_set_ajax, this);

  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), kInterceptURL));
  ASSERT_EQ(Success, EventLoopStart());
  EXPECT_FALSE(first_title_.empty());
  EXPECT_EQ(std::string::npos, first_title_.find("Google"))
      << "Substring \"Google\" in "
      << "\"" << first_title_ << "\"." << std::endl;
  EXPECT_NE(std::string::npos, first_title_.find("first intercepted"))
      << "Substring \"first intercepted\" not in "
      << "\"" << first_title_ << "\"." << std::endl;
  ASSERT_TRUE(api_result_page_.IsSet());
  ASSERT_EQ(EINA_TRUE, api_result_page_.Get());
  ASSERT_TRUE(api_result_ajax_.IsSet());
  ASSERT_EQ(EINA_TRUE, api_result_ajax_.Get());
  // title set in JS after ajax response:
  // <response_body> <status_code> <status_text> <header('Server')>
  EXPECT_STREQ("AJAX 200 OK TEST", ewk_view_title_get(GetEwkWebView()));
}

/**
 * @brief Tests if EINA_FALSE is returned for null intercept request.
 */
TEST_F(utc_blink_ewk_intercept_request_response_set,
       NEG_TEST_NULL_INTERCEPT_RESPONSE) {
  ewk_context_intercept_request_callback_set(
      ewk_view_context_get(GetEwkWebView()),
      intercept_request_callback_response_set_null_request_intercept, this);

  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), kInterceptURL));
  ASSERT_EQ(Timeout, EventLoopStart(3.0));
  ASSERT_TRUE(api_result_.IsSet());
  ASSERT_EQ(EINA_FALSE, api_result_.Get());
}

/**
 * @brief Tests if EINA_FALSE is returned for null headers.
 */
TEST_F(utc_blink_ewk_intercept_request_response_set, NEG_TEST_NULL_HEADERS) {
  ewk_context_intercept_request_callback_set(
      ewk_view_context_get(GetEwkWebView()),
      intercept_request_callback_response_set_null_headers, this);

  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), kInterceptURL));
  ASSERT_EQ(Timeout, EventLoopStart(3.0));
  ASSERT_TRUE(api_result_.IsSet());
  ASSERT_EQ(EINA_FALSE, api_result_.Get());
}

/**
 * @brief Tests if EINA_FALSE is returned for null body.
 */
TEST_F(utc_blink_ewk_intercept_request_response_set, NEG_TEST_NULL_BODY) {
  ewk_view_intercept_request_callback_set(
      GetEwkWebView(), intercept_request_callback_response_set_null_body, this);

  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), kInterceptURL));
  ASSERT_EQ(Success, EventLoopStart());
}

/**
 * @brief Tests if EINA_FALSE is returned for negative body length.
 */
TEST_F(utc_blink_ewk_intercept_request_response_set, NEG_TEST_NEGATIVE_LENGTH) {
  ewk_context_intercept_request_callback_set(
      ewk_view_context_get(GetEwkWebView()),
      intercept_request_callback_response_set_negative_length, this);

  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), kInterceptURL));
  ASSERT_EQ(Timeout, EventLoopStart(3.0));
  ASSERT_TRUE(api_result_.IsSet());
  ASSERT_EQ(EINA_FALSE, api_result_.Get());
}
