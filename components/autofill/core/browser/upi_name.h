// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_UPI_NAME_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_UPI_NAME_H_

#include "components/autofill/core/browser/autofill_data_model.h"

namespace autofill {
// A form group that stores UPI Name information.
class UPIName : public AutofillDataModel {
 public:
  UPIName(const std::string& guid, const std::string& origin);

  // For use in STL containers.
  UPIName();
  ~UPIName() override;

  base::string16 GetRawInfo(ServerFieldType type) const override;
  void SetRawInfo(ServerFieldType type, const base::string16& value) override;

  bool IsValid(const base::string16& value) const;

  bool IsPingpayID() const;

  int Compare(const UPIName& upi_name) const;

  // Returns true if there are no values (field types) set.
  bool IsEmpty(const std::string& app_locale) const;

  // Returns the UPI Name.
  const base::string16& GetUPIName() const { return upi_name_; }
  // Sets |upi_name_| to |upi_name| and computes the appropriate card |type_|.
  void SetUPIName(const base::string16& upi_name);

  const std::string& billing_address_id() const { return billing_address_id_; }
  void set_billing_address_id(const std::string& id) {
    billing_address_id_ = id;
  }

 private:
  // FormGroup:
  void GetSupportedTypes(ServerFieldTypeSet* supported_types) const override;

  // The cardholder's UPI name. May be empty.
  base::string16 upi_name_;

  // The identifier of the billing address for this upi.
  std::string billing_address_id_;
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_UPI_NAME_H_