// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_hit_test_request_base.h"

class utc_blink_ewk_hit_test_tag_name_get : public utc_blink_ewk_hit_test_request_base {
protected:
  void CheckHitTest(Ewk_Hit_Test* hit_test) override {
    ASSERT_TRUE(ewk_hit_test_tag_name_get(hit_test));
  }

  static const char* const test_path;
};

const char* const utc_blink_ewk_hit_test_tag_name_get::test_path = "/ewk_hit_test/index.html";

/**
 * @brief Checking whether the tag name of hit element is returned properly.
 */
TEST_F(utc_blink_ewk_hit_test_tag_name_get, POS_TEST1)
{
  ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(), GetResourceUrl(test_path).c_str()));
  ASSERT_EQ(Success, EventLoopStart());

  ewk_view_hit_test_request(GetEwkWebView(), 80, 100, EWK_HIT_TEST_MODE_NODE_DATA, hit_test_result, this);
  ASSERT_EQ(Success, EventLoopStart());
}

/**
 * @brief Checking whether function works properly in case of NULL of a hit test instance.
 */
TEST_F(utc_blink_ewk_hit_test_tag_name_get, NEG_TEST1)
{
  utc_check_eq(0, ewk_hit_test_tag_name_get(NULL));
}
