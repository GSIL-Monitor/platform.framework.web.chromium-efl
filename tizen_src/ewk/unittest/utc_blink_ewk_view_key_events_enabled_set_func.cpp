// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

class utc_blink_ewk_view_key_events_enabled_set : public utc_blink_ewk_base {};

/**
 * @brief Check if enable keydown & keyup callback is succeeds with
 * correct webview
 */
TEST_F(utc_blink_ewk_view_key_events_enabled_set, POS_KEY_EVENTS_ENABLE) {
  Eina_Bool result =
      ewk_view_key_events_enabled_set(GetEwkWebView(), EINA_TRUE);
  EXPECT_EQ(EINA_TRUE, result);
}

/**
 * @brief Check if disable keydown & keyup callback is succeeds with
 * correct webview
 */
TEST_F(utc_blink_ewk_view_key_events_enabled_set, POS_KEY_EVENTS_DISABLE) {
  Eina_Bool result =
      ewk_view_key_events_enabled_set(GetEwkWebView(), EINA_FALSE);
  EXPECT_EQ(EINA_TRUE, result);
}

/**
 * @brief Check if enable keydown & keyup callback is fails with
 * null webview
 */
TEST_F(utc_blink_ewk_view_key_events_enabled_set, NEG_KEY_EVENTS_ENABLE) {
  Eina_Bool result = ewk_view_key_events_enabled_set(NULL, EINA_TRUE);
  EXPECT_EQ(EINA_FALSE, result);
}

/**
 * @brief Check if disable keydown & keyup callback is fails with
 * null webview
 */
TEST_F(utc_blink_ewk_view_key_events_enabled_set, NEG_KEY_EVENTS_DISABLE) {
  Eina_Bool result = ewk_view_key_events_enabled_set(NULL, EINA_FALSE);
  EXPECT_EQ(EINA_FALSE, result);
}
