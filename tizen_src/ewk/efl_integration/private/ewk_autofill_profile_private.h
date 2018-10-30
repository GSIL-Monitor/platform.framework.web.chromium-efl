// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_AUTOFILL_PROFILE_PRIVATE_H_
#define EWK_AUTOFILL_PROFILE_PRIVATE_H_

#include <map>
#include <string>

#include "base/strings/utf_string_conversions.h"
#include "public/ewk_autofill_profile_product.h"

enum DataType {
  PROFILE_ID = 0,
  PROFILE_NAME,
  PROFILE_COMPANY,
  PROFILE_ADDRESS1,
  PROFILE_ADDRESS2,
  PROFILE_CITY_TOWN,
  PROFILE_STATE_PROVINCE_REGION,
  PROFILE_ZIPCODE,
  PROFILE_COUNTRY,
  PROFILE_PHONE,
  PROFILE_EMAIL,
  MAX_AUTOFILL
};

struct _Ewk_Form_Info {
  _Ewk_Form_Info(Ewk_Form_Type form_type,
                 std::string name,
                 std::string password)
      : type_(form_type), id_(name), password_(password) {}
  ~_Ewk_Form_Info(){};

  Ewk_Form_Type GetFormType() const;
  const char* GetFormId() const;
  const char* GetFormPassword() const;
  const char* GetFormUsernameElement() const;
  const char* GetFormPasswordElement() const;
  const char* GetFormActionUrl() const;

  Ewk_Form_Type type_;
  std::string id_;
  std::string password_;
  std::string username_element_;
  std::string password_element_;
  std::string action_url_;
};

class _Ewk_Autofill_Profile {
 public:
  _Ewk_Autofill_Profile();
  ~_Ewk_Autofill_Profile();

  typedef std::map<DataType, std::string> AutofillDataMap;

  unsigned getProfileID() const;
  void setProfileID(unsigned id);
  std::string getData(DataType name) const;
  base::string16 get16Data(DataType name) const;

  void setData(DataType name, const std::string& value);

 private:
  unsigned m_profileID_;
  AutofillDataMap m_data;
};

#endif /*EWK_AUTOFILL_PROFILE_PRIVATE_H_*/
