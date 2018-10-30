// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_notification_test_base.h"

class utc_blink_ewk_notification_icon_save_as_png : public utc_blink_ewk_notification_test_base {
 protected:
  utc_blink_ewk_notification_icon_save_as_png()
    : saved_to_file(false)
    , notification_icon_path("/tmp/notification_icon.png")
  {}

  void NotificationShow(Ewk_Notification* notification) override
  {
    saved_to_file = ewk_notification_icon_save_as_png(notification, notification_icon_path);
    EventLoopStop(Success);
  }

 protected:
  bool saved_to_file;
  const char *notification_icon_path;
};

/**
* @brief Positive test case for ewk_notification_icon_get()
*/
TEST_F(utc_blink_ewk_notification_icon_save_as_png, POS_TEST)
{
  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), notification_sample_1.c_str()));
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_TRUE(saved_to_file);
  Evas_Object* img = evas_object_image_add(GetEwkEvas());
  ASSERT_TRUE(img);
  evas_object_image_file_set(img, notification_icon_path, NULL);
  // Skia and Efl may decode png in different way, we add fuzziness of 3 points
  // difference between pixel values
  ASSERT_TRUE(CompareEvasImageWithResource(img, "/common/logo.png", 3));
  evas_object_del(img);
}

/**
* @brief Checking whether function works properly in case of nullptr value pass
*/
TEST_F(utc_blink_ewk_notification_icon_save_as_png, NEG_TEST)
{
  ASSERT_FALSE(ewk_notification_icon_save_as_png(nullptr, nullptr));
  ASSERT_FALSE(ewk_notification_icon_save_as_png(
      reinterpret_cast<Ewk_Notification*>(1), nullptr));
  ASSERT_FALSE(ewk_notification_icon_save_as_png(nullptr,
      notification_icon_path));
}
