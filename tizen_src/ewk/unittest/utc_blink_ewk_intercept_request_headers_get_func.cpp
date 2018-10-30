// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "utc_blink_ewk_intercept_request_get_base.h"

class utc_blink_ewk_intercept_request_headers_get
    : public utc_blink_ewk_intercept_request_get_base {
 public:
  utc_blink_ewk_intercept_request_headers_get() : headers_from_ewk_(nullptr) {}

 protected:
  void test_func(Ewk_Intercept_Request* intercept_request) override {
    headers_from_ewk_ = ewk_intercept_request_headers_get(intercept_request);
    if (headers_from_ewk_) {
      const char* accept =
          static_cast<const char*>(eina_hash_find(headers_from_ewk_, "Accept"));
      if (accept) {
        accept_header_from_ewk_ = accept;
      }
      const char* user_agent = static_cast<const char*>(
          eina_hash_find(headers_from_ewk_, "User-Agent"));
      if (user_agent) {
        user_agent_header_from_ewk_ = user_agent;
      }
    }
  }

  const Eina_Hash* headers_from_ewk_;
  std::string accept_header_from_ewk_;
  std::string user_agent_header_from_ewk_;
};

/**
 * @brief Tests if headers are returned for intercept request.
 */
TEST_F(utc_blink_ewk_intercept_request_headers_get, POS_TEST_HEADERS_GET) {
  pos_test();
  ASSERT_NE(nullptr, headers_from_ewk_);
  EXPECT_NE(std::string::npos, accept_header_from_ewk_.find("text/html"))
      << "Substring \"text/html\" not in "
      << "\"" << accept_header_from_ewk_ << "\"." << std::endl;
  EXPECT_NE(std::string::npos, user_agent_header_from_ewk_.find("KHTML"))
      << "Substring \"KHTML\" not in "
      << "\"" << user_agent_header_from_ewk_ << "\"." << std::endl;
}

/**
 * @brief Tests if null headers are returned for null intercept request.
 */
TEST_F(utc_blink_ewk_intercept_request_headers_get, NEG_TEST_HEADERS_GET_NULL) {
  neg_test();
  ASSERT_EQ(nullptr, headers_from_ewk_);
}
