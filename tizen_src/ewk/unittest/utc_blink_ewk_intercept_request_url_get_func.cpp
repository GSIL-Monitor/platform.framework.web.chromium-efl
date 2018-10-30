// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "utc_blink_ewk_intercept_request_get_base.h"

class utc_blink_ewk_intercept_request_url_get
    : public utc_blink_ewk_intercept_request_get_base {
 public:
  utc_blink_ewk_intercept_request_url_get() : url_from_ewk_ptr_(nullptr) {}

 protected:
  void test_func(Ewk_Intercept_Request* intercept_request) override {
    url_from_ewk_ptr_ = ewk_intercept_request_url_get(intercept_request);
    if (url_from_ewk_ptr_) {
      url_from_ewk_ = url_from_ewk_ptr_;
    }
  }

  std::string url_from_ewk_;
  const char* url_from_ewk_ptr_;
};

/**
 * @brief Tests if correct url is returned for intercept request.
 */
TEST_F(utc_blink_ewk_intercept_request_url_get, POS_TEST_URL_GET) {
  pos_test();
  ASSERT_EQ(kInterceptURL, url_from_ewk_);
}

/**
 * @brief Tests if null url is returned for null intercept request.
 */
TEST_F(utc_blink_ewk_intercept_request_url_get, NEG_TEST_URL_GET_NULL) {
  neg_test();
  ASSERT_EQ(nullptr, url_from_ewk_ptr_);
}
