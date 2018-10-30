// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// See README.md how to run this test in DESKTOP mode.

#include "utc_blink_ewk_base.h"

class utc_blink_ewk_context_application_type_get : public utc_blink_ewk_base {};

/**
 * @brief Checking whether application type is returned to
 * EWK_APPLICATION_TYPE_WEBBROWSER.
 */
TEST_F(utc_blink_ewk_context_application_type_get, POS_TEST1) {
  Ewk_Application_Type orig_type = EWK_APPLICATION_TYPE_WEBBROWSER;

  // set application type
  ewk_context_application_type_set(ewk_context_default_get(), orig_type);

  // check if returned type is the same as original type
  Ewk_Application_Type new_type =
      ewk_context_application_type_get(ewk_context_default_get());
  ASSERT_EQ(orig_type, new_type);
}

/**
 * @brief Checking whether application type is returned to
 * EWK_APPLICATION_TYPE_HBBTV.
 */
TEST_F(utc_blink_ewk_context_application_type_get, POS_TEST2) {
  Ewk_Application_Type orig_type = EWK_APPLICATION_TYPE_HBBTV;

  // set application type
  ewk_context_application_type_set(ewk_context_default_get(), orig_type);

  // check if returned type is the same as original type
  Ewk_Application_Type new_type =
      ewk_context_application_type_get(ewk_context_default_get());
  ASSERT_EQ(orig_type, new_type);
}

/**
 * @brief Checking whether application type is returned to
 * EWK_APPLICATION_TYPE_TIZENWRT.
 */
TEST_F(utc_blink_ewk_context_application_type_get, POS_TEST3) {
  Ewk_Application_Type orig_type = EWK_APPLICATION_TYPE_TIZENWRT;

  // set application type
  ewk_context_application_type_set(ewk_context_default_get(), orig_type);

  // check if returned type is the same as original type
  Ewk_Application_Type new_type =
      ewk_context_application_type_get(ewk_context_default_get());
  ASSERT_EQ(orig_type, new_type);
}
