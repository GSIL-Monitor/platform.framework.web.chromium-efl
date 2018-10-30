// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ewk_context_form_autofill_credit_card_private.h"

#if defined(TIZEN_AUTOFILL_SUPPORT)

#include <Eina.h>
#include <sstream>
#include <string>
#include <vector>

#include "base/guid.h"
#include "base/strings/stringprintf.h"
#include "browser/autofill/personal_data_manager_factory.h"
#include "components/autofill/core/browser/personal_data_manager.h"
#include "eweb_view.h"
#include "ewk_autofill_credit_card_private.h"
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

autofill::PersonalDataManager* PersonalDataManagerForEWKContext(
    Ewk_Context* ewk_context) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(ewk_context, nullptr);

  autofill::PersonalDataManagerFactory* personal_data_manager_factory =
      autofill::PersonalDataManagerFactory::GetInstance();
  content::BrowserContext* context = ewk_context->browser_context();
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, nullptr);

  return personal_data_manager_factory->PersonalDataManagerForContext(context);
}
}  // namespace

static std::map<CCDataType, autofill::ServerFieldType>
create_EWK_to_Autofill_credit_card_map() {
  std::map<CCDataType, autofill::ServerFieldType> card_map;
  card_map[CREDIT_CARD_NAME_FULL] = autofill::CREDIT_CARD_NAME_FULL;
  card_map[CREDIT_CARD_NUMBER] = autofill::CREDIT_CARD_NUMBER;
  card_map[CREDIT_CARD_EXP_MONTH] = autofill::CREDIT_CARD_EXP_MONTH;
  card_map[CREDIT_CARD_EXP_4_DIGIT_YEAR] =
      autofill::CREDIT_CARD_EXP_4_DIGIT_YEAR;
  return card_map;
}

void to_Autofill_CreditCard_set_data(
    const Ewk_Autofill_CreditCard* oldStyleCreditCard,
    CCDataType DataName,
    std::string locale,
    autofill::CreditCard& ret) {
  base::string16 value;
  static std::map<CCDataType, autofill::ServerFieldType> card_map =
      create_EWK_to_Autofill_credit_card_map();
  if (0 < (value = oldStyleCreditCard->get16Data(DataName)).length())
    ret.SetRawInfo(card_map.find(DataName)->second, value);
}

autofill::CreditCard to_Autofill_CreditCard(
    const Ewk_Autofill_CreditCard* oldStyleCreditCard) {
  std::string locale = EWebView::GetPlatformLocale();
  autofill::CreditCard ret(base::GenerateGUID(), locale);

  if (!oldStyleCreditCard)
    return ret;

  ret.set_guid(UnsignedToGUID(oldStyleCreditCard->getCreditCardID()));

  to_Autofill_CreditCard_set_data(oldStyleCreditCard, CREDIT_CARD_NAME_FULL,
                                  locale, ret);
  to_Autofill_CreditCard_set_data(oldStyleCreditCard, CREDIT_CARD_NUMBER,
                                  locale, ret);
  to_Autofill_CreditCard_set_data(oldStyleCreditCard, CREDIT_CARD_EXP_MONTH,
                                  locale, ret);
  to_Autofill_CreditCard_set_data(oldStyleCreditCard,
                                  CREDIT_CARD_EXP_4_DIGIT_YEAR, locale, ret);
  return ret;
}

static std::map<autofill::ServerFieldType, CCDataType>
create_Autofill_to_EWK_card_map() {
  std::map<autofill::ServerFieldType, CCDataType> card_map;
  std::map<CCDataType, autofill::ServerFieldType> autofill_map =
      create_EWK_to_Autofill_credit_card_map();
  for (std::map<CCDataType, autofill::ServerFieldType>::iterator it =
           autofill_map.begin();
       it != autofill_map.end(); ++it)
    card_map[it->second] = it->first;
  return card_map;
}

void to_EWK_CreditCard_set_data(const autofill::CreditCard& newStyleCreditCard,
                                autofill::ServerFieldType DataName,
                                std::string locale,
                                Ewk_Autofill_CreditCard* ret) {
  base::string16 value;
  static std::map<autofill::ServerFieldType, CCDataType> card_map =
      create_Autofill_to_EWK_card_map();
  value = newStyleCreditCard.GetRawInfo(DataName);
  if (value.length())
    ret->setData(card_map.find(DataName)->second, base::UTF16ToASCII(value));
}

