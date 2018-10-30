// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

static const char* const kUrl = "https://review.tizen.org/gerrit/#/";

class utc_blink_ewk_auth_challenge_realm_get : public utc_blink_ewk_base
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

    utc_blink_ewk_auth_challenge_realm_get *owner = static_cast<utc_blink_ewk_auth_challenge_realm_get*>(data);

    if(auth_challenge)
    {
      std::string realm = ewk_auth_challenge_realm_get(auth_challenge);
      if (realm == "Tizen.org ACCOUNT") {
        owner->EventLoopStop(utc_blink_ewk_base::Success);
        return;
      }
    }
    owner->EventLoopStop(utc_blink_ewk_base::Failure);
  }
};

/**
 * @brief Checking whether the realm of authentication challenge is returned properly.
 */
TEST_F(utc_blink_ewk_auth_challenge_realm_get, POS_TEST)
{
  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), kUrl));
  EXPECT_EQ(Success, EventLoopStart());
}

/**
 * @brief Checking whether function works properly in case of NULL of a webview.
 */
TEST_F(utc_blink_ewk_auth_challenge_realm_get, NEG_TEST)
{
  ASSERT_FALSE(ewk_view_url_set(NULL, kUrl));
}
