// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

static const char* const kUrl = "https://review.tizen.org/gerrit/#/";

class utc_blink_ewk_auth_challenge_suspend : public utc_blink_ewk_base
{
protected:

  void PostSetUp() override
  {
    ewk_view_authentication_callback_set(GetEwkWebView(),
                                         AuthenticationChallenge,
                                         this);
  }

  void PreTearDown() override
  {
    ewk_view_authentication_callback_set(GetEwkWebView(),
                                         nullptr,
                                         nullptr);
  }

  void LoadFinished(Evas_Object* webview) override {
    EventLoopStop( utc_blink_ewk_base::Failure ); // will noop if EventLoopStop was alraedy called
  }

  static void AuthenticationChallenge(Evas_Object* o,
                                      Ewk_Auth_Challenge* auth_challenge,
                                      void* data)
  {
    ASSERT_TRUE(data != NULL);

    utc_blink_ewk_auth_challenge_suspend *owner = static_cast<utc_blink_ewk_auth_challenge_suspend*>(data);

    Ewk_Auth_Challenge* auth_challenge = static_cast<Ewk_Auth_Challenge*>(event_info);

    if(auth_challenge)
    {
      ewk_auth_challenge_suspend(auth_challenge);

      owner->EventLoopStop(utc_blink_ewk_base::Success);
      return;
    }

    owner->EventLoopStop(utc_blink_ewk_base::Failure);
  }
};

/**
 * @brief Checking whether the suspend of authentication challenge works properly.
 */
TEST_F(utc_blink_ewk_auth_challenge_suspend, POS_TEST)
{
  ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(), kUrl));
  EXPECT_EQ(Success, EventLoopStart());
}

/**
 * @brief Checking whether function works properly in case of NULL of a webview.
 */
TEST_F(utc_blink_ewk_auth_challenge_suspend, NEG_TEST)
{
  ASSERT_EQ(EINA_FALSE, ewk_view_url_set(NULL, kUrl));
}