Ewk_Autofill_CreditCard* to_Ewk_Autofill_CreditCard(
    const autofill::CreditCard& newStyleCreditCard) {
  if (!IsValidEwkGUID(newStyleCreditCard.guid()))
    return nullptr;
  Ewk_Autofill_CreditCard* ret = new Ewk_Autofill_CreditCard();
  std::string locale = EWebView::GetPlatformLocale();
  ret->setCreditCardID(GUIDToUnsigned(newStyleCreditCard.guid()));
  to_EWK_CreditCard_set_data(newStyleCreditCard,
                             autofill::CREDIT_CARD_NAME_FULL, locale, ret);
  to_EWK_CreditCard_set_data(newStyleCreditCard, autofill::CREDIT_CARD_NUMBER,
                             locale, ret);
  to_EWK_CreditCard_set_data(newStyleCreditCard,
                             autofill::CREDIT_CARD_EXP_MONTH, locale, ret);
  to_EWK_CreditCard_set_data(
      newStyleCreditCard, autofill::CREDIT_CARD_EXP_4_DIGIT_YEAR, locale, ret);
  return ret;
}

Eina_List*
EwkContextFormAutofillCreditCardManager::priv_form_autofill_credit_card_get_all(
    Ewk_Context* ewk_context) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(ewk_context, nullptr);

  autofill::PersonalDataManager* manager =
      PersonalDataManagerForEWKContext(ewk_context);
  DCHECK(manager);

  std::vector<autofill::CreditCard*> dataVector = manager->GetCreditCards();
  Eina_List* list = nullptr;
  for (unsigned i = 0; i < dataVector.size(); ++i) {
    autofill::CreditCard* card = dataVector[i];
    if (card) {
      Ewk_Autofill_CreditCard* c = to_Ewk_Autofill_CreditCard(*card);
      if (c)
        list = eina_list_append(list, c);
    }
  }
  return list;
}

Ewk_Autofill_CreditCard*
EwkContextFormAutofillCreditCardManager::priv_form_autofill_credit_card_get(
    Ewk_Context* ewk_context,
    unsigned id) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(ewk_context, nullptr);

  autofill::PersonalDataManager* manager =
      PersonalDataManagerForEWKContext(ewk_context);
  DCHECK(manager);

  autofill::CreditCard* card = manager->GetCreditCardByGUID(UnsignedToGUID(id));

  Ewk_Autofill_CreditCard* ret = nullptr;
  if (card)
    ret = to_Ewk_Autofill_CreditCard(*card);
  return ret;
}

Eina_Bool
EwkContextFormAutofillCreditCardManager::priv_form_autofill_credit_card_set(
    Ewk_Context* ewk_context,
    unsigned id,
    Ewk_Autofill_CreditCard* ewk_credit_card) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(ewk_context, EINA_FALSE);

  autofill::PersonalDataManager* manager =
      PersonalDataManagerForEWKContext(ewk_context);
  DCHECK(manager);

  autofill::CreditCard* card = manager->GetCreditCardByGUID(UnsignedToGUID(id));
  if (card)
    manager->UpdateCreditCard(to_Autofill_CreditCard(ewk_credit_card));
  else
    manager->AddCreditCard(to_Autofill_CreditCard(ewk_credit_card));
  return EINA_TRUE;
}

Eina_Bool
EwkContextFormAutofillCreditCardManager::priv_form_autofill_credit_card_add(
    Ewk_Context* ewk_context,
    Ewk_Autofill_CreditCard* card) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(ewk_context, EINA_FALSE);

  autofill::PersonalDataManager* manager =
      PersonalDataManagerForEWKContext(ewk_context);
  DCHECK(manager);

  manager->AddCreditCard(to_Autofill_CreditCard(card));
  return EINA_TRUE;
}

Eina_Bool
EwkContextFormAutofillCreditCardManager::priv_form_autofill_credit_card_remove(
    Ewk_Context* ewk_context,
    unsigned id) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(ewk_context, EINA_FALSE);

  autofill::PersonalDataManager* manager =
      PersonalDataManagerForEWKContext(ewk_context);
  DCHECK(manager);

  manager->RemoveByGUID(UnsignedToGUID(id));
  return EINA_TRUE;
}

/* LCOV_EXCL_START */
void EwkContextFormAutofillCreditCardManager::
    priv_form_autofill_credit_card_changed_callback_set(
        Ewk_Context_Form_Autofill_CreditCard_Changed_Callback callback,
        void* user_data) {
  autofill::PersonalDataManagerFactory* personalDataManagerFactory =
      autofill::PersonalDataManagerFactory::GetInstance();
  personalDataManagerFactory->SetCallback(callback, user_data);
}
/* LCOV_EXCL_STOP */

#endif  // TIZEN_AUTOFILL_SUPPORT
