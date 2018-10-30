// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"
#include "utc_blink_ewk_context_form_autofill_credit_card_utils.h"

#include <map>

class utc_blink_ewk_context_form_autofill_credit_card_add
    : public utc_blink_ewk_context_form_autofill_credit_card_base {};

TEST_F(utc_blink_ewk_context_form_autofill_credit_card_add, POS_TEST) {
  Ewk_Autofill_CreditCard* card = getTestEwkAutofillCreditCard();
  ASSERT_TRUE(card);
  Eina_Bool result = ewk_context_form_autofill_credit_card_add(
      ewk_context_default_get(), card);
  ASSERT_TRUE(result);
  ewk_autofill_credit_card_delete(card);
}

TEST_F(utc_blink_ewk_context_form_autofill_credit_card_add, NEG_TEST) {
  Ewk_Autofill_CreditCard* card = getTestEwkAutofillCreditCard();
  Ewk_Context* context = ewk_context_default_get();
  ASSERT_TRUE(card);
  ASSERT_TRUE(context);
  ASSERT_EQ(EINA_FALSE, ewk_context_form_autofill_credit_card_add(NULL, card));
  ASSERT_EQ(EINA_FALSE,
            ewk_context_form_autofill_credit_card_add(context, NULL));
  ewk_autofill_credit_card_delete(card);
}
