// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"
#include "utc_blink_ewk_context_form_autofill_credit_card_utils.h"

#include <map>
#include <sstream>

class utc_blink_ewk_context_form_autofill_credit_card_set
    : public utc_blink_ewk_context_form_autofill_credit_card_base {};

TEST_F(utc_blink_ewk_context_form_autofill_credit_card_set, POS_TEST) {
  Ewk_Autofill_CreditCard* card = getTestEwkAutofillCreditCard();
  ASSERT_TRUE(card);
  Eina_Bool result = ewk_context_form_autofill_credit_card_add(
      ewk_context_default_get(), card);
  ewk_autofill_credit_card_delete(card);
  EventLoopWait(3.0);
  ASSERT_TRUE(result);
  card = getTestEwkAutofillCreditCard();
  ASSERT_TRUE(card);
  // change some property
  const char* new_card_name = "anotherName";
  ewk_autofill_credit_card_data_set(card, EWK_CREDIT_CARD_NAME_FULL,
                                    new_card_name);
  // set it
  ASSERT_EQ(EINA_TRUE,
            ewk_context_form_autofill_credit_card_set(
                ewk_context_default_get(), TEST_AUTOFILL_CREDIT_CARD_ID, card));
  ewk_autofill_credit_card_delete(card);
  // get it and check
  EventLoopWait(3.0);
  card = ewk_context_form_autofill_credit_card_get(
      ewk_context_default_get(), TEST_AUTOFILL_CREDIT_CARD_ID);
  ASSERT_TRUE(card);
  EXPECT_TRUE(checkOne(card, new_card_name, EWK_CREDIT_CARD_FULL_NAME, "name"));

  ewk_autofill_credit_card_delete(card);
}

TEST_F(utc_blink_ewk_context_form_autofill_credit_card_set, NEG_TEST) {
  Ewk_Autofill_CreditCard* card = getTestEwkAutofillCreditCard();
  Ewk_Context* context = ewk_context_default_get();
  ASSERT_TRUE(card);
  ASSERT_TRUE(context);
  ASSERT_EQ(NULL, ewk_context_form_autofill_credit_card_set(
                      NULL, TEST_AUTOFILL_CREDIT_CARD_ID, card));
  ASSERT_EQ(NULL, ewk_context_form_autofill_credit_card_set(
                      context, TEST_AUTOFILL_CREDIT_CARD_ID, NULL));
  ewk_autofill_credit_card_delete(card);
}
