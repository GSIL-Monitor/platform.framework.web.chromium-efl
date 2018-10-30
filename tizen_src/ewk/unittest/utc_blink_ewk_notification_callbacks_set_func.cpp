// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_notification_test_base.h"

class utc_blink_ewk_notification_callbacks_set : public utc_blink_ewk_notification_test_base {
 protected:
  utc_blink_ewk_notification_callbacks_set()
    : notification_id(0)
  {}

  void NotificationShow(Ewk_Notification* notification) override
  {
    notification_id = ewk_notification_id_get(notification);
    EventLoopStop(Success);
  }

  void NotificationCancel(uint64_t notification_id) override
  {
    EventLoopStop(Failure);
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

  static void dummy_notification_show_callback(Ewk_Notification* notification, void* user_data)
  {
  }

  static void dummy_notification_cancel_callback(uint64_t notificationId, void* user_data)
  {
  }

 protected:
  std::string console_message;
  uint64_t notification_id;
};

/**
* @brief Check if all callbacks are called
*/
TEST_F(utc_blink_ewk_notification_callbacks_set, POS_TEST)
{
  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), notification_sample_1.c_str()));
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_TRUE(notification_id);
  ASSERT_TRUE(ewk_notification_showed(notification_id));
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_STREQ("notification.show", console_message.c_str());
  ewk_view_script_execute(GetEwkWebView(), "new Notification('Title', {tag: 'replace_tag'});", nullptr, nullptr);
  ASSERT_EQ(Failure, EventLoopStart());
}

/**
* @brief Checking whether function works properly in case of nullptr parameters
*/
TEST_F(utc_blink_ewk_notification_callbacks_set, NEG_TEST)
{
  // both callbacks must be set
  ASSERT_FALSE(ewk_notification_callbacks_set(nullptr, nullptr, nullptr));
  ASSERT_FALSE(ewk_notification_callbacks_set(dummy_notification_show_callback, nullptr, nullptr));
  ASSERT_FALSE(ewk_notification_callbacks_set(nullptr, dummy_notification_cancel_callback, nullptr));
}
