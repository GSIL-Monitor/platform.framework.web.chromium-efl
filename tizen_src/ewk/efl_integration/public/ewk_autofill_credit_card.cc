/*
 * Copyright (C) 2017 Samsung Electronics. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY SAMSUNG ELECTRONICS. AND ITS CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SAMSUNG ELECTRONICS. OR ITS
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ewk_autofill_credit_card_internal.h"
#include "private/ewk_autofill_credit_card_private.h"
#include "private/ewk_private.h"

/* LCOV_EXCL_START */
Ewk_Autofill_CreditCard* ewk_autofill_credit_card_new()
{
  return new(std::nothrow) Ewk_Autofill_CreditCard();
}

void ewk_autofill_credit_card_delete(Ewk_Autofill_CreditCard* credit_card)
{
  EINA_SAFETY_ON_NULL_RETURN(credit_card);
  delete credit_card;
}

void ewk_autofill_credit_card_data_set(Ewk_Autofill_CreditCard* credit_card,
    Ewk_Autofill_Credit_Card_Data_Type type, const char* value)
{
  EINA_SAFETY_ON_NULL_RETURN(credit_card);
  EINA_SAFETY_ON_NULL_RETURN(value);
  credit_card->setData(CCDataType(type), value);
}

unsigned ewk_autofill_credit_card_id_get(Ewk_Autofill_CreditCard* credit_card)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(credit_card, 0);
  return credit_card->getCreditCardID();
}

Eina_Stringshare* ewk_autofill_credit_card_data_get(Ewk_Autofill_CreditCard* credit_card,
    Ewk_Autofill_Credit_Card_Data_Type type)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(credit_card, 0);
  std::string retVal = credit_card->getData(CCDataType(type));
  return (retVal.empty()) ?
          NULL : eina_stringshare_add(retVal.c_str());
}
/* LCOV_EXCL_STOP */
