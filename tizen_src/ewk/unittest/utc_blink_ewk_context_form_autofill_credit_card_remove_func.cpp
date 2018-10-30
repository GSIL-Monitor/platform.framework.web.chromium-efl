// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"
#include "utc_blink_ewk_context_form_autofill_credit_card_utils.h"

#include <map>
#include <sstream>

class utc_blink_ewk_context_form_autofill_credit_card_remove
    : public utc_blink_ewk_context_form_autofill_credit_card_base {};

TEST_F(utc_blink_ewk_context_form_autofill_credit_card_remove, POS_TEST) {
  Ewk_Autofill_CreditCard* card = getTestEwkAutofillCreditCard();
  ASSERT_TRUE(card);
  Eina_Bool result = ewk_context_form_autofill_credit_card_add(
      ewk_context_default_get(), card);
  ewk_autofill_credit_card_delete(card);
  ASSERT_TRUE(result);
  EventLoopWait(3.0);
  card = ewk_context_form_autofill_credit_card_get(
      ewk_context_default_get(), TEST_AUTOFILL_CREDIT_CARD_ID);
  ASSERT_TRUE(card);
  ASSERT_EQ(EINA_TRUE,
            ewk_context_form_autofill_credit_card_remove(
                ewk_context_default_get(), TEST_AUTOFILL_CREDIT_CARD_ID));
  ewk_autofill_credit_card_delete(card);
  EventLoopWait(3.0);
  card = ewk_context_form_autofill_credit_card_get(
      ewk_context_default_get(), TEST_AUTOFILL_CREDIT_CARD_ID);
  EXPECT_TRUE(NULL == card);
}

TEST_F(utc_blink_ewk_context_form_autofill_credit_card_remove, NEG_TEST) {
  ASSERT_EQ(EINA_FALSE, ewk_context_form_autofill_credit_card_remove(
                            NULL, TEST_AUTOFILL_CREDIT_CARD_ID));
}
