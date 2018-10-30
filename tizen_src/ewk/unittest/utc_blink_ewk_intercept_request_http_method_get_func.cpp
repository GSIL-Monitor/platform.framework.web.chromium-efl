// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "utc_blink_ewk_intercept_request_get_base.h"

class utc_blink_ewk_intercept_request_http_method_get
    : public utc_blink_ewk_intercept_request_get_base {
 public:
  utc_blink_ewk_intercept_request_http_method_get()
      : http_method_from_ewk_ptr_(nullptr) {}

 protected:
  void test_func(Ewk_Intercept_Request* intercept_request) override {
    http_method_from_ewk_ptr_ =
        ewk_intercept_request_http_method_get(intercept_request);
    if (http_method_from_ewk_ptr_) {
      http_method_from_ewk_ = http_method_from_ewk_ptr_;
    }
  }

  std::string http_method_from_ewk_;
  const char* http_method_from_ewk_ptr_;
};

/**
 * @brief Tests if correct http method is returned for intercept request.
 */
TEST_F(utc_blink_ewk_intercept_request_http_method_get, POS_TEST_METHOD_GET) {
  pos_test();
  ASSERT_EQ("GET", http_method_from_ewk_);
}

/**
 * @brief Tests if null http method is returned for null intercept request.
 */
TEST_F(utc_blink_ewk_intercept_request_http_method_get,
       NEG_TEST_METHOD_GET_NULL) {
  neg_test();
  ASSERT_EQ(nullptr, http_method_from_ewk_ptr_);
}
