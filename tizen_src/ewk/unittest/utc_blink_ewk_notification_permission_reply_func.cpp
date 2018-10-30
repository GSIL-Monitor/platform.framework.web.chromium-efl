// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_notification_test_base.h"

class utc_blink_ewk_notification_permission_reply : public utc_blink_ewk_notification_test_base {
 protected:
  utc_blink_ewk_notification_permission_reply()
    : notification_permission_request_cnt(0)
  {}

  Eina_Bool NotificationPermissionRequest(
      Evas_Object* webview,
      Ewk_Notification_Permission_Request* request) override {
    EXPECT_EQ(EINA_TRUE, ewk_notification_permission_reply(request, EINA_TRUE));
    ++notification_permission_request_cnt;
    return EINA_TRUE;
  }

  void NotificationShow(Ewk_Notification* notification) override {
    EventLoopStop(Success);
  }

 protected:
  int notification_permission_request_cnt;
};

/**
 * @brief Positive test case for ewk_notification_permission_reply function
 */
TEST_F(utc_blink_ewk_notification_permission_reply, POS_TEST)
{
  ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(), notification_sample_1.c_str()));
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_EQ(1, notification_permission_request_cnt);

  ASSERT_EQ(EINA_TRUE, ewk_view_script_execute(GetEwkWebView(), "new Notification('Notification title');", NULL, NULL));
  ASSERT_EQ(Success, EventLoopStart());
  // once granted it should not request permission anymore
  ASSERT_EQ(1, notification_permission_request_cnt);

  ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(), notification_sample_1.c_str()));
  ASSERT_EQ(Success, EventLoopStart());
  // once granted it should not request permission anymore
  ASSERT_EQ(1, notification_permission_request_cnt);
}

/**
 * @brief Tests whether function works properly in case of nullptr value pass.
 */
TEST_F(utc_blink_ewk_notification_permission_reply, NEG_TEST)
{
  /* TODO: this code should use ewk_notification_cached_permissions_set and check its behaviour.
  Results should be reported using one of utc_ macros */
  ewk_notification_permission_reply(nullptr, EINA_TRUE);

  // If  nullptr argument passing wont give segmentation fault negative test case will pass
  SUCCEED();
}
