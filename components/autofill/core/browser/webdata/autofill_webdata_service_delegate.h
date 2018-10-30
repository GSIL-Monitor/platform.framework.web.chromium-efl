// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if defined(S_TERRACE_SUPPORT)

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_WEBDATA_AUTOFILL_WEBDATA_SERVICE_DELEGATE_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_WEBDATA_AUTOFILL_WEBDATA_SERVICE_DELEGATE_H_

#include "components/autofill/core/browser/webdata/autofill_webdata_service.h"

namespace autofill {

class AutofillWebDataServiceDelegate {
 public:
  virtual ~AutofillWebDataServiceDelegate() {}

  virtual void RemoveAutofillDataModifiedBetween(
      const base::Time& delete_begin,
      const base::Time& delete_end,
      base::SingleThreadTaskRunner*) = 0;
  virtual void AddCreditCard(const CreditCard& credit_card) = 0;
  virtual void UpdateCreditCard(const CreditCard& credit_card) = 0;
  virtual void RemoveCreditCard(const std::string& guid,
                                base::SingleThreadTaskRunner*) = 0;
  virtual void GetCreditCards(std::vector<CreditCard*>* credit_cards) = 0;

  virtual void SetCreditCardStatus(const CreditCard& credit_card,
                                   uint64_t card_status,
                                   uint64_t use_count,
                                   uint64_t last_use_time) = 0;
  virtual std::map<uint32_t, std::tuple<uint64_t, uint64_t, uint64_t>>
  GetCreditCardStatusTable() = 0;

  // UPI Name
  virtual void AddUPIName(const UPIName& upi_name) = 0;
  virtual void UpdateUPIName(const UPIName& upi_name) = 0;
  virtual void RemoveUPIName(const std::string& guid) = 0;
  virtual void GetUpiData(std::vector<UPIName*>* upi_name) = 0;
};

}  // namespace autofill

#endif

#endif
