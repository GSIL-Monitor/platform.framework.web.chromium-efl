// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if defined(S_TERRACE_SUPPORT)

#include "components/autofill/core/browser/webdata/autofill_webdata_service_terrace.h"

#include "base/bind.h"
#include "base/stl_util.h"
#include "base/task_runner_util.h"
#include "components/autofill/core/browser/credit_card.h"
#include "components/autofill/core/browser/upi_name.h"
#include "components/autofill/core/browser/webdata/autofill_webdata_backend_impl.h"
#include "components/autofill/core/browser/webdata/autofill_webdata_service_delegate.h"
#include "components/webdata/common/web_data_results.h"
#include "components/webdata/common/web_database_service.h"

namespace autofill {

AutofillWebDataServiceTerrace::AutofillWebDataServiceTerrace(
    const scoped_refptr<WebDatabaseService>& wdbs,
    const scoped_refptr<base::SingleThreadTaskRunner>& ui_thread,
    const scoped_refptr<base::SingleThreadTaskRunner>& db_thread,
    const ProfileErrorCallback& callback,
    std::unique_ptr<AutofillWebDataServiceDelegate> delegate)
    : AutofillWebDataService(std::move(wdbs), ui_thread, db_thread, callback),
      db_thread_(db_thread),
      ui_thread_(ui_thread),
      delegate_(std::move(delegate)) {}

AutofillWebDataServiceTerrace::~AutofillWebDataServiceTerrace() {}

void AutofillWebDataServiceTerrace::RemoveAutofillDataModifiedBetween(
    const base::Time& delete_begin,
    const base::Time& delete_end) {
  AutofillWebDataService::RemoveAutofillDataModifiedBetween(
      delete_begin, delete_end);
  wdbs_->ScheduleDBTask(FROM_HERE, base::Bind(
      &AutofillWebDataServiceTerrace::RemoveAutofillDataModifiedBetweenImpl,
      this, delete_begin, delete_end));
}

void AutofillWebDataServiceTerrace::AddCreditCard(
    const CreditCard& credit_card) {
  wdbs_->ScheduleDBTask(
      FROM_HERE, base::Bind(&AutofillWebDataServiceTerrace::AddCreditCardImpl,
                            this, credit_card));
}

void AutofillWebDataServiceTerrace::AddUpiData(const UPIName& upi_data) {
  wdbs_->ScheduleDBTask(
      FROM_HERE, base::Bind(&AutofillWebDataServiceTerrace::AddUpiDataImpl,
                            this, upi_data));
}

void AutofillWebDataServiceTerrace::UpdateCreditCard(
    const CreditCard& credit_card) {
  wdbs_->ScheduleDBTask(
      FROM_HERE,
      base::Bind(&AutofillWebDataServiceTerrace::UpdateCreditCardImpl,
                 this, credit_card));
}

void AutofillWebDataServiceTerrace::UpdateUpiData(const UPIName& upi_data) {
  wdbs_->ScheduleDBTask(
      FROM_HERE, base::Bind(&AutofillWebDataServiceTerrace::UpdateUpiDataImpl,
                            this, upi_data));
}

void AutofillWebDataServiceTerrace::RemoveCreditCard(const std::string& guid) {
  wdbs_->ScheduleDBTask(
      FROM_HERE,
      base::Bind(&AutofillWebDataServiceTerrace::RemoveCreditCardImpl,
                 this, guid));
}

void AutofillWebDataServiceTerrace::RemoveUpiData(const std::string& guid) {
  wdbs_->ScheduleDBTask(
      FROM_HERE, base::Bind(&AutofillWebDataServiceTerrace::RemoveUpiDataImpl,
                            this, guid));
}

WebDataServiceBase::Handle AutofillWebDataServiceTerrace::GetCreditCards(
    WebDataServiceConsumer* consumer) {
  return wdbs_->ScheduleDBTaskWithResult(
      FROM_HERE,
      base::Bind(&AutofillWebDataServiceTerrace::GetCreditCardsImpl, this),
      consumer);
}

WebDataServiceBase::Handle AutofillWebDataServiceTerrace::GetUpiData(
    WebDataServiceConsumer* consumer) {
  return wdbs_->ScheduleDBTaskWithResult(
      FROM_HERE,
      base::Bind(&AutofillWebDataServiceTerrace::GetUpiDataImpl, this),
      consumer);
}

void AutofillWebDataServiceTerrace::SetCreditCardStatus(
    const CreditCard& credit_card,
    uint64_t card_status,
    uint64_t use_count,
    uint64_t last_use_time) {
  db_thread_->PostTask(
      FROM_HERE,
      base::Bind(&self::SetCreditCardStatusOnDBThread, this, credit_card,
                 card_status, use_count, last_use_time));
}

void AutofillWebDataServiceTerrace::GetCreditCardStatusTable(
    const base::Callback<void(
        const std::map<uint32_t, std::tuple<uint64_t, uint64_t, uint64_t>>&)>&
        callback) {
  base::PostTaskAndReplyWithResult(
      db_thread_.get(), FROM_HERE,
      base::Bind(&self::GetCreditCardStatusTableOnDBThread, this), callback);
}

void AutofillWebDataServiceTerrace::SetCreditCardStatusOnDBThread(
    const CreditCard& credit_card,
    uint64_t card_status,
    uint64_t use_count,
    uint64_t last_use_time) {
  DCHECK(delegate_);
  delegate_->SetCreditCardStatus(credit_card, card_status, use_count,
                                 last_use_time);
}

std::map<uint32_t, std::tuple<uint64_t, uint64_t, uint64_t>>
AutofillWebDataServiceTerrace::GetCreditCardStatusTableOnDBThread() {
  DCHECK(delegate_);
  return delegate_->GetCreditCardStatusTable();
}

WebDatabase::State AutofillWebDataServiceTerrace::AddCreditCardImpl(
    const CreditCard& credit_card,
    WebDatabase* unused_db) {
  DCHECK(delegate_);
  delegate_->AddCreditCard(credit_card);

  // All credit card operations are delegated to AutofilLWebDataServiceDelegate.
  // The delegate should take responsibility for DB transaction and commit.
  // So, we just return COMMIT_NOT_NEEDED here because there is no change in
  // native side DB.
  return WebDatabase::COMMIT_NOT_NEEDED;
}

WebDatabase::State AutofillWebDataServiceTerrace::UpdateCreditCardImpl(
    const CreditCard& credit_card,
    WebDatabase* unused_db) {
  DCHECK(delegate_);
  delegate_->UpdateCreditCard(credit_card);

  // All credit card operations are delegated to AutofilLWebDataServiceDelegate.
  // The delegate should take responsibility for DB transaction and commit.
  // So, we just return COMMIT_NOT_NEEDED here because there is no change in
  // native side DB.
  return WebDatabase::COMMIT_NOT_NEEDED;
}

WebDatabase::State AutofillWebDataServiceTerrace::RemoveCreditCardImpl(
    const std::string& guid,
    WebDatabase* unused_db) {
  DCHECK(delegate_);
  delegate_->RemoveCreditCard(guid, ui_thread_.get());

  // All credit card operations are delegated to AutofilLWebDataServiceDelegate.
  // The delegate should take responsibility for DB transaction and commit.
  // So, we just return COMMIT_NOT_NEEDED here because there is no change in
  // native side DB.
  return WebDatabase::COMMIT_NOT_NEEDED;
}

WebDatabase::State AutofillWebDataServiceTerrace::AddUpiDataImpl(
    const UPIName& upi_data,
    WebDatabase* unused_db) {
  DCHECK(delegate_);
  delegate_->AddUPIName(upi_data);

  // All upi data operations are delegated to AutofilLWebDataServiceDelegate.
  // The delegate should take responsibility for DB transaction and commit.
  // So, we just return COMMIT_NOT_NEEDED here because there is no change in
  // native side DB.
  return WebDatabase::COMMIT_NOT_NEEDED;
}

WebDatabase::State AutofillWebDataServiceTerrace::UpdateUpiDataImpl(
    const UPIName& upi_data,
    WebDatabase* unused_db) {
  DCHECK(delegate_);
  delegate_->UpdateUPIName(upi_data);

  // All upi data operations are delegated to AutofilLWebDataServiceDelegate.
  // The delegate should take responsibility for DB transaction and commit.
  // So, we just return COMMIT_NOT_NEEDED here because there is no change in
  // native side DB.
  return WebDatabase::COMMIT_NOT_NEEDED;
}

WebDatabase::State AutofillWebDataServiceTerrace::RemoveUpiDataImpl(
    const std::string& guid,
    WebDatabase* unused_db) {
  DCHECK(delegate_);
  delegate_->RemoveUPIName(guid);

  // All upi data operations are delegated to AutofilLWebDataServiceDelegate.
  // The delegate should take responsibility for DB transaction and commit.
  // So, we just return COMMIT_NOT_NEEDED here because there is no change in
  // native side DB.
  return WebDatabase::COMMIT_NOT_NEEDED;
}

std::unique_ptr<WDTypedResult>
AutofillWebDataServiceTerrace::GetCreditCardsImpl(WebDatabase* unused_db) {
  DCHECK(delegate_);
  std::vector<CreditCard*> credit_cards;
  delegate_->GetCreditCards(&credit_cards);
  return std::unique_ptr<WDTypedResult>(
      new WDResult<std::vector<CreditCard*>>(
          AUTOFILL_CREDITCARDS_RESULT, credit_cards));
}

std::unique_ptr<WDTypedResult> AutofillWebDataServiceTerrace::GetUpiDataImpl(
    WebDatabase* unused_db) {
  DCHECK(delegate_);
  std::vector<UPIName*> upi_data;
  delegate_->GetUpiData(&upi_data);
  return std::unique_ptr<WDTypedResult>(
      new WDResult<std::vector<UPIName*>>(AUTOFILL_UPIDATA_RESULT, upi_data));
}

WebDatabase::State
AutofillWebDataServiceTerrace::RemoveAutofillDataModifiedBetweenImpl(
    const base::Time& delete_begin,
    const base::Time& delete_end,
    WebDatabase* unused_db) {
  DCHECK(delegate_);
  delegate_->RemoveAutofillDataModifiedBetween(delete_begin, delete_end,
                                               ui_thread_.get());

  // All credit card operations are delegated to AutofilLWebDataServiceDelegate.
  // The delegate should take responsibility for DB transaction and commit.
  // So, we just return COMMIT_NOT_NEEDED here because there is no change in
  // native side DB.
  return WebDatabase::COMMIT_NOT_NEEDED;
}

}  // namespace autofill

#endif
