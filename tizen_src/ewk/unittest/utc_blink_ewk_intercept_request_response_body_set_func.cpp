// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "utc_blink_ewk_intercept_request_response_base.h"

class utc_blink_ewk_intercept_request_response_body_set
    : public utc_blink_ewk_intercept_request_response_base {
 protected:
  std::string get_js_title_test() override {
    return std::string(body_ajax_test_);
  }

  void pos_func(Ewk_Intercept_Request* intercept_request) override {}

  bool neg_func(Ewk_Intercept_Request* intercept_request) override {
    return ewk_intercept_request_response_body_set(
        intercept_request, body_ajax_response_, body_ajax_length_);
  }

  const char* body_ajax_test_ =
      "document.title = this.responseText + ': ' + this.responseText.length;";

  int body_ajax_length_;
};

/**
 * @brief Tests if headers are correctly added from header map.
 */
TEST_F(utc_blink_ewk_intercept_request_response_body_set,
       POS_TEST_RESPONSE_BODY_SET) {
  body_ajax_response_ = "HELLO EWK API";
  pos_test();
  // title set in JS after ajax response:
  // <AJAX_RESPONSE>: length(<AJAX_RESPONSE>)
  EXPECT_STREQ("HELLO EWK API: 13", ewk_view_title_get(GetEwkWebView()));
}

/**
 * @brief Tests if EINA_FALSE is returned for NULL body
 */
TEST_F(utc_blink_ewk_intercept_request_response_body_set, NEG_TEST_NULL_BODY) {
  body_ajax_response_ = nullptr;
  // some positive value, so safety checks for length won't interfere
  body_ajax_length_ = 1;
  neg_test();
}

/**
 * @brief Tests if EINA_FALSE is returned for negative body length
 */
TEST_F(utc_blink_ewk_intercept_request_response_body_set,
       NEG_TEST_NEGATIVE_BODY_LENGTH) {
  body_ajax_length_ = -1;
  neg_test();
}

/**
 * @brief Tests if EINA_FALSE is returned for null intercept request.
 */
TEST_F(utc_blink_ewk_intercept_request_response_body_set,
       NEG_TEST_NULL_INTERCEPT_RESPONSE) {
  intercept_request_null_ = true;
  body_ajax_length_ = strlen(body_ajax_response_);
  neg_test();
}
