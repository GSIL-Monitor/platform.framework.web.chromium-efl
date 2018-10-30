// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

class utc_blink_ewk_settings_text_autosizing_font_scale_factor_set_func : public utc_blink_ewk_base
{
protected:
 void PostSetUp() override {
   settings = ewk_view_settings_get(GetEwkWebView());
   ASSERT_TRUE(settings != NULL);
  }

  utc_blink_ewk_settings_text_autosizing_font_scale_factor_set_func() : settings(NULL){
  }

protected:
  Ewk_Settings* settings;
};

/**
 * @brief Positive test case of ewk_settings_text_autosizing_font_scale_factor_set()
 */
TEST_F(utc_blink_ewk_settings_text_autosizing_font_scale_factor_set_func, SetPositive)
{
  double factor = 1.23;
  ASSERT_TRUE(ewk_settings_text_autosizing_font_scale_factor_set(settings, factor));
  double setFactor = ewk_settings_text_autosizing_font_scale_factor_get(settings);
  const double delta = 0.001;
  EXPECT_TRUE((factor - setFactor) < delta && (setFactor - factor) < delta);
}

/**
 * @brief Positive test case of ewk_settings_text_autosizing_font_scale_factor_set()
 */
TEST_F(utc_blink_ewk_settings_text_autosizing_font_scale_factor_set_func, SetNegative)
{
  EXPECT_FALSE(ewk_settings_text_autosizing_font_scale_factor_set(settings, -1.0));
}

/**
 * @brief Test case of ewk_settings_text_autosizing_font_scale_factor_set() with null
 */
TEST_F(utc_blink_ewk_settings_text_autosizing_font_scale_factor_set_func, InvalidArg)
{
  EXPECT_FALSE(ewk_settings_text_autosizing_font_scale_factor_set(NULL, 1.0));
}
