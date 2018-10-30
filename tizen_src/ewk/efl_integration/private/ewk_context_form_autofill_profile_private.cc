// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ewk_context_form_autofill_profile_private.h"

#if defined(TIZEN_AUTOFILL_SUPPORT)

#include <vector>
#include <string>
#include <sstream>
#include <Eina.h>

#include "base/guid.h"
#include "base/strings/stringprintf.h"
#include "browser/autofill/personal_data_manager_factory.h"
#include "components/autofill/core/browser/personal_data_manager.h"
#include "eweb_view.h"
#include "ewk_autofill_profile_private.h"
#include "private/ewk_context_private.h"

namespace autofill {
class PersonalDataManagerFactory;
}

namespace {
unsigned GUIDToUnsigned(const std::string& guid) {
  unsigned result = 0;
  size_t pos = guid.rfind('-');
  if (std::string::npos == pos)
    pos = 0;
  else
    pos++;
  std::string substring = guid.substr(pos);
  std::stringstream stream;
  stream << substring;
  stream >> result;
  return result;
}

const std::string kEwkGuidStartString("00000000-0000-0000-0000-");

std::string UnsignedToGUID(unsigned id) {
  return kEwkGuidStartString + base::StringPrintf("%012u", id);
}

bool IsValidEwkGUID(const std::string& guid) {
  if (!base::IsValidGUID(guid))
    return false;

  if (guid.compare(0, kEwkGuidStartString.length(), kEwkGuidStartString))
    return false;

  for (size_t i = kEwkGuidStartString.length(); i < guid.length(); ++i) {
    if (guid[i] < '0' || guid[i] > '9')
      return false;
  }

  return true;
}
}

static std::map<DataType, autofill::ServerFieldType> create_EWK_to_Autofill_profile_map() {
  std::map<DataType, autofill::ServerFieldType> profile_map;
  profile_map[PROFILE_NAME] = autofill::NAME_FULL;
  profile_map[PROFILE_COMPANY] = autofill::COMPANY_NAME;
  profile_map[PROFILE_ADDRESS1] = autofill::ADDRESS_HOME_LINE1;
  profile_map[PROFILE_ADDRESS2] = autofill::ADDRESS_HOME_LINE2;
  profile_map[PROFILE_CITY_TOWN] = autofill::ADDRESS_HOME_CITY;
  profile_map[PROFILE_STATE_PROVINCE_REGION] = autofill::ADDRESS_HOME_STATE;
  profile_map[PROFILE_ZIPCODE] = autofill::ADDRESS_HOME_ZIP;
  profile_map[PROFILE_COUNTRY] = autofill::ADDRESS_HOME_COUNTRY;
  profile_map[PROFILE_PHONE] = autofill::PHONE_HOME_WHOLE_NUMBER;
  profile_map[PROFILE_EMAIL] = autofill::EMAIL_ADDRESS;
  return profile_map;
}

void to_Autofill_Profile_set_data(const Ewk_Autofill_Profile* oldStyleProfile,
    DataType DataName,
    std::string locale,
    autofill::AutofillProfile &ret) {
  base::string16 value;
  static std::map<DataType, autofill::ServerFieldType> profile_map =
      create_EWK_to_Autofill_profile_map();
  if (0 < (value = oldStyleProfile->get16Data(DataName)).length()) {
    if (DataName == PROFILE_NAME || DataName == PROFILE_COUNTRY)
      ret.SetInfo(autofill::AutofillType(profile_map.find(DataName)->second),
                  value, locale);
    else
      ret.SetRawInfo(profile_map.find(DataName)->second, value);
  }
}

