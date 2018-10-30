// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if defined(S_TERRACE_SUPPORT)

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_WEBDATA_AUTOFILL_WEBDATA_SERVICE_TERRACE_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_WEBDATA_AUTOFILL_WEBDATA_SERVICE_TERRACE_H_

#include "components/autofill/core/browser/webdata/autofill_webdata_service.h"

#include "base/callback.h"

namespace autofill {

class AutofillWebDataServiceDelegate;

class AutofillWebDataServiceTerrace : public AutofillWebDataService {
 public:
  AutofillWebDataServiceTerrace(
      const scoped_refptr<WebDatabaseService>& wdbs,
      const scoped_refptr<base::SingleThreadTaskRunner>& ui_thread,
      const scoped_refptr<base::SingleThreadTaskRunner>& db_thread,
      const ProfileErrorCallback& callback,
      std::unique_ptr<AutofillWebDataServiceDelegate> delegate);

  void RemoveAutofillDataModifiedBetween(const base::Time& delete_begin,
                                         const base::Time& delete_end) override;

  // Credit cards
  void AddCreditCard(const CreditCard& credit_card) override;
  void UpdateCreditCard(const CreditCard& credit_card) override;
  void RemoveCreditCard(const std::string& guid) override;
  WebDataServiceBase::Handle GetCreditCards(
      WebDataServiceConsumer* consumer) override;

  // Upi data
  void AddUpiData(const UPIName& upi_data) override;
  void UpdateUpiData(const UPIName& upi_data) override;
  void RemoveUpiData(const std::string& guid) override;
  WebDataServiceBase::Handle GetUpiData(
      WebDataServiceConsumer* consumer) override;

  // Credit card infobar only
  void SetCreditCardStatus(const CreditCard& credit_card,
                           uint64_t card_status,
                           uint64_t use_count,
                           uint64_t last_use_time);
  void GetCreditCardStatusTable(
      const base::Callback<void(
          const std::map<uint32_t, std::tuple<uint64_t, uint64_t, uint64_t>>&)>&
          callback);

 private:
  ~AutofillWebDataServiceTerrace() override;

  WebDatabase::State RemoveAutofillDataModifiedBetweenImpl(
      const base::Time& delete_begin,
      const base::Time& delete_end,
      WebDatabase* unused_db);

  using self = AutofillWebDataServiceTerrace;

  WebDatabase::State AddCreditCardImpl(const CreditCard& credit_card,
                                       WebDatabase* unused_db);
  WebDatabase::State UpdateCreditCardImpl(const CreditCard& credit_card,
                                          WebDatabase* unused_db);
  WebDatabase::State RemoveCreditCardImpl(const std::string& guid,
                                          WebDatabase* unused_db);
  WebDatabase::State AddUpiDataImpl(const UPIName& upi_data,
                                    WebDatabase* unused_db);
  WebDatabase::State UpdateUpiDataImpl(const UPIName& upi_data,
                                       WebDatabase* unused_db);
  WebDatabase::State RemoveUpiDataImpl(const std::string& guid,
                                       WebDatabase* unused_db);
  std::unique_ptr<WDTypedResult> GetCreditCardsImpl(WebDatabase* unused_db);
  std::unique_ptr<WDTypedResult> GetUpiDataImpl(WebDatabase* unused_db);
  void DestroyAutofillCreditCardResult(const WDTypedResult* result);

  void SetCreditCardStatusOnDBThread(const CreditCard& credit_card,
                                     uint64_t card_status,
                                     uint64_t use_count,
                                     uint64_t last_use_time);
  std::map<uint32_t, std::tuple<uint64_t, uint64_t, uint64_t>>
  GetCreditCardStatusTableOnDBThread();

  scoped_refptr<base::SingleThreadTaskRunner> db_thread_;
  scoped_refptr<base::SingleThreadTaskRunner> ui_thread_;
  std::unique_ptr<AutofillWebDataServiceDelegate> delegate_;

  DISALLOW_COPY_AND_ASSIGN(AutofillWebDataServiceTerrace);
};

}  // namespace autofill

#endif

#endif
