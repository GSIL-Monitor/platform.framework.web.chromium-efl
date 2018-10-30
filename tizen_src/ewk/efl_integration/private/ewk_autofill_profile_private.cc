// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <sstream>

#include "ewk_autofill_profile_private.h"

namespace {
unsigned s_currentID_ = 0;
}

/* LCOV_EXCL_START */
_Ewk_Autofill_Profile::_Ewk_Autofill_Profile() {
  m_profileID_ = s_currentID_;
  ++s_currentID_;
  m_data[PROFILE_ID] = std::to_string(m_profileID_);
}

_Ewk_Autofill_Profile::~_Ewk_Autofill_Profile() {
}

unsigned _Ewk_Autofill_Profile::getProfileID() const {
  return m_profileID_;
}

void _Ewk_Autofill_Profile::setProfileID(unsigned id) {
  m_profileID_ = id;
  m_data[PROFILE_ID] = std::to_string(m_profileID_);
}

std::string _Ewk_Autofill_Profile::getData(
    DataType name) const {
  AutofillDataMap::const_iterator it = m_data.find(name);
  if (it != m_data.end())
    return (*it).second;
  return "";
}

base::string16 _Ewk_Autofill_Profile::get16Data(
    DataType name) const {
  return base::string16(base::ASCIIToUTF16(getData(name)));
}

void _Ewk_Autofill_Profile::setData(DataType name,
    const std::string& value) {
  if (PROFILE_ID == name) {
    std::stringstream stream;
    stream << value;
    stream >> m_profileID_;
  }
  m_data[name] = value;
}
#if defined(OS_TIZEN_TV_PRODUCT)
Ewk_Form_Type _Ewk_Form_Info::GetFormType() const {
  return type_;
}

const char* _Ewk_Form_Info::GetFormId() const {
  return id_.c_str();
}

const char* _Ewk_Form_Info::GetFormPassword() const {
  return password_.c_str();
}

const char* _Ewk_Form_Info::GetFormUsernameElement() const {
  return username_element_.c_str();
}

const char* _Ewk_Form_Info::GetFormPasswordElement() const {
  return password_element_.c_str();
}

const char* _Ewk_Form_Info::GetFormActionUrl() const {
  return action_url_.c_str();
}
#endif
/* LCOV_EXCL_STOP */
