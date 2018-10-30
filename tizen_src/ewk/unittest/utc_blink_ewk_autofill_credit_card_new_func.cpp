// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

#include "ewk_autofill_credit_card_internal.h"

class utc_blink_ewk_autofill_credit_card_new : public utc_blink_ewk_base {};

TEST_F(utc_blink_ewk_autofill_credit_card_new, POS_TEST) {
  Ewk_Autofill_CreditCard* card = ewk_autofill_credit_card_new();
  ASSERT_TRUE(card);

  ewk_autofill_credit_card_delete(card);
}