autofill::AutofillProfile to_Autofill_Profile(
    const Ewk_Autofill_Profile* oldStyleProfile) {
  std::string locale = EWebView::GetPlatformLocale();
  autofill::AutofillProfile ret(base::GenerateGUID(), locale);

  if (!oldStyleProfile)
    return ret;

  ret.set_guid(UnsignedToGUID(oldStyleProfile->getProfileID()));

  to_Autofill_Profile_set_data(oldStyleProfile, PROFILE_NAME,
                               locale, ret);
  to_Autofill_Profile_set_data(oldStyleProfile, PROFILE_COMPANY,
                               locale, ret);
  to_Autofill_Profile_set_data(oldStyleProfile, PROFILE_ADDRESS1,
                               locale, ret);
  to_Autofill_Profile_set_data(oldStyleProfile, PROFILE_ADDRESS2,
                               locale, ret);
  to_Autofill_Profile_set_data(oldStyleProfile, PROFILE_CITY_TOWN,
                               locale, ret);
  to_Autofill_Profile_set_data(oldStyleProfile, PROFILE_STATE_PROVINCE_REGION,
                               locale, ret);
  to_Autofill_Profile_set_data(oldStyleProfile, PROFILE_ZIPCODE,
                               locale, ret);
  to_Autofill_Profile_set_data(oldStyleProfile, PROFILE_COUNTRY,
                               locale, ret);
  to_Autofill_Profile_set_data(oldStyleProfile, PROFILE_PHONE,
                               locale, ret);
  to_Autofill_Profile_set_data(oldStyleProfile, PROFILE_EMAIL,
                               locale, ret);
  return ret;
}

static std::map<autofill::ServerFieldType, DataType>
create_Autofill_to_EWK_profile_map() {
  std::map<autofill::ServerFieldType, DataType> profile_map;
  std::map<DataType, autofill::ServerFieldType> autofill_map =
      create_EWK_to_Autofill_profile_map();
  for (std::map<DataType, autofill::ServerFieldType>::iterator it =
      autofill_map.begin(); it!=autofill_map.end(); ++it)
    profile_map[it->second] = it->first;
  return profile_map;
}

void to_EWK_Profile_set_data(const autofill::AutofillProfile& newStyleProfile,
    autofill::ServerFieldType DataName,
    std::string locale,
    Ewk_Autofill_Profile* ret) {
  base::string16 value;
  static std::map<autofill::ServerFieldType, DataType> profile_map =
      create_Autofill_to_EWK_profile_map();
  if (DataName == autofill::NAME_FULL ||
      DataName == autofill::ADDRESS_HOME_COUNTRY)
    value = newStyleProfile.GetInfo(autofill::AutofillType(DataName), locale);
  else
    value = newStyleProfile.GetRawInfo(DataName);
  if (value.length())
    ret->setData(profile_map.find(DataName)->second, base::UTF16ToASCII(value));
}

Ewk_Autofill_Profile* to_Ewk_Autofill_Profile(
      const autofill::AutofillProfile& newStyleProfile) {
  if (!IsValidEwkGUID(newStyleProfile.guid()))
    return nullptr;
  Ewk_Autofill_Profile* ret = new Ewk_Autofill_Profile();
  std::string locale = EWebView::GetPlatformLocale();
  ret->setProfileID(GUIDToUnsigned(newStyleProfile.guid()));
  to_EWK_Profile_set_data(newStyleProfile,
      autofill::NAME_FULL, locale, ret);
  to_EWK_Profile_set_data(newStyleProfile,
      autofill::COMPANY_NAME, locale, ret);
  to_EWK_Profile_set_data(newStyleProfile,
      autofill::ADDRESS_HOME_LINE1, locale, ret);
  to_EWK_Profile_set_data(newStyleProfile,
      autofill::ADDRESS_HOME_LINE2, locale, ret);
  to_EWK_Profile_set_data(newStyleProfile,
      autofill::ADDRESS_HOME_CITY, locale, ret);
  to_EWK_Profile_set_data(newStyleProfile,
      autofill::ADDRESS_HOME_STATE, locale, ret);
  to_EWK_Profile_set_data(newStyleProfile,
      autofill::ADDRESS_HOME_ZIP, locale, ret);
  to_EWK_Profile_set_data(newStyleProfile, autofill::ADDRESS_HOME_COUNTRY,
                          locale, ret);
  to_EWK_Profile_set_data(newStyleProfile, autofill::PHONE_HOME_WHOLE_NUMBER,
                          locale, ret);
  to_EWK_Profile_set_data(newStyleProfile,
      autofill::EMAIL_ADDRESS, locale, ret);
  return ret;
}

