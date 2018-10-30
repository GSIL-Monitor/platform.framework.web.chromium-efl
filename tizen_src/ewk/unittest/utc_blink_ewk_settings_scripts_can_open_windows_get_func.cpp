// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

class utc_blink_ewk_settings_scripts_can_open_windows_get : public utc_blink_ewk_base {
protected:
  utc_blink_ewk_settings_scripts_can_open_windows_get()
    : settings(NULL)
  {
  }

  void PostSetUp() override {
    settings = ewk_view_settings_get(GetEwkWebView());
    ASSERT_TRUE(settings);
    // make sure default value is proper
    ASSERT_EQ(EINA_TRUE, ewk_settings_scripts_can_open_windows_get(settings));
  }

protected:
  Ewk_Settings* settings;
};

/**
 * @brief Positive test case of ewk_settings_scripts_can_open_windows_get()
 */
TEST_F(utc_blink_ewk_settings_scripts_can_open_windows_get, SetFalse)
{
  // toggle option
  ASSERT_EQ(EINA_TRUE, ewk_settings_scripts_can_open_windows_set(settings, EINA_FALSE));
  // check if option was toggled
  ASSERT_EQ(EINA_FALSE, ewk_settings_scripts_can_open_windows_get(settings));
}

/**
 * @brief Test case of ewk_settings_scripts_can_open_windows_get() when view is NULL
 */
TEST_F(utc_blink_ewk_settings_scripts_can_open_windows_get, InvalidArg)
{
  EXPECT_EQ(EINA_FALSE, ewk_settings_scripts_can_open_windows_get(NULL));
}
