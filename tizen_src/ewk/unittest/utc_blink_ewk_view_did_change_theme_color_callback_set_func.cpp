// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

static const char* const kThemeColor1Url = "common/theme-color_1.html";
static const char* const kThemeColor2Url = "common/theme-color_2.html";
static const char* const kNoThemeColorUrl = "common/sample.html";

class utc_blink_ewk_view_did_change_theme_color_callback_set
    : public utc_blink_ewk_base {
 public:
  utc_blink_ewk_view_did_change_theme_color_callback_set()
      : evas_object_(nullptr), url_loaded_(false) {}

  void LoadFinished(Evas_Object* o) override { url_loaded_ = true; }

  static void DidChangeThemeColorCallback(Evas_Object* o,
                                          int r,
                                          int g,
                                          int b,
                                          int a,
                                          void* user_data) {
    if (!user_data)
      return;
    auto owner =
        static_cast<utc_blink_ewk_view_did_change_theme_color_callback_set*>(
            user_data);
    owner->evas_object_ = o;
    owner->r_ = r;
    owner->g_ = g;
    owner->b_ = b;
    owner->a_ = a;
    owner->EventLoopStop(Success);
  }

  static void ThemeColor1UrlSetJob(utc_blink_ewk_base* data) {
    auto owner =
        static_cast<utc_blink_ewk_view_did_change_theme_color_callback_set*>(
            data);
    ewk_view_url_set(owner->GetEwkWebView(),
                     owner->GetResourceUrl(kThemeColor1Url).c_str());
  }

  static void ThemeColor2UrlSetJob(utc_blink_ewk_base* data) {
    auto owner =
        static_cast<utc_blink_ewk_view_did_change_theme_color_callback_set*>(
            data);
    ewk_view_url_set(owner->GetEwkWebView(),
                     owner->GetResourceUrl(kThemeColor2Url).c_str());
  }

  static void NoThemeColorUrlSetJob(utc_blink_ewk_base* data) {
    auto owner =
        static_cast<utc_blink_ewk_view_did_change_theme_color_callback_set*>(
            data);
    ewk_view_url_set(owner->GetEwkWebView(),
                     owner->GetResourceUrl(kNoThemeColorUrl).c_str());
  }

 protected:
  int r_;
  int g_;
  int b_;
  int a_;
  Evas_Object* evas_object_;
  Eina_Bool url_loaded_;
};

/**
 * @brief Checks if color received in callback is same as in "theme-color" meta
 *        tag.
**/
TEST_F(utc_blink_ewk_view_did_change_theme_color_callback_set,
       POS_THEME_COLOR) {
  ASSERT_EQ(EINA_TRUE, ewk_view_did_change_theme_color_callback_set(
                           GetEwkWebView(), DidChangeThemeColorCallback, this));
  SetTestJob(ThemeColor1UrlSetJob);
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_EQ(GetEwkWebView(), evas_object_);
  EXPECT_EQ(50, r_);
  EXPECT_EQ(100, g_);
  EXPECT_EQ(150, b_);
  EXPECT_EQ(255, a_);
}

/**
 * @brief Checks if callback is received only first time, when urls with same
 *        theme color are loaded one after another.
**/
TEST_F(utc_blink_ewk_view_did_change_theme_color_callback_set,
       POS_THEME_COLORS_SAME) {
  ASSERT_EQ(EINA_TRUE, ewk_view_did_change_theme_color_callback_set(
                           GetEwkWebView(), DidChangeThemeColorCallback, this));
  SetTestJob(ThemeColor1UrlSetJob);
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_EQ(GetEwkWebView(), evas_object_);
  SetTestJob(ThemeColor1UrlSetJob);
  ASSERT_EQ(Timeout, EventLoopStart(3.0));
  ASSERT_TRUE(url_loaded_);
}

/**
 * @brief Checks if callback is received twice first time, when urls with
 *        different theme color are loaded one after another.
**/
TEST_F(utc_blink_ewk_view_did_change_theme_color_callback_set,
       POS_THEME_COLORS_DIFFERENT) {
  ASSERT_EQ(EINA_TRUE, ewk_view_did_change_theme_color_callback_set(
                           GetEwkWebView(), DidChangeThemeColorCallback, this));
  SetTestJob(ThemeColor1UrlSetJob);
  ASSERT_EQ(Success, EventLoopStart());
  evas_object_ = nullptr;
  SetTestJob(ThemeColor2UrlSetJob);
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_EQ(GetEwkWebView(), evas_object_);
  EXPECT_EQ(100, r_);
  EXPECT_EQ(150, g_);
  EXPECT_EQ(200, b_);
  EXPECT_EQ(255, a_);
}

/**
 * @brief Checks if color is not received if url with no theme color is loaded
 *        as first one.
**/
TEST_F(utc_blink_ewk_view_did_change_theme_color_callback_set,
       POS_NO_THEME_COLOR) {
  ASSERT_EQ(EINA_TRUE, ewk_view_did_change_theme_color_callback_set(
                           GetEwkWebView(), DidChangeThemeColorCallback, this));
  SetTestJob(NoThemeColorUrlSetJob);
  ASSERT_EQ(Timeout, EventLoopStart(3.0));
  ASSERT_TRUE(url_loaded_);
}

/**
 * @brief Checks if callback is received if url with no theme color is loaded
 *        after url with theme color.
**/
TEST_F(utc_blink_ewk_view_did_change_theme_color_callback_set,
       POS_NO_THEME_COLOR_AFTER_THEME_COLOR) {
  ASSERT_EQ(EINA_TRUE, ewk_view_did_change_theme_color_callback_set(
                           GetEwkWebView(), DidChangeThemeColorCallback, this));
  SetTestJob(ThemeColor1UrlSetJob);
  ASSERT_EQ(Success, EventLoopStart());
  SetTestJob(NoThemeColorUrlSetJob);
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_EQ(GetEwkWebView(), evas_object_);
  EXPECT_EQ(0, r_);
  EXPECT_EQ(0, g_);
  EXPECT_EQ(0, b_);
  EXPECT_EQ(0, a_);
}

/**
 * @brief Checks if callback is reset properly.
**/
TEST_F(utc_blink_ewk_view_did_change_theme_color_callback_set,
       POS_RESET_CALLBACK) {
  ASSERT_EQ(EINA_TRUE, ewk_view_did_change_theme_color_callback_set(
                           GetEwkWebView(), DidChangeThemeColorCallback, this));
  SetTestJob(ThemeColor1UrlSetJob);
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_EQ(EINA_TRUE, ewk_view_did_change_theme_color_callback_set(
                           GetEwkWebView(), nullptr, this));
  SetTestJob(ThemeColor2UrlSetJob);
  ASSERT_EQ(Timeout, EventLoopStart(3.0));
  ASSERT_TRUE(url_loaded_);
}

/**
 * @brief Checks if fails with webview being null.
**/
TEST_F(utc_blink_ewk_view_did_change_theme_color_callback_set,
       NEG_INVALID_WEBVIEW) {
  ASSERT_EQ(EINA_FALSE, ewk_view_did_change_theme_color_callback_set(
                            nullptr, DidChangeThemeColorCallback, this));
}
