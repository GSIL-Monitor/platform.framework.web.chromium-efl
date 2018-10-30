// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "utc_blink_ewk_intercept_request_response_base.h"

class utc_blink_ewk_intercept_request_response_header_add
    : public utc_blink_ewk_intercept_request_response_base {
 protected:
  std::string get_js_title_test() override {
    return std::string(body_ajax_test_);
  }

  void pos_func(Ewk_Intercept_Request* intercept_request) override {
    header_add_result_field_1_ = ewk_intercept_request_response_header_add(
        intercept_request, "TEST_FIELD_1", "TEST_VALUE_1");
    header_add_result_field_2_ = ewk_intercept_request_response_header_add(
        intercept_request, "TEST_FIELD_2", "TEST_VALUE_2");
  }

  bool neg_func(Ewk_Intercept_Request* intercept_request) override {
    return ewk_intercept_request_response_header_add(
        intercept_request, header_field_name_, header_field_value_);
  }

  const char* body_ajax_test_ =
      "document.title = 'TEST_FIELD_1: '"
      "+ this.getResponseHeader('TEST_FIELD_1') + ', TEST_FIELD_2: '"
      "+ this.getResponseHeader('TEST_FIELD_2');";

  const char* header_field_name_;
  const char* header_field_value_;
  bool header_add_result_field_1_;
  bool header_add_result_field_2_;
};

/**
 * @brief Tests if headers are correctly added.
 */
TEST_F(utc_blink_ewk_intercept_request_response_header_add,
       POS_TEST_RESPONSE_HEADER_ADD) {
  pos_test();
  EXPECT_TRUE(header_add_result_field_1_);
  EXPECT_TRUE(header_add_result_field_2_);
  /*
   * title set in JS after ajax response:
   * TEST_FIELD_1: <header('TEST_FIELD_1')>, \
   * TEST_FIELD_2: <header('TEST_FIELD_2')>
   */
  EXPECT_STREQ("TEST_FIELD_1: TEST_VALUE_1, TEST_FIELD_2: TEST_VALUE_2",
               ewk_view_title_get(GetEwkWebView()));
}

/**
 * @brief Tests if EINA_FALSE is returned for null header field name.
 */
TEST_F(utc_blink_ewk_intercept_request_response_header_add,
       NEG_TEST_NULL_HEADER_FIELD_NAME) {
  intercept_request_null_ = false;
  header_field_name_ = nullptr;
  header_field_value_ = "TEST";
  neg_test();
}

/**
 * @brief Tests if EINA_FALSE is returned for null header field value.
 */
TEST_F(utc_blink_ewk_intercept_request_response_header_add,
       NEG_TEST_NULL_HEADER_FIELD_VALUE) {
  intercept_request_null_ = false;
  header_field_name_ = "Server";
  header_field_value_ = nullptr;
  neg_test();
}

/**
 * @brief Tests if EINA_FALSE is returned for null intercept request.
 */
TEST_F(utc_blink_ewk_intercept_request_response_header_add,
       NEG_TEST_NULL_INTERCEPT_RESPONSE) {
  intercept_request_null_ = true;
  header_field_name_ = "Server";
  header_field_value_ = "TEST";
  neg_test();
}
