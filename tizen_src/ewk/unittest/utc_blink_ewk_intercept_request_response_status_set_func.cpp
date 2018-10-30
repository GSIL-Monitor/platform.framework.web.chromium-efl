// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "utc_blink_ewk_intercept_request_response_base.h"

class utc_blink_ewk_intercept_request_response_status_set
    : public utc_blink_ewk_intercept_request_response_base {
 protected:
  std::string get_js_title_test() override {
    return std::string(body_ajax_test_);
  }

  void pos_func(Ewk_Intercept_Request* intercept_request) override {}

  bool neg_func(Ewk_Intercept_Request* intercept_request) override {
    return ewk_intercept_request_response_status_set(nullptr, 200, "OK");
  }

  const char* body_ajax_test_ =
      "document.title = this.status + ' ' + this.statusText;";
};

/**
 * @brief Tests if status code and custom status text are set.
 */
TEST_F(utc_blink_ewk_intercept_request_response_status_set,
       POS_TEST_RESPONSE_STATUS_SET) {
  custom_response_status_text_ = "COOL";
  pos_test();
  // title set in JS after ajax response: <status_code> <status_text>
  EXPECT_STREQ("200 COOL", ewk_view_title_get(GetEwkWebView()));
}

/**
 * @brief Tests if status code and default status text are set.
 */
TEST_F(utc_blink_ewk_intercept_request_response_status_set,
       POS_TEST_RESPONSE_STATUS_SET_DEFAULT_TEXT) {
  custom_response_status_text_ = nullptr;
  pos_test();
  // title set in JS after ajax response: <status_code ><status_text>
  EXPECT_STREQ("200 OK", ewk_view_title_get(GetEwkWebView()));
}

/**
 * @brief Tests if EINA_FALSE is returned for null intercept request.
 */
TEST_F(utc_blink_ewk_intercept_request_response_status_set,
       NEG_TEST_NULL_INTERCEPT_RESPONSE) {
  intercept_request_null_ = true;
  neg_test();
}
