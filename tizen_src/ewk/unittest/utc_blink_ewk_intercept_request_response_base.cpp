// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstring>

#include "utc_blink_ewk_intercept_request_response_base.h"

const char* utc_blink_ewk_intercept_request_response_base::kInterceptURL =
    "http://www.google.com/";

const char* utc_blink_ewk_intercept_request_response_base::kBodyAjaxPart1 =
    "<html><head><title>first intercepted</title></head>"
    "<body><script>"
    "function ajax_listener() {";

const char* utc_blink_ewk_intercept_request_response_base::kBodyAjaxPart2 =
    "}"
    "var ajax = new XMLHttpRequest();"
    "ajax.onload = ajax_listener;"
    "ajax.open('GET', '";

const char* utc_blink_ewk_intercept_request_response_base::kInterceptAjaxURL =
    "http://www.google.com/ewk-test/";

const char* utc_blink_ewk_intercept_request_response_base::kBodyAjaxPart3 =
    "', true);"
    "ajax.send();"
    "</script>"
    "</body></html>";

utc_blink_ewk_intercept_request_response_base::
    utc_blink_ewk_intercept_request_response_base()
    : body_ajax_response_("AJAX"),
      custom_response_status_text_(nullptr),
      intercept_request_null_(false) {}

utc_blink_ewk_intercept_request_response_base::
    ~utc_blink_ewk_intercept_request_response_base() {}

void utc_blink_ewk_intercept_request_response_base::PostSetUp() {
  evas_object_smart_callback_add(GetEwkWebView(), "title,changed",
                                 title_changed, this);
}

void utc_blink_ewk_intercept_request_response_base::PreTearDown() {
  evas_object_smart_callback_del(GetEwkWebView(), "title,changed",
                                 title_changed);
}

void utc_blink_ewk_intercept_request_response_base::LoadFinished(
    Evas_Object* webview) {
  // different title for first intercepted request
  first_title_ = ewk_view_title_get(webview);
}

void utc_blink_ewk_intercept_request_response_base::check_results(
    ApiResultLocked<ewk_callback_results>& results) {
  ASSERT_TRUE(results.IsSet());
  auto res = results.Get();
  EXPECT_TRUE(res.status_set_);
  EXPECT_TRUE(res.header_add_content_type_);
  EXPECT_TRUE(res.header_add_content_length_);
  EXPECT_TRUE(res.header_add_access_control_);
  EXPECT_TRUE(res.body_set_);
}

void utc_blink_ewk_intercept_request_response_base::construct_page() {
  body_ajax_ = kBodyAjaxPart1;
  body_ajax_ += get_js_title_test();
  body_ajax_ += kBodyAjaxPart2;
  body_ajax_ += kInterceptAjaxURL;
  body_ajax_ += kBodyAjaxPart3;
}

void utc_blink_ewk_intercept_request_response_base::title_changed(
    void* data,
    Evas_Object* webview,
    void* event_info) {
  auto* owner =
      static_cast<utc_blink_ewk_intercept_request_response_base*>(data);
  const char* title = ewk_view_title_get(owner->GetEwkWebView());

  if (strcmp(title, "first intercepted") != 0) {
    owner->EventLoopStop(Success);
  }
}

void utc_blink_ewk_intercept_request_response_base::
    intercept_request_callback_response_header_add(
        Ewk_Context* /*ctx*/,
        Ewk_Intercept_Request* intercept_request,
        void* user_data) {
  auto* owner =
      static_cast<utc_blink_ewk_intercept_request_response_base*>(user_data);
  const char* url = ewk_intercept_request_url_get(intercept_request);
  int body_length;
  const char* body;
  ApiResultLocked<ewk_callback_results>* results;

  if (strcmp(url, kInterceptAjaxURL) == 0) {
    // intercept ajax request
    body_length = strlen(owner->body_ajax_response_);
    body = owner->body_ajax_response_;
    results = &owner->ajax_ewk_results_;
  } else if (strcmp(url, kInterceptURL) == 0) {
    // intercept first page request
    body_length = owner->body_ajax_.length();
    body = owner->body_ajax_.c_str();
    results = &owner->page_ewk_results_;
  } else {
    ewk_intercept_request_ignore(intercept_request);
    return;
  }
  ewk_callback_results res;
  res.status_set_ = ewk_intercept_request_response_status_set(
      intercept_request, 200, owner->custom_response_status_text_);
  res.header_add_content_type_ = ewk_intercept_request_response_header_add(
      intercept_request, "Content-Type", "text/html; charset=UTF-8");
  res.header_add_content_length_ = ewk_intercept_request_response_header_add(
      intercept_request, "Content-Length", std::to_string(body_length).c_str());
  res.header_add_access_control_ = ewk_intercept_request_response_header_add(
      intercept_request, "Access-Control-Allow-Origin", "*");
  owner->pos_func(intercept_request);
  res.body_set_ = ewk_intercept_request_response_body_set(intercept_request,
                                                          body, body_length);
  results->Set(std::move(res));
}

void utc_blink_ewk_intercept_request_response_base::
    intercept_request_callback_response_header_add_null(
        Ewk_Context* /*ctx*/,
        Ewk_Intercept_Request* intercept_request,
        void* user_data) {
  auto* owner =
      static_cast<utc_blink_ewk_intercept_request_response_base*>(user_data);

  intercept_request =
      owner->intercept_request_null_ ? nullptr : intercept_request;
  owner->neg_api_result_ = owner->neg_func(intercept_request);
}

void utc_blink_ewk_intercept_request_response_base::pos_test() {
  construct_page();
  ewk_context_intercept_request_callback_set(
      ewk_view_context_get(GetEwkWebView()),
      intercept_request_callback_response_header_add, this);

  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), kInterceptURL));
  ASSERT_EQ(Success, EventLoopStart());
  // initial title from HTML
  EXPECT_EQ("first intercepted", first_title_);
  check_results(page_ewk_results_);
  check_results(ajax_ewk_results_);
}

void utc_blink_ewk_intercept_request_response_base::neg_test() {
  ewk_context_intercept_request_callback_set(
      ewk_view_context_get(GetEwkWebView()),
      intercept_request_callback_response_header_add_null, this);

  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), kInterceptURL));
  ASSERT_EQ(Timeout, EventLoopStart(3.0));
  ASSERT_TRUE(neg_api_result_.IsSet());
  ASSERT_EQ(EINA_FALSE, neg_api_result_.Get());
}
