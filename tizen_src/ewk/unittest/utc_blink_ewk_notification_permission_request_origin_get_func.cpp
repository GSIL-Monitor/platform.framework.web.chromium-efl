// Copyright 2014-2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_notification_test_base.h"

class utc_blink_ewk_notification_permission_request_origin_get : public utc_blink_ewk_notification_test_base {
 protected:
  utc_blink_ewk_notification_permission_request_origin_get()
      : expected_origin_protocol(""),
        expected_origin_host(""),
        expected_origin_port(0),
        received_origin_protocol(NULL),
        received_origin_host(NULL),
        received_origin_port(1) {}

  ~utc_blink_ewk_notification_permission_request_origin_get() override {
    eina_stringshare_del(received_origin_protocol);
    eina_stringshare_del(received_origin_host);
  }

  /* Callback for "notification,permission,request" */
  Eina_Bool NotificationPermissionRequest(
      Evas_Object* webview,
      Ewk_Notification_Permission_Request* request) override {
    const Ewk_Security_Origin* origin = ewk_notification_permission_request_origin_get(request);

    EXPECT_FALSE(received_origin_protocol);
    EXPECT_FALSE(received_origin_host);
    EXPECT_NE(0, received_origin_port);
    if (received_origin_protocol || received_origin_host ||
        0 == received_origin_port) {
      EventLoopStop(Failure);
      return EINA_FALSE;
    }

    if (origin) {
      received_origin_host = eina_stringshare_add(ewk_security_origin_host_get(origin));
      received_origin_protocol = eina_stringshare_add(ewk_security_origin_protocol_get(origin));
      received_origin_port = ewk_security_origin_port_get(origin);
    }

    //allow the notification
    ewk_notification_permission_reply(request, EINA_TRUE);
    EventLoopStop(Success);
    return EINA_TRUE;
  }

 protected:
  const char* const expected_origin_protocol;
  const char* const expected_origin_host;
  const uint16_t expected_origin_port;

  Eina_Stringshare* received_origin_protocol;
  Eina_Stringshare* received_origin_host;
  uint16_t received_origin_port;
};

/**
 * @brief Positive test case for ewk_notification_permission_reply function
 */
TEST_F(utc_blink_ewk_notification_permission_request_origin_get, POS_TEST)
{
  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), notification_sample_1.c_str()));
  ASSERT_EQ(Success, EventLoopStart());

  EXPECT_TRUE(received_origin_protocol);
  EXPECT_TRUE(received_origin_host);
  EXPECT_EQ(expected_origin_port, received_origin_port);
  EXPECT_STREQ(expected_origin_host, received_origin_host);
  EXPECT_STREQ(expected_origin_protocol, received_origin_protocol);
}

/**
 * @brief Tests whether function works properly in case of NULL value pass.
 */
TEST_F(utc_blink_ewk_notification_permission_request_origin_get, NEG_TEST)
{
  ASSERT_FALSE(ewk_notification_permission_request_origin_get(NULL));
}
