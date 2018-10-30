// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ewk_autofill_credit_card_internal.h"
#include "utc_blink_ewk_base.h"

class utc_blink_ewk_autofill_credit_card_id_get : public utc_blink_ewk_base {};

TEST_F(utc_blink_ewk_autofill_credit_card_id_get, NullArg) {
  ASSERT_EQ(0, ewk_autofill_credit_card_id_get(NULL));
}

TEST_F(utc_blink_ewk_autofill_credit_card_id_get, POS_TEST) {
  Ewk_Autofill_CreditCard* const m_card0 = ewk_autofill_credit_card_new();
  Ewk_Autofill_CreditCard* const m_card1 = ewk_autofill_credit_card_new();

  EXPECT_NE(ewk_autofill_credit_card_id_get(m_card0),
            ewk_autofill_credit_card_id_get(m_card1));
  EXPECT_NE(0, ewk_autofill_credit_card_id_get(m_card1));

  ewk_autofill_credit_card_delete(m_card1);
  ewk_autofill_credit_card_delete(m_card0);
}
