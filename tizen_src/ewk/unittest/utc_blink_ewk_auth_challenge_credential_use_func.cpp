// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

#define URL "https://review.tizen.org/gerrit/#/"

class utc_blink_ewk_auth_challenge_credential_use : public utc_blink_ewk_base
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
    utc_message("[authentication_challenge] :: ");
    utc_blink_ewk_auth_challenge_credential_use *owner = static_cast<utc_blink_ewk_auth_challenge_credential_use*>(data);

    if (!auth_challenge) {
      owner->EventLoopStop( utc_blink_ewk_base::Failure ); // will noop if EventLoopStop was alraedy called
      utc_fail();
    }

    const char* realm = ewk_auth_challenge_realm_get(auth_challenge);
    const char* url = ewk_auth_challenge_url_get(auth_challenge);
    if (!realm || !url) {
      owner->EventLoopStop( utc_blink_ewk_base::Failure ); // will noop if EventLoopStop was alraedy called
      utc_fail();
    }

    const char* user = "access";
    const char* password = "early";
    ewk_auth_challenge_credential_use(auth_challenge, user, password);
    owner->EventLoopStop( utc_blink_ewk_base::Success );
    // TODO
    // Here is a logical error because success is called a question about authorization,
    // not about the result of the authorization.
    // In the casete of re-call up the callback should be return failuare
  }
};

  /**
  * @brief Checking whether sending cancellation notification for authentication challenge works properly.
  */
TEST_F(utc_blink_ewk_auth_challenge_credential_use, POS_TEST)
{
  utc_check_true( ewk_view_url_set( GetEwkWebView(), URL));

  utc_check_eq( EventLoopStart(), utc_blink_ewk_base::Success);

  evas_object_show( GetEwkWebView());
  evas_object_show( GetEwkWindow());
}

  /**
  * @brief Checking whether function works properly in case of NULL of a webview.
  */
TEST_F(utc_blink_ewk_auth_challenge_credential_use, NEG_TEST)
{
  Eina_Bool result = ewk_view_url_set( NULL, URL);

  if (result) {
    utc_fail();
  }

  evas_object_show( GetEwkWebView());
  evas_object_show( GetEwkWindow());
}

