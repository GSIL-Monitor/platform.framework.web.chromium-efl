// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "utc_blink_ewk_intercept_request_get_base.h"

class utc_blink_ewk_intercept_request_scheme_get
    : public utc_blink_ewk_intercept_request_get_base {
 public:
  utc_blink_ewk_intercept_request_scheme_get()
      : scheme_from_ewk_ptr_(nullptr) {}

 protected:
  void test_func(Ewk_Intercept_Request* intercept_request) override {
    scheme_from_ewk_ptr_ = ewk_intercept_request_scheme_get(intercept_request);
    if (scheme_from_ewk_ptr_) {
      scheme_from_ewk_ = scheme_from_ewk_ptr_;
    }
  }

  std::string scheme_from_ewk_;
  const char* scheme_from_ewk_ptr_;
  static const std::string kInterceptScheme;
};

const std::string utc_blink_ewk_intercept_request_scheme_get::kInterceptScheme =
    kInterceptURL.substr(0, kInterceptURL.find(':'));

/**
 * @brief Tests if correct scheme is returned for intercept request.
 */
TEST_F(utc_blink_ewk_intercept_request_scheme_get, POS_TEST_SCHEME_GET) {
  pos_test();
  ASSERT_EQ(kInterceptScheme, scheme_from_ewk_);
}

/**
 * @brief Tests if null scheme is returned for null intercept request.
 */
TEST_F(utc_blink_ewk_intercept_request_scheme_get, NEG_TEST_SCHEME_GET_NULL) {
  neg_test();
  ASSERT_EQ(nullptr, scheme_from_ewk_ptr_);
}
