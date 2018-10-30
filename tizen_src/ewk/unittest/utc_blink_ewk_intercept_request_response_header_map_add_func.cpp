// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "utc_blink_ewk_intercept_request_response_base.h"

class utc_blink_ewk_intercept_request_response_header_map_add
    : public utc_blink_ewk_intercept_request_response_base {
 protected:
  utc_blink_ewk_intercept_request_response_header_map_add()
      : header_map_(nullptr) {}

  ~utc_blink_ewk_intercept_request_response_header_map_add() override {
    if (header_map_)
      eina_hash_free(header_map_);
  }

  std::string get_js_title_test() override {
    return std::string(body_ajax_test_);
  }

  void pos_func(Ewk_Intercept_Request* intercept_request) override {
    header_add_map_result_ = ewk_intercept_request_response_header_map_add(
        intercept_request, header_map_);
  }

  bool neg_func(Ewk_Intercept_Request* intercept_request) override {
    return ewk_intercept_request_response_header_map_add(intercept_request,
                                                         header_map_);
  }

  void build_header_map() {
    header_map_ = eina_hash_string_small_new(nullptr);
    eina_hash_add(header_map_, "TEST_FIELD_1", "TEST_VALUE_1");
    eina_hash_add(header_map_, "TEST_FIELD_2", "TEST_VALUE_2");
  }

  const char* body_ajax_test_ =
      "document.title = 'TEST_FIELD_1: '"
      "+ this.getResponseHeader('TEST_FIELD_1') + ', TEST_FIELD_2: '"
      "+ this.getResponseHeader('TEST_FIELD_2');";

  bool header_add_map_result_;
  Eina_Hash* header_map_;
};

/**
 * @brief Tests if headers are correctly added from header map.
 */
TEST_F(utc_blink_ewk_intercept_request_response_header_map_add,
       POS_TEST_RESPONSE_HEADER_ADD) {
  build_header_map();
  pos_test();
  EXPECT_TRUE(header_add_map_result_);
  /*
   * title set in JS after ajax response:
   * TEST_FIELD_1: <header('TEST_FIELD_1')>, \
   * TEST_FIELD_2: <header('TEST_FIELD_2')>
   */
  EXPECT_STREQ("TEST_FIELD_1: TEST_VALUE_1, TEST_FIELD_2: TEST_VALUE_2",
               ewk_view_title_get(GetEwkWebView()));
}

/**
 * @brief Tests if EINA_FALSE is returned for null header map.
 */
TEST_F(utc_blink_ewk_intercept_request_response_header_map_add,
       NEG_TEST_NULL_HEADER_FIELD_NAME) {
  intercept_request_null_ = false;
  header_map_ = nullptr;
  neg_test();
}

/**
 * @brief Tests if EINA_FALSE is returned for null intercept request.
 */
TEST_F(utc_blink_ewk_intercept_request_response_header_map_add,
       NEG_TEST_NULL_INTERCEPT_RESPONSE) {
  intercept_request_null_ = true;
  build_header_map();
  neg_test();
}
