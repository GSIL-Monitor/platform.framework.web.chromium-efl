// Copyright 2014-2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

class utc_blink_ewk_policy_decision_scheme_get : public utc_blink_ewk_base {
 protected:
  utc_blink_ewk_policy_decision_scheme_get() : received_scheme_(nullptr) {}

  void PostSetUp() override {
    evas_object_smart_callback_add(GetEwkWebView(), "policy,navigation,decide",
                                   policy_navigation_decide, this);
  }

  void PreTearDown() override {
    evas_object_smart_callback_del(GetEwkWebView(), "policy,navigation,decide",
                                   policy_navigation_decide);
  }

  void PreTearDown() {
    evas_object_smart_callback_del(GetEwkWebView(), "policy,navigation,decide",
                                   policy_navigation_decide);
  }

  static void policy_navigation_decide(void* data,
                                       Evas_Object* webview,
                                       void* event_info) {
    utc_message("[policy_navigation_decide] :: \n");
    auto owner = static_cast<utc_blink_ewk_policy_decision_scheme_get*>(data);
    auto policy_decision = static_cast<Ewk_Policy_Decision*>(event_info);
    if (!policy_decision) {
      utc_message("[policy_navigation_decide] :: policy_decision is nullptr");
      owner->EventLoopStop(Failure);
      return;
    }
    owner->received_scheme_ = ewk_policy_decision_scheme_get(policy_decision);
    owner->EventLoopStop(Success);
  }

  static void job_do_http(utc_blink_ewk_base* data) {
    auto owner = static_cast<utc_blink_ewk_policy_decision_scheme_get*>(data);
    ASSERT_EQ(EINA_TRUE, ewk_view_url_set(owner->GetEwkWebView(),
                                          "http://www.google.com"));
  }

  static void job_do_mailto(utc_blink_ewk_base* data) {
    auto owner = static_cast<utc_blink_ewk_policy_decision_scheme_get*>(data);
    ASSERT_EQ(EINA_TRUE, ewk_view_url_set(owner->GetEwkWebView(),
                                          "mailto:someone@example.com"));
  }

  const char* received_scheme_;
};

/**
 * @brief Tests if the scheme for policy decision is returned properly. Expected
 * scheme is http.
 */
TEST_F(utc_blink_ewk_policy_decision_scheme_get, POS_HTTP) {
  SetTestJob(utc_blink_ewk_policy_decision_scheme_get::job_do_http);
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_STREQ("http", received_scheme_);
}

/**
 * @brief Tests if the scheme for policy decision is returned properly. Expected
 * scheme is mailto.
 */
TEST_F(utc_blink_ewk_policy_decision_scheme_get, POS_MAILTO) {
  SetTestJob(utc_blink_ewk_policy_decision_scheme_get::job_do_mailto);
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_STREQ("mailto", received_scheme_);
}

/**
 * @brief Tests if function works properly in case of NULL of a webview
 */
TEST_F(utc_blink_ewk_policy_decision_scheme_get, NEG_INVALID_PARAM) {
  ASSERT_EQ(nullptr, ewk_policy_decision_scheme_get(nullptr));
}
