// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"
#include "utc_blink_ewk_context_form_autofill_profile_utils.h"

#include <map>
#include <sstream>

class utc_blink_ewk_context_form_autofill_profile_remove
    : public utc_blink_ewk_context_form_autofill_profile_base {
};

TEST_F(utc_blink_ewk_context_form_autofill_profile_remove, POS_TEST)
{
  Ewk_Autofill_Profile* profile = getTestEwkAutofillProfile();
  ASSERT_TRUE(NULL != profile);
  Eina_Bool result = ewk_context_form_autofill_profile_add(
      ewk_context_default_get(), profile);
  ewk_autofill_profile_delete(profile);
  ASSERT_TRUE(EINA_FALSE != result);
  EventLoopWait(3.0);
  profile = ewk_context_form_autofill_profile_get(ewk_context_default_get(),
                                                  TEST_AUTOFILL_PROFILE_ID);
  ASSERT_TRUE(NULL != profile);
  ASSERT_EQ(EINA_TRUE, ewk_context_form_autofill_profile_remove(
      ewk_context_default_get(), TEST_AUTOFILL_PROFILE_ID));
  ewk_autofill_profile_delete(profile);
  EventLoopWait(3.0);
  profile = ewk_context_form_autofill_profile_get(ewk_context_default_get(),
                                                  TEST_AUTOFILL_PROFILE_ID);
  EXPECT_TRUE(NULL == profile);
}

TEST_F(utc_blink_ewk_context_form_autofill_profile_remove, NEG_TEST)
{
  ASSERT_EQ(EINA_FALSE, ewk_context_form_autofill_profile_remove(
      NULL, TEST_AUTOFILL_PROFILE_ID));
}
