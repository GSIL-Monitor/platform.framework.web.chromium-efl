// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"
#include "utc_blink_ewk_context_form_autofill_profile_utils.h"

#include <map>
#include <sstream>

class utc_blink_ewk_context_form_autofill_profile_set
    : public utc_blink_ewk_context_form_autofill_profile_base {
};

TEST_F(utc_blink_ewk_context_form_autofill_profile_set, POS_TEST)
{
  Ewk_Autofill_Profile* profile = getTestEwkAutofillProfile();
  ASSERT_TRUE(NULL != profile);
  Eina_Bool result = ewk_context_form_autofill_profile_add(
      ewk_context_default_get(), profile);
  ewk_autofill_profile_delete(profile);
  EventLoopWait(3.0);
  ASSERT_TRUE(EINA_FALSE != result);
  profile = getTestEwkAutofillProfile();
  ASSERT_TRUE(NULL != profile);
  //change some property
  const char* new_company_name = "anotherCompany";
  ewk_autofill_profile_data_set(profile, EWK_PROFILE_COMPANY, new_company_name);
  //set it
  ASSERT_EQ(EINA_TRUE, ewk_context_form_autofill_profile_set(
      ewk_context_default_get(),TEST_AUTOFILL_PROFILE_ID, profile));
  ewk_autofill_profile_delete(profile);
  //get it and check
  EventLoopWait(3.0);
  profile = ewk_context_form_autofill_profile_get(ewk_context_default_get(),
                                                  TEST_AUTOFILL_PROFILE_ID);
  ASSERT_TRUE(NULL != profile);
  EXPECT_TRUE(checkOne(profile, new_company_name, EWK_PROFILE_COMPANY, "company"));

  ewk_autofill_profile_delete(profile);
}

TEST_F(utc_blink_ewk_context_form_autofill_profile_set, NEG_TEST)
{
  Ewk_Autofill_Profile* profile = getTestEwkAutofillProfile();
  Ewk_Context* context = ewk_context_default_get();
  ASSERT_TRUE(NULL != profile);
  ASSERT_TRUE(NULL != context);
  ASSERT_EQ(NULL, ewk_context_form_autofill_profile_set(
      NULL, TEST_AUTOFILL_PROFILE_ID, profile));
  ASSERT_EQ(NULL, ewk_context_form_autofill_profile_set(
      context, TEST_AUTOFILL_PROFILE_ID, NULL));
  ewk_autofill_profile_delete(profile);
}
