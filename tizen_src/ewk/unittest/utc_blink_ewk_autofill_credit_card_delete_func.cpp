// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

#include "ewk_autofill_credit_card_internal.h"

class utc_blink_ewk_autofill_credit_card_delete : public utc_blink_ewk_base {};

/* No way to check if delete function worked without access to
ewk_autofill_credit_card_private.h
*/

TEST_F(utc_blink_ewk_autofill_credit_card_delete, POS_TEST) {
  Ewk_Autofill_CreditCard* mCreditCard = ewk_autofill_credit_card_new();
  ewk_autofill_credit_card_delete(mCreditCard);
  utc_pass();
}
