// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

class utc_blink_ewk_settings_dom_paste_allowed_set_func
    : public utc_blink_ewk_base {
 protected:
  utc_blink_ewk_settings_dom_paste_allowed_set_func()
      : settings_(nullptr) {
  }

  void PostSetUp() override {
    settings_ = ewk_view_settings_get(GetEwkWebView());
    ASSERT_TRUE(settings_ != nullptr);
  }

  Ewk_Settings* settings_;
};

/**
 * @brief Tests whether the function works properly for case 2nd param is TRUE.
 */
TEST_F(utc_blink_ewk_settings_dom_paste_allowed_set_func, POS_ENABLE_TRUE) {
  EXPECT_TRUE(ewk_settings_dom_paste_allowed_set(settings_, EINA_TRUE));
}

/**
 * @brief Tests whether the function works properly for case 2nd param is FALSE.
 */
TEST_F(utc_blink_ewk_settings_dom_paste_allowed_set_func, POS_ENABLE_FALSE) {
  EXPECT_TRUE(ewk_settings_dom_paste_allowed_set(settings_, EINA_FALSE));
}

/**
 * @brief Tests whether the function works properly
 *        for case Ewk_Settings object is NULL.
 */
TEST_F(utc_blink_ewk_settings_dom_paste_allowed_set_func,
       NEG_INVALID_SETTINGS_PARAM) {
  EXPECT_FALSE(ewk_settings_dom_paste_allowed_set(nullptr, EINA_TRUE));
}
