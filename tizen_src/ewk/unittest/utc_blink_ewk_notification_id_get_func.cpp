// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/* Define those macros _before_ you include the utc_blink_ewk.h header file. */

#include "utc_blink_ewk_notification_test_base.h"

class utc_blink_ewk_notification_id_get : public utc_blink_ewk_notification_test_base {
 protected:
  utc_blink_ewk_notification_id_get()
    : notification_id(0)
  {}

  void NotificationShow(Ewk_Notification* notification) override
  {
    //call ewk_notification API
    notification_id = ewk_notification_id_get(notification);
    EventLoopStop(Success);
  }

 protected:
  uint64_t notification_id;
};

/**
* @brief Positive test case for ewk_notification_id_get(). Text returned by
* api is compared against expected text and result is set in notificationShow()
*/
TEST_F(utc_blink_ewk_notification_id_get, POS_TEST)
{
  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), notification_sample_1.c_str()));
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_TRUE(notification_id);
}

/**
* @brief Checking whether function works properly in case of nullptr.
*/
TEST_F(utc_blink_ewk_notification_id_get, NEG_TEST)
{
  ASSERT_EQ(0, ewk_notification_id_get(nullptr));
}
