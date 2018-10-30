// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

const char* const kUrl = "ewk_view/index_big_red_square.html";

const int kScrollX = 2;
const int kScrollY = 3;

class utc_blink_ewk_view_scroll_pos_get : public utc_blink_ewk_base {
 private:
  void LoadFinished(Evas_Object* o) override { EventLoopStop(Success); }
};

/**
 *  * @brief Check if api succeeds with no scroll previously set
 *   */
TEST_F(utc_blink_ewk_view_scroll_pos_get, POS_NO_SET) {
  ASSERT_EQ(EINA_TRUE,
            ewk_view_url_set(GetEwkWebView(), GetResourceUrl(kUrl).c_str()));
  ASSERT_EQ(Success, EventLoopStart());

  int x = 0, y = 0;
  Eina_Bool result = ewk_view_scroll_pos_get(GetEwkWebView(), &x, &y);
  ASSERT_EQ(EINA_TRUE, result);
  EXPECT_EQ(0, x);
  EXPECT_EQ(0, y);
}

/**
 *  * @brief Check if scroll value got equals to positive value set
 *   */
TEST_F(utc_blink_ewk_view_scroll_pos_get, POS) {
  ASSERT_EQ(EINA_TRUE,
            ewk_view_url_set(GetEwkWebView(), GetResourceUrl(kUrl).c_str()));
  ASSERT_EQ(Success, EventLoopStart());

  Eina_Bool result = ewk_view_scroll_set(GetEwkWebView(), kScrollX, kScrollY);
  ASSERT_EQ(EINA_TRUE, result);
  int x = 0, y = 0;
  result = ewk_view_scroll_pos_get(GetEwkWebView(), &x, &y);
  ASSERT_EQ(EINA_TRUE, result);
  EXPECT_EQ(kScrollX, x);
  EXPECT_EQ(kScrollY, y);
}

/**
 *  * @brief Check if scroll value got equals to positive value set called
 *  twice
 *   */
TEST_F(utc_blink_ewk_view_scroll_pos_get, POS_TWICE) {
  ASSERT_EQ(EINA_TRUE,
            ewk_view_url_set(GetEwkWebView(), GetResourceUrl(kUrl).c_str()));
  ASSERT_EQ(Success, EventLoopStart());

  Eina_Bool result = ewk_view_scroll_set(GetEwkWebView(), kScrollX, kScrollY);
  ASSERT_EQ(EINA_TRUE, result);
  int x = 0, y = 0;
  result = ewk_view_scroll_pos_get(GetEwkWebView(), &x, &y);
  ASSERT_EQ(EINA_TRUE, result);
  EXPECT_EQ(kScrollX, x);
  EXPECT_EQ(kScrollY, y);
  result = ewk_view_scroll_set(GetEwkWebView(), kScrollY, kScrollX);
  ASSERT_EQ(EINA_TRUE, result);
  result = ewk_view_scroll_pos_get(GetEwkWebView(), &x, &y);
  ASSERT_EQ(EINA_TRUE, result);
  EXPECT_EQ(kScrollY, x);
  EXPECT_EQ(kScrollX, y);
}

/**
 *  * @brief Check if x scroll value got equals to positive x value set
 *   *        while y passed is null.
 *    */
TEST_F(utc_blink_ewk_view_scroll_pos_get, POS_SCROLL_Y_NULL) {
  ASSERT_EQ(EINA_TRUE,
            ewk_view_url_set(GetEwkWebView(), GetResourceUrl(kUrl).c_str()));
  ASSERT_EQ(Success, EventLoopStart());

  Eina_Bool result = ewk_view_scroll_set(GetEwkWebView(), kScrollX, kScrollY);
  ASSERT_EQ(EINA_TRUE, result);
  int x = 0;
  result = ewk_view_scroll_pos_get(GetEwkWebView(), &x, NULL);
  ASSERT_EQ(EINA_TRUE, result);
  EXPECT_EQ(kScrollX, x);
}

/**
 *  * @brief Check if y scroll value got equals to positive y value set
 *   *        while x passed is null.
 *    */
TEST_F(utc_blink_ewk_view_scroll_pos_get, POS_SCROLL_X_NULL) {
  ASSERT_EQ(EINA_TRUE,
            ewk_view_url_set(GetEwkWebView(), GetResourceUrl(kUrl).c_str()));
  ASSERT_EQ(Success, EventLoopStart());

  Eina_Bool result = ewk_view_scroll_set(GetEwkWebView(), kScrollX, kScrollY);
  ASSERT_EQ(EINA_TRUE, result);
  int y = 0;
  result = ewk_view_scroll_pos_get(GetEwkWebView(), NULL, &y);
  ASSERT_EQ(EINA_TRUE, result);
  EXPECT_EQ(kScrollY, y);
}

/**
 *  * @brief Check if api succeeds if x and y values passed are null
 *   */
TEST_F(utc_blink_ewk_view_scroll_pos_get, POS_SCROLL_X_Y_NULL) {
  ASSERT_EQ(EINA_TRUE,
            ewk_view_url_set(GetEwkWebView(), GetResourceUrl(kUrl).c_str()));
  ASSERT_EQ(Success, EventLoopStart());

  Eina_Bool result = ewk_view_scroll_set(GetEwkWebView(), kScrollX, kScrollY);
  ASSERT_EQ(EINA_TRUE, result);
  result = ewk_view_scroll_pos_get(GetEwkWebView(), NULL, NULL);
  ASSERT_EQ(EINA_TRUE, result);
}

/**
 *  * @brief Check if api returns EINA_FALSE for no context initialized
 *   */
TEST_F(utc_blink_ewk_view_scroll_pos_get, NEG_NO_CONTEXT) {
  int x, y;
  ASSERT_EQ(EINA_FALSE, ewk_view_scroll_pos_get(GetEwkWebView(), &x, &y));
}

/**
 *  * @brief Check if api returns EINA_FALSE for null webview
 *   */
TEST_F(utc_blink_ewk_view_scroll_pos_get, NEG_INVALID_WEBVIEW) {
  int x, y;
  ASSERT_EQ(EINA_FALSE, ewk_view_scroll_pos_get(NULL, &x, &y));
}

