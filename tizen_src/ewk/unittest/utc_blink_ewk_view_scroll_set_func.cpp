// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

const int kScrollX = 2;
const int kScrollY = 3;

class utc_blink_ewk_view_scroll_set : public utc_blink_ewk_base {
 private:
  void LoadFinished(Evas_Object* o) override { EventLoopStop(Success); }
};

/**
 * @brief Check if setting positive scroll values works
 */
TEST_F(utc_blink_ewk_view_scroll_set, POS_SCROLL_POSITIVE) {
  ASSERT_EQ(EINA_TRUE,
            ewk_view_url_set(GetEwkWebView(), GetResourceUrl(kUrl).c_str()));
  ASSERT_EQ(Success, EventLoopStart());

  Eina_Bool result = ewk_view_scroll_set(GetEwkWebView(), kScrollX, kScrollY);
  ASSERT_EQ(EINA_TRUE, result);
}

/**
 * @brief Check if positive scroll values set works with no context
 */
TEST_F(utc_blink_ewk_view_scroll_set, POS_SCROLL_POSITIVE_NO_CONTEXT) {
  Eina_Bool result = ewk_view_scroll_set(GetEwkWebView(), kScrollX, kScrollY);
  ASSERT_EQ(EINA_TRUE, result);
}

/**
 * @brief Check if positive scroll values over max scroll positions are not set
 */
TEST_F(utc_blink_ewk_view_scroll_set, POS_SCROLL_POSITIVE_OVERSCROLL) {
  ASSERT_EQ(EINA_TRUE,
            ewk_view_url_set(GetEwkWebView(), GetResourceUrl(kUrl).c_str()));
  ASSERT_EQ(Success, EventLoopStart());

  Eina_Bool result = ewk_view_scroll_set(GetEwkWebView(), 9999, 9999);
  ASSERT_EQ(EINA_TRUE, result);
  int x = 0, y = 0;
  result = ewk_view_scroll_pos_get(GetEwkWebView(), &x, &y);
  ASSERT_EQ(EINA_TRUE, result);
  EXPECT_LT(0, x);
  EXPECT_LT(0, y);
}

/**
 * @brief Check if negative scroll values are not set
 */
TEST_F(utc_blink_ewk_view_scroll_set, POS_SCROLL_NEGATIVE) {
  ASSERT_EQ(EINA_TRUE,
            ewk_view_url_set(GetEwkWebView(), GetResourceUrl(kUrl).c_str()));
  ASSERT_EQ(Success, EventLoopStart());

  Eina_Bool result = ewk_view_scroll_set(GetEwkWebView(), -2, -3);
  ASSERT_EQ(EINA_TRUE, result);
  int x = 0, y = 0;
  result = ewk_view_scroll_pos_get(GetEwkWebView(), &x, &y);
  ASSERT_EQ(EINA_TRUE, result);
  EXPECT_EQ(0, x);
  EXPECT_EQ(0, y);
}

/**
 * @brief Check if api returns EINA_FALSE for null webview
 */
TEST_F(utc_blink_ewk_view_scroll_set, NEG_INVALID_WEBVIEW) {
  ASSERT_EQ(EINA_FALSE, ewk_view_scroll_set(NULL, kScrollX, kScrollY));
}
