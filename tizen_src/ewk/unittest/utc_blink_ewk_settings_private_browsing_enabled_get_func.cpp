// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

class utc_blink_ewk_settings_private_browsing_enabled_get_func : public utc_blink_ewk_base
{
};


/**
 * @brief Tests getting private browsing enabled.
 */
TEST_F(utc_blink_ewk_settings_private_browsing_enabled_get_func, POS_TEST_TRUE)
{
  Ewk_Settings* settings = ewk_view_settings_get(GetEwkWebView());
  ASSERT_TRUE(settings);

  ewk_settings_private_browsing_enabled_set(settings, EINA_TRUE);
  ASSERT_TRUE(ewk_settings_private_browsing_enabled_get(settings));
}

/**
 * @brief Tests if getting private browsing enabled with NULL settings fails.
 */
TEST_F(utc_blink_ewk_settings_private_browsing_enabled_get_func, NEG_TEST)
{

  Ewk_Settings* settings = ewk_view_settings_get(GetEwkWebView());
  ASSERT_TRUE(settings);
  ewk_settings_private_browsing_enabled_set(settings, EINA_TRUE);
  ASSERT_TRUE(ewk_settings_private_browsing_enabled_get(settings));

  ASSERT_FALSE(ewk_settings_private_browsing_enabled_get(NULL));
}
