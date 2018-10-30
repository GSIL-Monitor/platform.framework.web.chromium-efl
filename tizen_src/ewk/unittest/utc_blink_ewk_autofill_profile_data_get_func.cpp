// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "utc_blink_ewk_base.h"

#include "ewk_autofill_profile.h"

class utc_blink_ewk_autofill_profile_data_get : public utc_blink_ewk_base
{
protected:
  utc_blink_ewk_autofill_profile_data_get() : m_profileName("MyProfile")
  {
    m_profile = ewk_autofill_profile_new();
  }

  ~utc_blink_ewk_autofill_profile_data_get() override {
    ewk_autofill_profile_delete(m_profile);
  }

  const std::string m_profileName;
  Ewk_Autofill_Profile* m_profile;
};

TEST_F(utc_blink_ewk_autofill_profile_data_get, POS_TEST)
{
  ASSERT_TRUE(m_profile != NULL);

  ewk_autofill_profile_data_set(m_profile, EWK_PROFILE_NAME,
      m_profileName.c_str());

  const std::string result(ewk_autofill_profile_data_get(m_profile,
      EWK_PROFILE_NAME));

  ASSERT_TRUE(result.compare(m_profileName) == 0);
}
