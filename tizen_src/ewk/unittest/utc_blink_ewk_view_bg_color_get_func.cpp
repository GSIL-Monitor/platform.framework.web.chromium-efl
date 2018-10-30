// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

class utc_blink_ewk_view_bg_color_get : public utc_blink_ewk_base {
 public:
  utc_blink_ewk_view_bg_color_get()
      : utc_blink_ewk_base(),
        bgcolor_r_value_(23),
        bgcolor_g_value_(30),
        bgcolor_b_value_(55),
        bgcolor_a_value_(2) {}

  void LoadFinished(Evas_Object* webview) override { EventLoopStop(Success); }

  static void BackgroundColorGetCallback(Evas_Object* o,
                                         int r,
                                         int g,
                                         int b,
                                         int a,
                                         void* user_data) {
    ASSERT_TRUE(user_data != NULL);

    utc_blink_ewk_view_bg_color_get* const owner =
        static_cast<utc_blink_ewk_view_bg_color_get*>(user_data);

    owner->EventLoopStop(
        owner->bgcolor_r_value_ == r && owner->bgcolor_g_value_ == g &&
                owner->bgcolor_b_value_ == b && owner->bgcolor_a_value_ == a
            ? Success
            : Failure);
  }

  static Eina_Bool GetBackgroundColorOnTimer(void* user_data) {
    if (user_data) {
      utc_blink_ewk_view_bg_color_get* const owner =
          static_cast<utc_blink_ewk_view_bg_color_get*>(user_data);

      if (!ewk_view_bg_color_get(owner->GetEwkWebView(),
                                 BackgroundColorGetCallback, owner)) {
        owner->EventLoopStop(Failure);
      }
    }
    return ECORE_CALLBACK_CANCEL;
  }

 protected:
  unsigned int bgcolor_r_value_;
  unsigned int bgcolor_g_value_;
  unsigned int bgcolor_b_value_;
  unsigned int bgcolor_a_value_;
};

/**
* @brief Tests if setting and getting the background color is fetched
*  from webkit properly.
*/
TEST_F(utc_blink_ewk_view_bg_color_get, POS_TEST) {
  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(),
                               GetResourceUrl("/common/sample.html").c_str()));

  ASSERT_EQ(EventLoopStart(), Success);

  ewk_view_bg_color_set(GetEwkWebView(), bgcolor_r_value_, bgcolor_g_value_,
                        bgcolor_b_value_, bgcolor_a_value_);

  ASSERT_TRUE(ecore_timer_add(1, GetBackgroundColorOnTimer, this) != NULL);

  ASSERT_EQ(EventLoopStart(), Success);
}

/**
* @brief Tests if getter returns false when callback function is NULL
*  or view is NULL
*/
TEST_F(utc_blink_ewk_view_bg_color_get, NEG_TEST) {
  EXPECT_FALSE(ewk_view_bg_color_get(NULL, BackgroundColorGetCallback, this));
  EXPECT_FALSE(ewk_view_bg_color_get(GetEwkWebView(), NULL, this));
}
