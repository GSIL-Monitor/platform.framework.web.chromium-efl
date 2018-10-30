// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

class utc_blink_ewk_settings_uses_scrollbar_thumb_focus_notifications_set
    : public utc_blink_ewk_base {
 protected:
  utc_blink_ewk_settings_uses_scrollbar_thumb_focus_notifications_set()
      : settings_(nullptr) {}

  void PostSetUp() override {
    settings_ = ewk_view_settings_get(GetEwkWebView());
    ASSERT_TRUE(settings_);
  }

  Ewk_Settings* settings_;
};

/**
 * @brief Tests whether the function works properly for case 2nd param is
 * EINA_TRUE
 */
TEST_F(utc_blink_ewk_settings_uses_scrollbar_thumb_focus_notifications_set,
       POS_USES_SCROLLBAR_THUMB_FOCUS_NOTIFICATION_TRUE) {
  EXPECT_TRUE(ewk_settings_uses_scrollbar_thumb_focus_notifications_set(
      settings_, EINA_TRUE));
}

/**
 * @brief Tests whether the function works properly for case 2nd param is
 * EINA_FALSE
 */
TEST_F(utc_blink_ewk_settings_uses_scrollbar_thumb_focus_notifications_set,
       POS_USES_SCROLLBAR_THUMB_FOCUS_NOTIFICATION_FALSE) {
  EXPECT_TRUE(ewk_settings_uses_scrollbar_thumb_focus_notifications_set(
      settings_, EINA_FALSE));
}

/**
 * @brief Tests whether the function works properly
 *        for case Ewk_Settings object is NULL.
 */
TEST_F(utc_blink_ewk_settings_uses_scrollbar_thumb_focus_notifications_set,
       NEG_TEST) {
  EXPECT_FALSE(ewk_settings_uses_scrollbar_thumb_focus_notifications_set(
      nullptr, EINA_TRUE));
}
