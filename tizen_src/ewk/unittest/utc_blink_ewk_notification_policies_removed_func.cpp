// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_notification_test_base.h"

class utc_blink_ewk_notification_policies_removed : public utc_blink_ewk_notification_test_base {
 protected:
  utc_blink_ewk_notification_policies_removed()
    : permission_request_call_cnt(0)
    , proto_(NULL)
    , host_(NULL)
    , expected_proto_("")
    , expected_host_("")
  {}

  ~utc_blink_ewk_notification_policies_removed() override {
    eina_stringshare_del(proto_);
    eina_stringshare_del(host_);
  }

  /* Callback for notification permission request */
  Eina_Bool NotificationPermissionRequest(
      Evas_Object* obj,
      Ewk_Notification_Permission_Request* request) override {
    const Ewk_Security_Origin* origin = ewk_notification_permission_request_origin_get(request);

    if (!origin) {
      EventLoopStop(Failure);
      EXPECT_TRUE(origin);
      return EINA_FALSE;
    }

    ++permission_request_call_cnt;
    proto_ = eina_stringshare_add(ewk_security_origin_protocol_get(origin));
    host_ = eina_stringshare_add(ewk_security_origin_host_get(origin));
    ewk_notification_permission_reply(request, EINA_TRUE);
    return EINA_TRUE;
  }

  void NotificationShow(Ewk_Notification*) override { EventLoopStop(Success); }

 protected:
  int permission_request_call_cnt;
  Eina_Stringshare* proto_;
  Eina_Stringshare* host_;
  const char* const expected_proto_;
  const char* const expected_host_;
};

TEST_F(utc_blink_ewk_notification_policies_removed, POS_TEST)
{
  ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(), notification_sample_1.c_str()));
  ASSERT_EQ(Success, EventLoopStart());

  ASSERT_EQ(1, permission_request_call_cnt);
  ASSERT_STREQ(expected_proto_, proto_);
  ASSERT_STREQ(expected_host_, host_);

  ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(), notification_sample_1.c_str()));
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_EQ(1, permission_request_call_cnt);


  Eina_List *list = NULL;
  Ewk_Security_Origin *origin = ewk_security_origin_new_from_string((std::string(proto_) + "://" + std::string(host_)).c_str());
  list = eina_list_append(list, origin);

  ASSERT_EQ(EINA_TRUE, ewk_notification_policies_removed(list));

  void* data = NULL;
  EINA_LIST_FREE(list, data) {
    origin = static_cast<Ewk_Security_Origin*>(data);
    ewk_security_origin_free(origin);
  }

  ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(), notification_sample_1.c_str()));
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_EQ(2, permission_request_call_cnt);
}

TEST_F(utc_blink_ewk_notification_policies_removed, NEG_TEST)
{
  ewk_notification_policies_removed(NULL);
  SUCCEED();
}
