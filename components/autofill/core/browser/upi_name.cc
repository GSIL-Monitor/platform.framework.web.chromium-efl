// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/upi_name.h"

#include "base/guid.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/browser/autofill_type.h"
#include "components/autofill/core/browser/validation.h"
#include "components/autofill/core/common/autofill_regexes.h"

using base::ASCIIToUTF16;

namespace autofill {

UPIName::UPIName(const std::string& guid, const std::string& origin)
    : AutofillDataModel(guid, origin) {}

UPIName::UPIName() : UPIName(base::GenerateGUID(), std::string()) {}
UPIName::~UPIName() {}

base::string16 UPIName::GetRawInfo(ServerFieldType type) const {
  DCHECK_EQ(UPI_DATA, AutofillType(type).group());
  switch (type) {
    case UPI_FIELD:
      return upi_name_;

    default:
      // ComputeDataPresentForArray will hit this repeatedly.
      return base::string16();
  }
}

bool UPIName::IsValid(const base::string16& value) const {
  return IsValidUpiName(value);
}

bool UPIName::IsPingpayID() const {
  return MatchesPattern(upi_name_, base::ASCIIToUTF16("@pingpay$"));
}

void UPIName::SetRawInfo(ServerFieldType type, const base::string16& value) {
  DCHECK_EQ(UPI_DATA, AutofillType(type).group());
  switch (type) {
    case UPI_FIELD:
      upi_name_ = value;
      break;

    default:
      NOTREACHED() << "Attempting to set unknown info-type " << type;
      break;
  }
}

void UPIName::SetUPIName(const base::string16& upi_name) {
  upi_name_ = upi_name;
}

int UPIName::Compare(const UPIName& upi_name) const {
  // The following UPIName field types are the only types we store in the
  // WebDB so far, so we're only concerned with matching these types in the
  // upi
  const ServerFieldType types[] = {UPI_FIELD};
  for (ServerFieldType type : types) {
    int comparison = GetRawInfo(type).compare(upi_name.GetRawInfo(type));
    if (comparison != 0)
      return comparison;
  }

  int comparison = billing_address_id_.compare(upi_name.billing_address_id_);
  if (comparison != 0)
    return comparison;

  return 0;
}

bool UPIName::IsEmpty(const std::string& app_locale) const {
  ServerFieldTypeSet types;
  GetNonEmptyTypes(app_locale, &types);
  return types.empty();
}

void UPIName::GetSupportedTypes(ServerFieldTypeSet* supported_types) const {
  supported_types->insert(UPI_FIELD);
}

}  // namespace autofill
