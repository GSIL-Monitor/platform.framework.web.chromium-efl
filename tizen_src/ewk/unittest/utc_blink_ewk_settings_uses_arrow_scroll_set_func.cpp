// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

class utc_blink_ewk_settings_uses_arrow_scroll_set : public utc_blink_ewk_base {
 protected:
  void PostSetUp() override {
    settings_ = ewk_view_settings_get(GetEwkWebView());
    ASSERT_TRUE(settings_);
  }

  Ewk_Settings* settings_;
};

/**
 * @brief Tests if sets correctly, when called with correct WebSettings object
 */
TEST_F(utc_blink_ewk_settings_uses_arrow_scroll_set, POS_TEST_EINA_TRUE) {
  ASSERT_EQ(ewk_settings_uses_arrow_scroll_set(settings_, EINA_TRUE),
            EINA_TRUE);
  EXPECT_EQ(ewk_settings_uses_arrow_scroll_get(settings_), EINA_TRUE);
}

/**
 * @brief Tests if sets correctly, when called with correct WebSettings object
 */
TEST_F(utc_blink_ewk_settings_uses_arrow_scroll_set, POS_TEST_EINA_FALSE) {
  ASSERT_EQ(ewk_settings_uses_arrow_scroll_set(settings_, EINA_FALSE),
            EINA_TRUE);
  EXPECT_EQ(ewk_settings_uses_arrow_scroll_get(settings_), EINA_FALSE);
}

/**
 * @brief Tests if returns NULL when called with NULL WebSettings object
 */
TEST_F(utc_blink_ewk_settings_uses_arrow_scroll_set, NEG_TEST) {
  EXPECT_NE(ewk_settings_uses_arrow_scroll_set(NULL, EINA_FALSE), EINA_TRUE);
}
