// Copyright 2014-2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

class utc_blink_ewk_view_user_agent_set : public utc_blink_ewk_base {
 protected:
  static const char* kUserAgentString;
  static const char* kAboutBlankURL;

  void PostSetUp() override {
    default_user_agent_ =
        eina_stringshare_add(ewk_view_user_agent_get(GetEwkWebView()));
    current_user_agent_ = nullptr;
    ASSERT_TRUE(default_user_agent_);
    ASSERT_STRNE(default_user_agent_, kUserAgentString);
  }

  void PreTearDown() override {
    // reset user agent to default value
    ewk_view_user_agent_set(GetEwkWebView(), "");
    if (default_user_agent_) {
      eina_stringshare_del(default_user_agent_);
      default_user_agent_ = nullptr;
    }
    if (current_user_agent_) {
      eina_stringshare_del(current_user_agent_);
      current_user_agent_ = nullptr;
    }
  }

  void LoadFinished(Evas_Object* webview) override {
    if (!ewk_view_script_execute(GetEwkWebView(), "navigator.userAgent",
                                 scriptExecutionFinished, this)) {
      EventLoopStop(Failure);
    }
  }

  /* Callback for script execution */
  static void scriptExecutionFinished(Evas_Object* webview,
                                      const char* result_value,
                                      void* data) {
    auto owner = static_cast<utc_blink_ewk_view_user_agent_set*>(data);
    owner->current_user_agent_ = eina_stringshare_add(result_value);
    owner->EventLoopStop(Success);
  }

  Eina_Stringshare* current_user_agent_;
  Eina_Stringshare* default_user_agent_;
};

const char* utc_blink_ewk_view_user_agent_set::kUserAgentString =
    "TEST_USER_AGENT";
const char* utc_blink_ewk_view_user_agent_set::kAboutBlankURL = "about:blank";

/**
 * @brief Positive test case of ewk_view_user_agent_set()
 */
TEST_F(utc_blink_ewk_view_user_agent_set, POS_SET_USER_AGENT) {
  ASSERT_TRUE(ewk_view_user_agent_set(GetEwkWebView(), kUserAgentString));
  Eina_Stringshare* user_agent =
      eina_stringshare_add(ewk_view_user_agent_get(GetEwkWebView()));
  ASSERT_TRUE(user_agent);
  ASSERT_STREQ(kUserAgentString, user_agent);

  ewk_view_url_set(GetEwkWebView(), kAboutBlankURL);
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_STREQ(kUserAgentString, current_user_agent_);
  eina_stringshare_del(user_agent);
}

/**
 * @brief Positive test case of ewk_view_user_agent_set() with empty string
 */
TEST_F(utc_blink_ewk_view_user_agent_set, POS_EMPTY_STRING) {
  ASSERT_TRUE(ewk_view_user_agent_set(GetEwkWebView(), kUserAgentString));
  Eina_Stringshare* user_agent =
      eina_stringshare_add(ewk_view_user_agent_get(GetEwkWebView()));
  ASSERT_TRUE(user_agent);
  ASSERT_STREQ(kUserAgentString, user_agent);
  ewk_view_url_set(GetEwkWebView(), kAboutBlankURL);
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_STREQ(kUserAgentString, current_user_agent_);
  eina_stringshare_del(user_agent);

  ASSERT_TRUE(ewk_view_user_agent_set(GetEwkWebView(), ""));

  user_agent = eina_stringshare_add(ewk_view_user_agent_get(GetEwkWebView()));
  ASSERT_TRUE(user_agent);
  ASSERT_STREQ(default_user_agent_, user_agent);
  ewk_view_url_set(GetEwkWebView(), kAboutBlankURL);
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_STREQ(default_user_agent_, current_user_agent_);
  eina_stringshare_del(user_agent);
}

/**
 * @brief Positive test case of ewk_view_user_agent_set() with Null string
 */
TEST_F(utc_blink_ewk_view_user_agent_set, POS_NULL_STRING) {
  ASSERT_TRUE(ewk_view_user_agent_set(GetEwkWebView(), kUserAgentString));
  Eina_Stringshare* user_agent =
      eina_stringshare_add(ewk_view_user_agent_get(GetEwkWebView()));
  ASSERT_TRUE(user_agent);
  ASSERT_STREQ(kUserAgentString, user_agent);
  ewk_view_url_set(GetEwkWebView(), kAboutBlankURL);
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_STREQ(kUserAgentString, current_user_agent_);
  eina_stringshare_del(user_agent);

  ASSERT_TRUE(ewk_view_user_agent_set(GetEwkWebView(), NULL));

  user_agent = eina_stringshare_add(ewk_view_user_agent_get(GetEwkWebView()));
  ASSERT_TRUE(user_agent);
  ASSERT_STREQ(default_user_agent_, user_agent);
  eina_stringshare_del(user_agent);
}

/**
 * @brief Negative test case of ewk_view_user_agent_set()
 */
TEST_F(utc_blink_ewk_view_user_agent_set, NEG_EWK_VIEW_NULL) {
  ASSERT_FALSE(ewk_view_user_agent_set(nullptr, ""));
}
