// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

class utc_blink_ewk_policy_decision_response_headers_get : public utc_blink_ewk_base
{
protected:
 void PostSetUp() override {
   evas_object_smart_callback_add(GetEwkWebView(), "policy,response,decide",
                                  policy_response_decide, this);
  }

  void PreTearDown() override {
    evas_object_smart_callback_del(GetEwkWebView(), "policy,response,decide", policy_response_decide);
  }

  void LoadFinished(Evas_Object* webview) override {
    EventLoopStop(utc_blink_ewk_base::Failure);
  }

  static void policy_response_decide(void* data, Evas_Object* webview, void* event_info)
  {
    ASSERT_TRUE(data != NULL);
    ASSERT_TRUE(event_info != NULL);
    utc_message("[policy_response_decide] ::");
    utc_blink_ewk_policy_decision_response_headers_get* const owner = static_cast<utc_blink_ewk_policy_decision_response_headers_get*>(data);
    Ewk_Policy_Decision* const policy_decision = static_cast<Ewk_Policy_Decision*>(event_info);
    const Eina_Hash* const headers = ewk_policy_decision_response_headers_get(policy_decision);
    owner->EventLoopStop((NULL != headers) ? Success : Failure);
  }
};

/**
 * @brief Tests if the headers for policy decision is returned properly.
 */
TEST_F(utc_blink_ewk_policy_decision_response_headers_get, POS_TEST)
{
  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), "http://www.samsung.com"));

  EXPECT_EQ(Success, EventLoopStart());
}

/**
 * @brief Tests if function works properly in case of NULL of a webview
 */
TEST_F(utc_blink_ewk_policy_decision_response_headers_get, NEG_TEST)
{
  EXPECT_EQ(0, ewk_policy_decision_response_headers_get(0));
}
