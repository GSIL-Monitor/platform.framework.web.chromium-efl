// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_notification_test_base.h"

class utc_blink_ewk_notification_clicked : public utc_blink_ewk_notification_test_base
{
 protected:
  utc_blink_ewk_notification_clicked()
    : notification_id(0)
  {}

  void NotificationShow(Ewk_Notification* notification) override
  {
    notification_id = ewk_notification_id_get(notification);
    EventLoopStop(Success);;
  }

  void ConsoleMessage(Evas_Object* webview, const Ewk_Console_Message* message) override
  {
    if (message) {
      const char* message_text = ewk_console_message_text_get(message);
      if (message_text) {
        console_message = message_text;
      }
    }

    EventLoopStop(Success);
  }

 protected:
  uint64_t notification_id;
  std::string console_message;
};

/**
* @brief Positive test case for ewk_notification_clicked()
*/
TEST_F(utc_blink_ewk_notification_clicked, POS_TEST)
{
  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), notification_sample_1.c_str()));
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_TRUE(notification_id);
  ASSERT_TRUE(ewk_notification_showed(notification_id));
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_STREQ("notification.show", console_message.c_str());
  ASSERT_TRUE(ewk_notification_clicked(notification_id));
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_STREQ("notification.click", console_message.c_str());
}

/**
* @brief Checking whether function works properly in case of invalid notification id value pass
*/
TEST_F(utc_blink_ewk_notification_clicked, NEG_TEST)
{
  ASSERT_FALSE(ewk_notification_clicked(0));
}
