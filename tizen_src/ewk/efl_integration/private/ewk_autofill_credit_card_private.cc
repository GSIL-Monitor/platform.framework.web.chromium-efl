// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <sstream>

#include "ewk_autofill_credit_card_private.h"

namespace {
unsigned s_currentID_ = 0;
}

_Ewk_Autofill_CreditCard::_Ewk_Autofill_CreditCard() {
  m_creditCardID_ = s_currentID_;
  ++s_currentID_;
  m_data[CREDIT_CARD_ID] = std::to_string(m_creditCardID_);
}

_Ewk_Autofill_CreditCard::~_Ewk_Autofill_CreditCard() {}

unsigned _Ewk_Autofill_CreditCard::getCreditCardID() const {
  return m_creditCardID_;
}

void _Ewk_Autofill_CreditCard::setCreditCardID(unsigned id) {
  m_creditCardID_ = id;
  m_data[CREDIT_CARD_ID] = std::to_string(m_creditCardID_);
}

std::string _Ewk_Autofill_CreditCard::getData(CCDataType name) const {
  AutofillDataMap::const_iterator it = m_data.find(name);
  if (it != m_data.end())
    return (*it).second;
  return "";
}

base::string16 _Ewk_Autofill_CreditCard::get16Data(CCDataType name) const {
  return base::string16(base::ASCIIToUTF16(getData(name)));
}

void _Ewk_Autofill_CreditCard::setData(CCDataType name,
                                       const std::string& value) {
  if (CREDIT_CARD_ID == name) {
    std::stringstream stream;
    stream << value;
    stream >> m_creditCardID_;
  }
  m_data[name] = value;
}
