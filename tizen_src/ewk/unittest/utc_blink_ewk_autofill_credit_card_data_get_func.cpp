// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "utc_blink_ewk_base.h"

#include "ewk_autofill_credit_card_internal.h"

class utc_blink_ewk_autofill_credit_card_data_get : public utc_blink_ewk_base {
 protected:
  utc_blink_ewk_autofill_credit_card_data_get() : m_cardName("MyCreditCard") {
    m_card = ewk_autofill_credit_card_new();
  }

  ~utc_blink_ewk_autofill_credit_card_data_get() override {
    ewk_autofill_credit_card_delete(m_card);
  }

  const std::string m_cardName;
  Ewk_Autofill_CreditCard* m_card;
};

TEST_F(utc_blink_ewk_autofill_credit_card_data_get, POS_TEST) {
  ASSERT_TRUE(m_card);

  ewk_autofill_credit_card_data_set(m_card, EWK_CREDIT_CARD_NAME,
                                    m_cardName.c_str());

  const std::string result(
      ewk_autofill_credit_card_data_get(m_card, EWK_CREDIT_CARD_NAME));

  ASSERT_TRUE(result.compare(m_cardName) == 0);
}
