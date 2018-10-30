// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_AUTOFILL_CREDIT_CARD_PRIVATE_H_
#define EWK_AUTOFILL_CREDIT_CARD_PRIVATE_H_

#include <map>
#include <string>

#include "base/strings/utf_string_conversions.h"

enum CCDataType {
  CREDIT_CARD_ID = 0,
  CREDIT_CARD_NAME_FULL,
  CREDIT_CARD_NUMBER,
  CREDIT_CARD_EXP_MONTH,
  CREDIT_CARD_EXP_4_DIGIT_YEAR,
  CREDIT_CARD_MAX_AUTOFILL
};

struct _Ewk_Autofill_CreditCard {
 public:
  _Ewk_Autofill_CreditCard();
  ~_Ewk_Autofill_CreditCard();

  typedef std::map<CCDataType, std::string> AutofillDataMap;

  unsigned getCreditCardID() const;
  void setCreditCardID(unsigned id);
  std::string getData(CCDataType name) const;
  base::string16 get16Data(CCDataType name) const;

  void setData(CCDataType name, const std::string& value);

 private:
  unsigned m_creditCardID_;
  AutofillDataMap m_data;
};

#endif /*EWK_AUTOFILL_CREDIT_CARD_PRIVATE_H_*/