autofill::PersonalDataManager* PersonalDataManagerForEWKContext(
    Ewk_Context* ewk_context) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(ewk_context, nullptr);

  autofill::PersonalDataManagerFactory* personal_data_manager_factory =
      autofill::PersonalDataManagerFactory::GetInstance();
  content::BrowserContext* context = ewk_context->browser_context();
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, nullptr);

  return personal_data_manager_factory->PersonalDataManagerForContext(context);
}

Eina_List*
EwkContextFormAutofillProfileManager::priv_form_autofill_profile_get_all(
    Ewk_Context* ewk_context) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(ewk_context, nullptr);

  autofill::PersonalDataManager* manager =
      PersonalDataManagerForEWKContext(ewk_context);
  DCHECK(manager);

  std::vector<autofill::AutofillProfile*> dataVector =
      manager->GetProfiles();
  Eina_List* list = nullptr;
  for (unsigned i = 0; i < dataVector.size(); ++i) {
    autofill::AutofillProfile* profile = dataVector[i];
    if (profile) {
      Ewk_Autofill_Profile* p = to_Ewk_Autofill_Profile(*profile);
      if (p)
        list = eina_list_append(list, p);
    }
  }
  return list;
}

Ewk_Autofill_Profile*
EwkContextFormAutofillProfileManager::priv_form_autofill_profile_get(
    Ewk_Context* ewk_context, unsigned id) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(ewk_context, nullptr);

  autofill::PersonalDataManager* manager =
      PersonalDataManagerForEWKContext(ewk_context);
  DCHECK(manager);

  autofill::AutofillProfile* profile = manager->GetProfileByGUID(UnsignedToGUID(id));

  Ewk_Autofill_Profile* ret = nullptr;
  if (profile)
    ret = to_Ewk_Autofill_Profile(*profile);
  return ret;
}

Eina_Bool EwkContextFormAutofillProfileManager::priv_form_autofill_profile_set(
    Ewk_Context* ewk_context,
    unsigned id, Ewk_Autofill_Profile* ewk_profile) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(ewk_context, EINA_FALSE);

  autofill::PersonalDataManager* manager =
      PersonalDataManagerForEWKContext(ewk_context);
  DCHECK(manager);

  autofill::AutofillProfile* profile = manager->GetProfileByGUID(UnsignedToGUID(id));
  if (profile)
    manager->UpdateProfile(to_Autofill_Profile(ewk_profile));
  else
    manager->AddProfile(to_Autofill_Profile(ewk_profile));
  return EINA_TRUE;
}

Eina_Bool EwkContextFormAutofillProfileManager::priv_form_autofill_profile_add(
    Ewk_Context* ewk_context, Ewk_Autofill_Profile* profile) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(ewk_context, EINA_FALSE);

  autofill::PersonalDataManager* manager =
      PersonalDataManagerForEWKContext(ewk_context);
  DCHECK(manager);

  manager->AddProfile(to_Autofill_Profile(profile));
  return EINA_TRUE;
}

Eina_Bool
EwkContextFormAutofillProfileManager::priv_form_autofill_profile_remove(
    Ewk_Context* ewk_context, unsigned id) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(ewk_context, EINA_FALSE);

  autofill::PersonalDataManager* manager =
      PersonalDataManagerForEWKContext(ewk_context);
  DCHECK(manager);

  manager->RemoveByGUID(UnsignedToGUID(id));
  return EINA_TRUE;
}

/* LCOV_EXCL_START */
void
EwkContextFormAutofillProfileManager::priv_form_autofill_profile_changed_callback_set(
    Ewk_Context_Form_Autofill_Profile_Changed_Callback callback,
    void* user_data) {
  autofill::PersonalDataManagerFactory* personalDataManagerFactory =
      autofill::PersonalDataManagerFactory::GetInstance();
  personalDataManagerFactory->SetCallback(callback, user_data);
}
/* LCOV_EXCL_STOP */

#endif // TIZEN_AUTOFILL_SUPPORT
