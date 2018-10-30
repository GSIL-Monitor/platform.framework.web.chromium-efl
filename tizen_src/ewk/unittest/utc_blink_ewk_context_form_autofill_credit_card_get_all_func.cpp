// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"
#include "utc_blink_ewk_context_form_autofill_credit_card_utils.h"

#include <map>
#include <sstream>

class utc_blink_ewk_context_form_autofill_credit_card_get_all
    : public utc_blink_ewk_context_form_autofill_credit_card_base {};

TEST_F(utc_blink_ewk_context_form_autofill_credit_card_get_all, POS_TEST) {
  Ewk_Autofill_CreditCard* card = getTestEwkAutofillCreditCard();
  ASSERT_TRUE(card);
  Eina_Bool result = ewk_context_form_autofill_credit_card_add(
      ewk_context_default_get(), card);
  ewk_autofill_credit_card_delete(card);
  EventLoopWait(3.0);
  ASSERT_TRUE(result);
  Eina_List* cardsList =
      ewk_context_form_autofill_credit_card_get_all(ewk_context_default_get());
  ASSERT_TRUE(cardsList);
  Eina_List* l;
  void* data;
  card = NULL;
  EINA_LIST_FOREACH(cardsList, l, data) {
    Ewk_Autofill_CreditCard* tmp_card =
        static_cast<Ewk_Autofill_CreditCard*>(data);
    if (NULL != tmp_card) {
      if (TEST_AUTOFILL_CREDIT_CARD_ID ==
          ewk_autofill_credit_card_id_get(tmp_card)) {
        utc_message("found proper card ID");
        card = tmp_card;
      } else {
        ewk_autofill_credit_card_delete(tmp_card);
      }
    }
  }
  ASSERT_TRUE(card);
  EXPECT_TRUE(checkIfCreditCardContainsTestData(card));
  ewk_autofill_credit_card_delete(card);
}

TEST_F(utc_blink_ewk_context_form_autofill_credit_card_get_all, NEG_TEST) {
  ASSERT_EQ(NULL, ewk_context_form_autofill_credit_card_get_all(NULL));
}
