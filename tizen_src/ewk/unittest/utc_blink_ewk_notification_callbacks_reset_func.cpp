// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_notification_test_base.h"

class utc_blink_ewk_notification_callbacks_reset : public utc_blink_ewk_notification_test_base {
 protected:
  utc_blink_ewk_notification_callbacks_reset()
    : notification_id(0)
  {}

  void NotificationShow(Ewk_Notification* notification) override
  {
    notification_id = ewk_notification_id_get(notification);
    EventLoopStop(Success);
  }

 protected:
  uint64_t notification_id;
};

/**
* @brief Check if all callbacks are called
*/
TEST_F(utc_blink_ewk_notification_callbacks_reset, POS_TEST)
{
  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), notification_sample_1.c_str()));
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_TRUE(notification_id);

  notification_id = 0;
  ASSERT_EQ(EINA_TRUE, ewk_notification_callbacks_reset());
  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), notification_sample_1.c_str()));
  ASSERT_EQ(Timeout, EventLoopStart(10.0)); // 10 seconds should be enough
  ASSERT_EQ(0, notification_id);
}

