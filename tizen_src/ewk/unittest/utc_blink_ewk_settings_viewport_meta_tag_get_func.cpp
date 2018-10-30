// Copyright 2016-17 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

class utc_blink_ewk_settings_viewport_meta_tag_get_func
    : public utc_blink_ewk_base {
 protected:
  utc_blink_ewk_settings_viewport_meta_tag_get_func() : settings_(nullptr) {}

  void PostSetUp() override {
    settings_ = ewk_view_settings_get(GetEwkWebView());
    ASSERT_TRUE(settings_);
  }

  Ewk_Settings* settings_;
};

/**
 * @brief Tests whether the function works properly for case 2nd param is TRUE.
 */
TEST_F(utc_blink_ewk_settings_viewport_meta_tag_get_func, POS_ENABLE_TRUE) {
  ASSERT_TRUE(ewk_settings_viewport_meta_tag_set(settings_, EINA_TRUE));
  EXPECT_TRUE(ewk_settings_viewport_meta_tag_get(settings_));
}

/**
 * @brief Tests whether the function works properly for case 2nd param is FALSE.
 */
TEST_F(utc_blink_ewk_settings_viewport_meta_tag_get_func, POS_ENABLE_FALSE) {
  ASSERT_TRUE(ewk_settings_viewport_meta_tag_set(settings_, EINA_FALSE));
  EXPECT_FALSE(ewk_settings_viewport_meta_tag_get(settings_));
}

/**
 * @brief Tests whether the function works properly
 *        for case Ewk_Settings object is NULL.
 */
TEST_F(utc_blink_ewk_settings_viewport_meta_tag_get_func,
       NEG_INVALID_SETTINGS_PARAM) {
  EXPECT_FALSE(ewk_settings_viewport_meta_tag_get(nullptr));
}