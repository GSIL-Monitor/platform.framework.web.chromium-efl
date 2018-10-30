// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

class utc_blink_ewk_policy_decision_suspend : public utc_blink_ewk_base {
protected:
  utc_blink_ewk_policy_decision_suspend()
    : policy_decision(NULL)
  {
  }

  void LoadFinished(Evas_Object* webview) override {
    EventLoopStop(utc_blink_ewk_base::Failure); // will noop if EventLoopStop was already called
  }

  static void policy_decision_suspend(utc_blink_ewk_policy_decision_suspend* owner, Evas_Object* webview, Ewk_Policy_Decision* policy_decision)
  {
    utc_message("[policy_decision_suspend] :: \n");
    ASSERT_TRUE(owner);
    EXPECT_TRUE(policy_decision);

    if (!owner->policy_decision) {
      // We need to handle suspend here, as if suspend fails then policy_decision is treated as not decided and deleted
      if (EINA_TRUE == ewk_policy_decision_suspend(policy_decision)) {
        owner->policy_decision = policy_decision;
      }

      owner->EventLoopStop(utc_blink_ewk_base::Success);
    }
  }

  static void fail_event_loop(utc_blink_ewk_policy_decision_suspend* owner, Evas_Object* webview, void*)
  {
    ASSERT_TRUE(owner);
    owner->policy_decision = NULL;
    owner->EventLoopStop(Failure);
  }

protected:
  Ewk_Policy_Decision *policy_decision;
};

/**
 * @brief Tests if suspend operation for policy decision is set properly
 */
TEST_F(utc_blink_ewk_policy_decision_suspend, SuspendNavigation)
{
  evas_object_smart_callback_auto sc(GetEwkWebView(), "policy,navigation,decide", ToSmartCallback(policy_decision_suspend), this);

  ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(), "http://www.google.com"));
  // Wait for policy decision smart callback
  ASSERT_EQ(Success, EventLoopStart());

  ASSERT_FALSE(policy_decision);
  // TODO: enable path below if we manage to support suspend for navigation
  /*
  ASSERT_TRUE(policy_decision) << "suspend failed - suspend for navigation may be not supported";

  // we've suspended navigation - try waiting for load finished. It should timeout
  ASSERT_EQ(Timeout, EventLoopStart(5.0));

  // now resume it
  ASSERT_EQ(EINA_TRUE, ewk_policy_decision_use(policy_decision));

  // and wait till request finished - LoadFinished sets error to Failure
  ASSERT_EQ(Failure, EventLoopStart());
  */
}

TEST_F(utc_blink_ewk_policy_decision_suspend, SuspendNewWindow)
{
  evas_object_smart_callback_auto sc(GetEwkWebView(), "policy,newwindow,decide", ToSmartCallback(policy_decision_suspend), this);
  evas_object_smart_callback_auto popup_blocked(GetEwkWebView(), "popup,blocked", ToSmartCallback(fail_event_loop), this);
  evas_object_smart_callback_auto create_window(GetEwkWebView(), "create,window", ToSmartCallback(fail_event_loop), this);

  ASSERT_EQ(EINA_TRUE, ewk_view_html_string_load(GetEwkWebView(), "<html><body>Page</body></html>", NULL, NULL));

  // Wait for page to load
  ASSERT_EQ(Failure, EventLoopStart());

  // execute window.open
  ewk_view_script_execute(GetEwkWebView(), "window.open('www.google.com');", NULL, NULL);
  // Wait for policy,newwindow,decide callback
  ASSERT_EQ(Success, EventLoopStart());

  ASSERT_TRUE(policy_decision) << "suspend failed - suspend for newwindow may be not supported";

  // we've suspended decision - we should not get create,window or popup,blocked smart callback
  ASSERT_EQ(Timeout, EventLoopStart(5.0));

  // Allow new window
  ASSERT_EQ(EINA_TRUE, ewk_policy_decision_use(policy_decision));

  // window,create will NULLify policy_decision to was called - it will be called synchronously from ewk_policy_decision_use
  ASSERT_FALSE(policy_decision);
}

TEST_F(utc_blink_ewk_policy_decision_suspend, SuspendResponse)
{
  evas_object_smart_callback_auto sc(GetEwkWebView(), "policy,response,decide", ToSmartCallback(policy_decision_suspend), this);

  ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(), "http://www.google.com"));
  // Wait for policy decision smart callback
  ASSERT_EQ(Success, EventLoopStart());

  ASSERT_TRUE(policy_decision) << "suspend failed - suspend for response may be not supported";

  // we've suspended navigation - try waiting for load finished. It should timeout
  ASSERT_EQ(Timeout, EventLoopStart(5.0));

  // now resume it
  ASSERT_EQ(EINA_TRUE, ewk_policy_decision_use(policy_decision));

  // and wait till request finished - LoadFinished sets error to Failure
  ASSERT_EQ(Failure, EventLoopStart());
}

/**
 * @brief Tests if function works properly in case of NULL of a webview
 */
TEST_F(utc_blink_ewk_policy_decision_suspend, InvalidArgs)
{
  ASSERT_EQ(EINA_FALSE, ewk_policy_decision_suspend(NULL));
}
