// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"
#include "utc_blink_ewk_context_form_autofill_credit_card_utils.h"

#include <map>
#include <sstream>

class utc_blink_ewk_context_form_autofill_credit_card_get
    : public utc_blink_ewk_context_form_autofill_credit_card_base {};

TEST_F(utc_blink_ewk_context_form_autofill_credit_card_get, POS_TEST) {
  Ewk_Autofill_CreditCard* card = getTestEwkAutofillCreditCard();
  ASSERT_TRUE(card);
  Eina_Bool result = ewk_context_form_autofill_credit_card_add(
      ewk_context_default_get(), card);
  ewk_autofill_credit_card_delete(card);
  EventLoopWait(3.0);
  ASSERT_TRUE(result);
  card = ewk_context_form_autofill_credit_card_get(
      ewk_context_default_get(), TEST_AUTOFILL_CREDIT_CARD_ID);
  ASSERT_TRUE(card);
  ASSERT_TRUE(checkIfCreditCardContainsTestData(card));
  ewk_autofill_credit_card_delete(card);
}

TEST_F(utc_blink_ewk_context_form_autofill_credit_card_get, NEG_TEST) {
  ASSERT_EQ(NULL, ewk_context_form_autofill_credit_card_get(
                      NULL, TEST_AUTOFILL_CREDIT_CARD_ID));
}
