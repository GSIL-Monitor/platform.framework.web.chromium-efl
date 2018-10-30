// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_notification_test_base.h"

class utc_blink_ewk_notification_body_get : public utc_blink_ewk_notification_test_base {
 protected:
  utc_blink_ewk_notification_body_get()
    : body(NULL)
  {}

  void NotificationShow(Ewk_Notification* notification) override
  {
    //call ewk_notification API
    body = eina_stringshare_add(ewk_notification_body_get(notification));
    EventLoopStop(Success);
  }

 protected:
  static const char* const notification_body_ref;
  Eina_Stringshare* body;
};

const char* const utc_blink_ewk_notification_body_get::notification_body_ref = "Notification body content";

/**
* @brief Positive test case for ewk_notification_body_get(). Text returned by api is compared against expected text and result is set in notificationShow()
*/
TEST_F(utc_blink_ewk_notification_body_get, POS_TEST)
{
  ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(), notification_sample_1.c_str()));
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_TRUE(body);
  ASSERT_STREQ(body, notification_body_ref);
  eina_stringshare_del(body);
}

/**
* @brief Checking whether function works properly in case of nullptr of a webview.
*/
TEST_F(utc_blink_ewk_notification_body_get, NEG_TEST)
{
  ASSERT_EQ(nullptr, ewk_notification_body_get(nullptr));
}
