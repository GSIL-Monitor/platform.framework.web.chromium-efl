// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"
#define URL "https://review.tizen.org/gerrit/#/"

static Eina_Bool is_failed;
static Eina_Bool is_Authenticated;

class utc_blink_ewk_auth_challenge_credential_cancel : public utc_blink_ewk_base {
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
      EventLoopStop(utc_blink_ewk_base::Failure);  // will noop if EventLoopStop
                                                   // was alraedy called
    }

    static void AuthenticationChallenge(Evas_Object* o,
                                        Ewk_Auth_Challenge* auth_challenge,
                                        void* data)
    {
        utc_blink_ewk_auth_challenge_credential_cancel *owner = static_cast<utc_blink_ewk_auth_challenge_credential_cancel*>(data);
        utc_message("[authentication_challenge] :: ");

        if (!auth_challenge) {
            owner->EventLoopStop(utc_blink_ewk_base::Failure);
            return;
        }

        const char* realm = ewk_auth_challenge_realm_get(auth_challenge);
        const char* url = ewk_auth_challenge_url_get(auth_challenge);

        if (!realm || !url) {
            owner->EventLoopStop(utc_blink_ewk_base::Failure);
            return;
        }

        // TODO: invalid test
        // this test does not check if cancel was successful. It checks if authentication,challange was called
        ewk_auth_challenge_credential_cancel(auth_challenge);
        owner->EventLoopStop(utc_blink_ewk_base::Success);
    }
};

/**
  * @brief Checking whether sending cancellation notification for authentication challenge works properly.
  */
TEST_F(utc_blink_ewk_auth_challenge_credential_cancel, POS_TEST)
{
    is_failed = EINA_FALSE;
    is_Authenticated = EINA_FALSE;

    Eina_Bool result = ewk_view_url_set(GetEwkWebView(), URL);
    if (!result)
        FAIL();

    utc_blink_ewk_base::MainLoopResult main_result = EventLoopStart();

    if (main_result != utc_blink_ewk_base::Success)
        FAIL();

    evas_object_show(GetEwkWebView());
    evas_object_show(GetEwkWindow());
}

// TODO: this case does nothing. It didn't even start event loop, so callback wouldn't be ever called.
/**
* @brief Checking whether function works properly in case of NULL of a webview.
*/
/*
TEST_F(utc_blink_ewk_auth_challenge_credential_cancel, NEG_TEST)
{
    Eina_Bool result = ewk_view_url_set(NULL, URL);
    if (result)
        FAIL();

    utc_blink_ewk_base::MainLoopResult main_result = EventLoopStart(15.0);
    evas_object_show(GetEwkWebView());
    evas_object_show(GetEwkWindow());



    EXPECT_NE(main_result, utc_blink_ewk_base::Success);
}
*/

