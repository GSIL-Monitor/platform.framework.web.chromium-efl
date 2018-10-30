// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_context_form_autofill_credit_card_utils.h"
#include <stdio.h>
#include <iostream>
#include <sstream>
#include "ewk_autofill_credit_card_internal.h"
#include "utc_blink_ewk_base.h"

Ewk_Autofill_CreditCard* getTestEwkAutofillCreditCard() {
  Ewk_Autofill_CreditCard* card = ewk_autofill_credit_card_new();
  if (NULL == card)
    return card;
  std::stringstream stream;
  stream << TEST_AUTOFILL_CREDIT_CARD_ID;
  std::string id = stream.str();
  ewk_autofill_credit_card_data_set(card, EWK_CREDIT_CARD_ID, id.c_str());
  ewk_autofill_credit_card_data_set(card, EWK_CREDIT_CARD_NAME_FULL,
                                    TEST_AUTOFILL_CREDIT_CARD_NAME_FULL);
  ewk_autofill_credit_card_data_set(card, EWK_CREDIT_CARD_NUMBER,
                                    TEST_AUTOFILL_CREDIT_CARD_NUMBER);
  ewk_autofill_credit_card_data_set(card, EWK_CREDIT_CARD_EXP_MONTH,
                                    TEST_AUTOFILL_CREDIT_CARD_EXP_MONTH);
  ewk_autofill_credit_card_data_set(card, EWK_CREDIT_CARD_EXP_4_DIGIT_YEAR,
                                    TEST_AUTOFILL_CREDIT_CARD_EXP_4_DIGIT_YEAR);
  return card;
}

bool checkOne(Ewk_Autofill_CreditCard* cardToCheck,
              const char* reference,
              Ewk_Autofill_CreditCard_Data_Type kind,
              const char* errMessage) {
  const char* ptr = ewk_autofill_credit_card_data_get(cardToCheck, kind);
  if (NULL == ptr) {
    utc_message("%s, expected: %s obtained: NULL\n", errMessage, reference);
    return false;
  } else {
    utc_message("%s, expected: %s obtained: %s\n", errMessage, reference, ptr);
  }
  return (0 == strcmp(reference, ptr));
}

bool checkIfCreditCardContainsTestData(Ewk_Autofill_CreditCard* cardToCheck) {
  if (NULL == cardToCheck) {
    return false;
  }
  bool dataValid = true;
  if (TEST_AUTOFILL_CREDIT_CARD_ID !=
      ewk_autofill_credit_card_id_get(cardToCheck)) {
    utc_message("card ID check failed %i %i", TEST_AUTOFILL_CREDIT_CARD_ID,
                ewk_autofill_credit_card_id_get(cardToCheck));
    dataValid = false;
  }
  dataValid = checkOne(cardToCheck, TEST_AUTOFILL_CREDIT_CARD_NAME_FULL,
                       EWK_CREDIT_CARD_NAME_FULL, "name") &&
              dataValid;
  dataValid = checkOne(cardToCheck, TEST_AUTOFILL_CREDIT_CARD_NUMBER,
                       EWK_CREDIT_CARD_NUMBER, "number") &&
              dataValid;
  dataValid = checkOne(cardToCheck, TEST_AUTOFILL_CREDIT_CARD_EXP_MONTH,
                       EWK_CREDIT_CARD_EXP_MONTH, "month") &&
              dataValid;
  dataValid = checkOne(cardToCheck, TEST_AUTOFILL_CREDIT_CARD_EXP_4_DIGIT_YEAR,
                       EWK_CREDIT_CARD_EXP_4_DIGIT_YEAR, "year") &&
              dataValid;
  return dataValid;
}

void utc_blink_ewk_context_form_autofill_credit_card_base::PostSetUp() {
  EventLoopWait(3.0);
  RemoveTestCreditCard();
}

void utc_blink_ewk_context_form_autofill_credit_card_base::PreTearDown() {
  RemoveTestCreditCard();
}
