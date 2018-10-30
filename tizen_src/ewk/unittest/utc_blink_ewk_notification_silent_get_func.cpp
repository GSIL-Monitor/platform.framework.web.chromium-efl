// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/* Define those macros _before_ you include the utc_blink_ewk.h header file. */
#include "utc_blink_ewk_notification_test_base.h"

class utc_blink_ewk_notification_silent_get : public utc_blink_ewk_notification_test_base {
 protected:
  utc_blink_ewk_notification_silent_get()
      : silent(false), notification_silent_ref(true) {}

  ~utc_blink_ewk_notification_silent_get() override {}

  void NotificationShow(Ewk_Notification* notification) override {
    silent = ewk_notification_silent_get(notification);
    EventLoopStop(Success);
  }

 protected:
  bool silent;
  const bool notification_silent_ref;
};

/**
* @brief Positive test case for ewk_notification_silent_get(). Text returned by api is compared against expected text and result is set in notificationShow()
*/
TEST_F(utc_blink_ewk_notification_silent_get, POS_TEST)
{
  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), notification_sample_1.c_str()));
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_EQ(notification_silent_ref, silent);
}

/**
* @brief Checking whether function works properly in case of nullptr of a webview.
*/
TEST_F(utc_blink_ewk_notification_silent_get, NEG_TEST)
{
  ASSERT_FALSE(ewk_notification_silent_get(nullptr));
}
