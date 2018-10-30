// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

class utc_blink_ewk_context_check_accessible_path_callback_set
    : public utc_blink_ewk_base {
 public:
  void LoadFinished(Evas_Object* webview) override {
    EventLoopStop(utc_blink_ewk_base::Success);
  }

 protected:
  static Eina_Bool allowAccessCallback(const char* tizen_app_id,
                                       const char* url,
                                       void* user_data) {
    return EINA_TRUE;
  }

  static Eina_Bool disallowAccessCallback(const char* tizen_app_id,
                                          const char* url,
                                          void* user_data) {
    return EINA_FALSE;
  }

  static const char* test_url_;
  static const char* test_app_id_;
};

const char*
    utc_blink_ewk_context_check_accessible_path_callback_set::test_url_ =
        "common/sample.html";
const char*
    utc_blink_ewk_context_check_accessible_path_callback_set::test_app_id_ =
        "test_app";

/**
 * @brief Checks whether interface works properly in case of NULL of context
 */
TEST_F(utc_blink_ewk_context_check_accessible_path_callback_set,
       NEG_CONTEXT_NULL) {
  Eina_Bool result = ewk_context_check_accessible_path_callback_set(
      NULL, allowAccessCallback, this);

  EXPECT_EQ(EINA_FALSE, result);
}

/**
 * @brief Checks whether interface works properly in case of NULL of callback
 */
TEST_F(utc_blink_ewk_context_check_accessible_path_callback_set,
       NEG_CALLBACK_NULL) {
  Eina_Bool result = ewk_context_check_accessible_path_callback_set(
      ewk_context_default_get(), NULL, this);

  EXPECT_EQ(EINA_FALSE, result);
}

/**
 * @brief Checks if load fails with disallowing callback.
 */
TEST_F(utc_blink_ewk_context_check_accessible_path_callback_set,
       POS_DISALLOW_TEST) {
  ewk_context_tizen_app_id_set(ewk_context_default_get(), test_app_id_);
  Eina_Bool result = ewk_context_check_accessible_path_callback_set(
      ewk_context_default_get(), disallowAccessCallback, this);

  ASSERT_EQ(EINA_TRUE, result);

  std::string resource_url = GetResourceUrl(test_url_);
  utc_message("Loading file: %s", resource_url.c_str());

  result = ewk_view_url_set(GetEwkWebView(), resource_url.c_str());

  ASSERT_EQ(EINA_TRUE, result);

  EXPECT_EQ(LoadFailure, EventLoopStart());
}

/**
 * @brief Tests ewk_context_check_accessible_path_callback_set with
 *        ewk_view_urls_set local file, in this case the callback
 *        allow the file loading
 */
TEST_F(utc_blink_ewk_context_check_accessible_path_callback_set,
       POS_ALLOW_TEST) {
  ewk_context_tizen_app_id_set(ewk_context_default_get(), test_app_id_);
  Eina_Bool result = ewk_context_check_accessible_path_callback_set(
      ewk_context_default_get(), allowAccessCallback, this);

  ASSERT_EQ(EINA_TRUE, result);

  std::string resource_url = GetResourceUrl(test_url_);
  utc_message("Loading file: %s", resource_url.c_str());

  result = ewk_view_url_set(GetEwkWebView(), resource_url.c_str());

  ASSERT_EQ(EINA_TRUE, result);

  EXPECT_EQ(Success, EventLoopStart());
}
