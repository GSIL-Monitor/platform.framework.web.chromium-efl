// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

class utc_blink_ewk_policy_decision_response_status_code_get : public utc_blink_ewk_base {
protected:
  utc_blink_ewk_policy_decision_response_status_code_get()
    : status_code(-1)
  {
  }

  void LoadFinished(Evas_Object* webview) override {
    EventLoopStop(utc_blink_ewk_base::Failure);
  }

  static void policy_response_decide(utc_blink_ewk_policy_decision_response_status_code_get* owner, Evas_Object* webview, Ewk_Policy_Decision* policy_decision)
  {
    ASSERT_TRUE(owner);

    if (policy_decision && webview) {
      const char* webview_url = ewk_view_url_get(webview);
      const char* policy_url = ewk_policy_decision_url_get(policy_decision);

      if (webview_url && policy_url && strcmp(policy_url, webview_url) == 0) {
        owner->status_code = ewk_policy_decision_response_status_code_get(policy_decision);
        owner->EventLoopStop(Success);
      }
    }
  }

protected:
  int status_code;
};

/**
 * @brief Tests if response status code for policy decision is returned properly.
 */
TEST_F(utc_blink_ewk_policy_decision_response_status_code_get, response_status_code)
{
  {
    // policy decision callback will be send for each element on the page, we're interested only in the first
    // one as it is reponse for request url. Other responses may break TC logic
    evas_object_smart_callback_auto cb(GetEwkWebView(), "policy,response,decide", ToSmartCallback(policy_response_decide), this);
    ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(), "https://www.enlightenment.org/")); // this url should not redirect
    ASSERT_EQ(Success, EventLoopStart()) << "policy,response,decide callback not triggered with proper url";
  }

  ASSERT_EQ(200, status_code);
  // Wait for load,finished
  ASSERT_EQ(Failure, EventLoopStart());

  {
    // policy decision callback will be send for each element on the page, we're interested only in the first
    // one as it is reponse for request url. Other responses may break TC logic
    evas_object_smart_callback_auto cb(GetEwkWebView(), "policy,response,decide", ToSmartCallback(policy_response_decide), this);
    // Load page that should produce 404 error
    ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(), "http://www.google.com/random_page_that_most_likely_does_not_exist"));
    ASSERT_EQ(Success, EventLoopStart()) << "policy,response,decide callback not triggered with proper url";
  }

  ASSERT_EQ(404, status_code);
  // Wait for load,finished
  ASSERT_EQ(Failure, EventLoopStart());
}

/**
 * @brief Tests if default value is returned for policy decision with navigation type
 */
TEST_F(utc_blink_ewk_policy_decision_response_status_code_get, request_status_code)
{
  evas_object_smart_callback_auto cb(GetEwkWebView(), "policy,navigation,decide", ToSmartCallback(policy_response_decide), this);
  ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(), "http://www.google.com"));
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_EQ(0, status_code);
}

/**
 * @brief Tests if function works properly in case of NULL of a policy decision
 */
TEST_F(utc_blink_ewk_policy_decision_response_status_code_get, invalid_args)
{
  ASSERT_EQ(0, ewk_policy_decision_response_status_code_get(NULL));
}
