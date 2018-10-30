// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_notification_test_base.h"

class utc_blink_ewk_notification_security_origin_get : public utc_blink_ewk_notification_test_base
{
 protected:
  utc_blink_ewk_notification_security_origin_get()
    : origin_protocol(NULL)
    , origin_host(NULL)
    , origin_port(1)
  {}

  void NotificationShow(Ewk_Notification* notification) override {
    //call ewk_notification API
    const Ewk_Security_Origin* origin = ewk_notification_security_origin_get(notification);

    if (origin) {
      origin_protocol = eina_stringshare_add(ewk_security_origin_protocol_get(origin));
      origin_host = eina_stringshare_add(ewk_security_origin_host_get(origin));
      origin_port = ewk_security_origin_port_get(origin);
    }

    EventLoopStop(Success);
  }

 protected:
  Eina_Stringshare* origin_protocol;
  Eina_Stringshare* origin_host;
  uint16_t origin_port;
};

/**
* @brief Positive test case for ewk_notification_security_origin_get().
*/
TEST_F(utc_blink_ewk_notification_security_origin_get, POS_TEST)
{
  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), notification_sample_1.c_str()));
  ASSERT_EQ(Success, EventLoopStart());
  EXPECT_TRUE(origin_protocol);
  EXPECT_TRUE(origin_host);
  EXPECT_STREQ("", origin_protocol);
  EXPECT_STREQ("", origin_host);
  EXPECT_EQ(0, origin_port);
  eina_stringshare_del(origin_protocol);
  eina_stringshare_del(origin_host);
}

/**
* @brief Checking whether function works properly in case of nullptr of a webview.
*/
TEST_F(utc_blink_ewk_notification_security_origin_get, InvalidArg)
{
  EXPECT_EQ(nullptr, ewk_notification_security_origin_get(nullptr));
}
