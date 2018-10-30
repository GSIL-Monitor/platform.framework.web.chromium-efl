// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_notification_test_base.h"

class utc_blink_ewk_view_notification_permission_callback_set : public utc_blink_ewk_notification_test_base
{
 protected:
  Eina_Bool NotificationPermissionRequest(
      Evas_Object* webview,
      Ewk_Notification_Permission_Request* request) override {
    EventLoopStop(Success);
    return EINA_TRUE;
  }
};

TEST_F(utc_blink_ewk_view_notification_permission_callback_set, POS_TEST)
{
  ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(), notification_sample_1.c_str()));
  ASSERT_EQ(Success, EventLoopStart());

  ewk_view_notification_permission_callback_set(GetEwkWebView(), NULL, NULL);
  ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(), notification_sample_1.c_str()));
  ASSERT_EQ(Timeout, EventLoopStart(5.0));
}

TEST_F(utc_blink_ewk_view_notification_permission_callback_set, NEG_TEST)
{
  ewk_view_notification_permission_callback_set(NULL, NULL, NULL);
}
