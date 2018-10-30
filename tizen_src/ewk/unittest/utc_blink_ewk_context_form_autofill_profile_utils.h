// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UTC_BLINK_EWK_CONTEXT_FORM_AUTOFILL_PROFILE_UTILS_H
#define UTC_BLINK_EWK_CONTEXT_FORM_AUTOFILL_PROFILE_UTILS_H

#include "utc_blink_ewk_base.h"

const unsigned     TEST_AUTOFILL_PROFILE_ID                    =   12345;
const char* const  TEST_AUTOFILL_PROFILE_NAME                  =   "Mr. Smith";
const char* const  TEST_AUTOFILL_PROFILE_COMPANY               =   "Samsung";
const char* const  TEST_AUTOFILL_PROFILE_ADDRESS1              =   "Existing Street 15";
const char* const  TEST_AUTOFILL_PROFILE_ADDRESS2              =   "NonExisting Street -15";
const char* const  TEST_AUTOFILL_PROFILE_CITY_TOWN             =   "Capitol";
const char* const  TEST_AUTOFILL_PROFILE_STATE_PROVINCE_REGION =   "Beautiful Province";
const char* const  TEST_AUTOFILL_PROFILE_ZIPCODE               =   "12-345";
const char* const  TEST_AUTOFILL_PROFILE_COUNTRY               =   "Neverland";
const char* const  TEST_AUTOFILL_PROFILE_PHONE                 =   "123456789";
const char* const  TEST_AUTOFILL_PROFILE_EMAIL                 =   "someEmail@someServer.com";

Ewk_Autofill_Profile* getTestEwkAutofillProfile();

bool checkOne(Ewk_Autofill_Profile* profileToCheck, const char* reference,
              Ewk_Autofill_Profile_Data_Type kind, const char* errMessage);
bool checkIfProfileContainsTestData(Ewk_Autofill_Profile* profileToCheck);

class utc_blink_ewk_context_form_autofill_profile_base
    : public utc_blink_ewk_base {
 protected:
  void PostSetUp() override;

  void PreTearDown() override;

 private:
  void RemoveTestProfile() {
    Ewk_Autofill_Profile* profile =
        ewk_context_form_autofill_profile_get(ewk_context_default_get(),
                                              TEST_AUTOFILL_PROFILE_ID);
    if (profile) {
      ewk_context_form_autofill_profile_remove(ewk_context_default_get(),
                                               TEST_AUTOFILL_PROFILE_ID);
      ewk_autofill_profile_delete(profile);
      EventLoopWait(3.0);
      profile =
          ewk_context_form_autofill_profile_get(ewk_context_default_get(),
                                                TEST_AUTOFILL_PROFILE_ID);
      ASSERT_TRUE(profile == nullptr);
    }
  }
};

#endif //UTC_BLINK_EWK_CONTEXT_FORM_AUTOFILL_PROFILE_UTILS_H
