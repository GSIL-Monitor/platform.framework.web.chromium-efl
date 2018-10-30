// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UTC_BLINK_EWK_CONTEXT_FORM_AUTOFILL_CREDIT_CARD_UTILS_H
#define UTC_BLINK_EWK_CONTEXT_FORM_AUTOFILL_CREDIT_CARD_UTILS_H

#include "utc_blink_ewk_base.h"

const unsigned TEST_AUTOFILL_CREDIT_CARD_ID = 12345;
const char* const TEST_AUTOFILL_CREDIT_CARD_NAME_FULL = "Mr. Smith";
const char* const TEST_AUTOFILL_CREDIT_CARD_NUMBER = "411111111111111";
const char* const TEST_AUTOFILL_CREDIT_CARD_EXP_4_DIGIT_YEAR = "2018";
const char* const TEST_AUTOFILL_CREDIT_CARD_EXP_MONTH = "01";

Ewk_Autofill_CreditCard* getTestEwkAutofillCreditCard();

bool checkOne(Ewk_Autofill_CreditCard* cardToCheck,
              const char* reference,
              Ewk_Autofill_CreditCard_Data_Type kind,
              const char* errMessage);
bool checkIfCreditCardContainsTestData(Ewk_Autofill_CreditCard* cardToCheck);

class utc_blink_ewk_context_form_autofill_credit_card_base
    : public utc_blink_ewk_base {
 protected:
  void PostSetUp() override;

  void PreTearDown() override;

 private:
  void RemoveTestCreditCard() {
    Ewk_Autofill_CreditCard* card = ewk_context_form_autofill_credit_card_get(
        ewk_context_default_get(), TEST_AUTOFILL_CREDIT_CARD_ID);
    if (card) {
      ewk_context_form_autofill_credit_card_remove(
          ewk_context_default_get(), TEST_AUTOFILL_CREDIT_CARD_ID);
      ewk_autofill_credit_card_delete(card);
      EventLoopWait(3.0);
      card = ewk_context_form_autofill_credit_card_get(
          ewk_context_default_get(), TEST_AUTOFILL_CREDIT_CARD_ID);
      ASSERT_TRUE(card);
    }
  }
};

#endif  // UTC_BLINK_EWK_CONTEXT_FORM_AUTOFILL_CREDIT_CARD_UTILS_H
