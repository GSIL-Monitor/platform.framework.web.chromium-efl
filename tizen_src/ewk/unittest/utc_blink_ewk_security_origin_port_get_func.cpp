// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_notification_test_base.h"

class utc_blink_ewk_security_origin_port_get : public utc_blink_ewk_notification_test_base
{
 protected:
  void NotificationShow(Ewk_Notification* notification) override {
    MainLoopResult result = Failure;

    const Ewk_Security_Origin* org = ewk_notification_security_origin_get((const Ewk_Notification*)notification);
    if (org) {
      uint16_t port = ewk_security_origin_port_get(org);
      if (port == 0)
        result = Success;
    }

    EventLoopStop(result);
  }
};

/**
 * @brief Positive test case for ewk_security_origin_protocol_get().
 */
TEST_F(utc_blink_ewk_security_origin_port_get, POS_TEST)
{
  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(),notification_sample_1.c_str()));
  EXPECT_EQ(Success, EventLoopStart());
}

/**
 * @brief Checking whether function works properly in case of nullptr as origin.
 */
TEST_F(utc_blink_ewk_security_origin_port_get, NEG_TEST)
{
  EXPECT_EQ(0, ewk_security_origin_port_get(nullptr));

}
