// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_notification_test_base.h"

class utc_blink_ewk_notification_permission_request_suspend : public utc_blink_ewk_notification_test_base {
 protected:
  utc_blink_ewk_notification_permission_request_suspend()
    : permission_request(NULL)
  {}

  /* Callback for notification permission request */
  Eina_Bool NotificationPermissionRequest(
      Evas_Object* webview,
      Ewk_Notification_Permission_Request* request) override {
    EXPECT_EQ(EINA_TRUE, ewk_notification_permission_request_suspend(request));
    permission_request = request;
    EventLoopStop(Success);
    return EINA_TRUE;
  }

  void NotificationShow(Ewk_Notification* notification) override {
    EventLoopStop(Success);
  }

 protected:
  Ewk_Notification_Permission_Request* permission_request;
};

/**
* @brief Positive test case for ewk_notification_permission_request_suspened()
*/
TEST_F(utc_blink_ewk_notification_permission_request_suspend, POS_TEST)
{
  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), notification_sample_1.c_str()));
  EXPECT_EQ(Success, EventLoopStart());
  ASSERT_TRUE(permission_request);
  ASSERT_EQ(EINA_TRUE, ewk_notification_permission_reply(permission_request, EINA_TRUE));
  permission_request = NULL;
  ASSERT_EQ(Success, EventLoopStart());
}

/**
* @brief Checking whether function works properly in case of nullptr value pass
*/
TEST_F(utc_blink_ewk_notification_permission_request_suspend, NEG_TEST)
{
  ASSERT_EQ(EINA_FALSE, ewk_notification_permission_request_suspend(nullptr));
}
