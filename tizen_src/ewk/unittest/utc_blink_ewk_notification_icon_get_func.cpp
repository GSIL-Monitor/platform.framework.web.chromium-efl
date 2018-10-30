// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_notification_test_base.h"

class utc_blink_ewk_notification_icon_get : public utc_blink_ewk_notification_test_base {
 protected:
  utc_blink_ewk_notification_icon_get()
    : notification_icon(NULL)
  {}

  void NotificationShow(Ewk_Notification* notification) override
  {
    notification_icon = ewk_notification_icon_get(notification, GetEwkEvas());
    EventLoopStop(Success);
  }

 protected:
  Evas_Object* notification_icon;
};

/**
* @brief Positive test case for ewk_notification_icon_get()
*/
TEST_F(utc_blink_ewk_notification_icon_get, POS_TEST)
{
  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), notification_sample_1.c_str()));
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_TRUE(notification_icon);
  // Skia and Efl may decode png in different way, we add fuzziness of 3 points
  // difference between pixel values
  ASSERT_TRUE(CompareEvasImageWithResource(notification_icon, "/common/logo.png", 3));
  evas_object_del(notification_icon);
}

/**
* @brief Checking whether function works properly in case of nullptr value pass
*/
TEST_F(utc_blink_ewk_notification_icon_get, NEG_TEST)
{
  ASSERT_FALSE(ewk_notification_icon_get(nullptr, nullptr));
  ASSERT_FALSE(ewk_notification_icon_get(nullptr, GetEwkEvas()));
}
