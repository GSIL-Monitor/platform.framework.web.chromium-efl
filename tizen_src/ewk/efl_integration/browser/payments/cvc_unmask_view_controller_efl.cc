// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/payments/cvc_unmask_view_controller_efl.h"

#include <memory>

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "browser/payments/payment_request_dialog_efl.h"
#include "components/autofill/core/browser/autofill_client.h"
#include "components/autofill/core/browser/credit_card.h"
#include "components/autofill/core/browser/payments/full_card_request.h"
#include "components/autofill/core/browser/validation.h"
#include "components/autofill/core/common/autofill_clock.h"
#include "components/grit/components_scaled_resources.h"
#include "components/payments/content/payment_request_state.h"
#include "components/payments/core/payment_request_delegate.h"
#include "components/signin/core/browser/profile_identity_provider.h"
#include "components/signin/core/browser/signin_manager.h"
#include "components/strings/grit/components_strings.h"
#include "components/vector_icons/vector_icons.h"
#include "content/public/browser/web_contents.h"
#include "tizen_src/ewk/efl_integration/browser_context_efl.h"

namespace payments {

CvcUnmaskViewControllerEfl::CvcUnmaskViewControllerEfl(
    PaymentRequestState* state,
    PaymentRequestDialogEfl* dialog,
    const autofill::CreditCard& credit_card,
    base::WeakPtr<autofill::payments::FullCardRequest::ResultDelegate>
        result_delegate,
    content::WebContents* web_contents)
    : credit_card_(credit_card),
      web_contents_(web_contents),
      dialog_(dialog),
      payments_client_((static_cast<content::BrowserContextEfl*>(
                            web_contents_->GetBrowserContext()))
                           ->GetRequestContextEfl(),
                       this),
      full_card_request_(nullptr),
      weak_ptr_factory_(this) {
#if defined(TIZEN_AUTOFILL_SUPPORT)
  full_card_request_ = new autofill::payments::FullCardRequest(
      this, &payments_client_, state->GetPersonalDataManager());
  full_card_request_->GetFullCard(
      credit_card,
      autofill::AutofillClient::UnmaskCardReason::UNMASK_FOR_PAYMENT_REQUEST,
      result_delegate, weak_ptr_factory_.GetWeakPtr());
#endif
}

CvcUnmaskViewControllerEfl::~CvcUnmaskViewControllerEfl() {
  if (full_card_request_)
    delete full_card_request_;
}

IdentityProvider* CvcUnmaskViewControllerEfl::GetIdentityProvider() {
  return NULL;
}

void CvcUnmaskViewControllerEfl::OnDidGetRealPan(
    autofill::AutofillClient::PaymentsRpcResult result,
    const std::string& real_pan) {
#if defined(TIZEN_AUTOFILL_SUPPORT)
  if (full_card_request_)
    full_card_request_->OnDidGetRealPan(result, real_pan);
#endif
}

void CvcUnmaskViewControllerEfl::OnDidGetUploadDetails(
    autofill::AutofillClient::PaymentsRpcResult result,
    const base::string16& context_token,
    std::unique_ptr<base::DictionaryValue> legal_message) {
  NOTIMPLEMENTED();
}

void CvcUnmaskViewControllerEfl::OnDidUploadCard(
    autofill::AutofillClient::PaymentsRpcResult result,
    const std::string& server_id) {
  NOTIMPLEMENTED();
}

void CvcUnmaskViewControllerEfl::LoadRiskData(
    const base::Callback<void(const std::string&)>& callback) {
  NOTIMPLEMENTED();
}

void CvcUnmaskViewControllerEfl::ShowUnmaskPrompt(
    const autofill::CreditCard& card,
    autofill::AutofillClient::UnmaskCardReason reason,
    base::WeakPtr<autofill::CardUnmaskDelegate> delegate) {
  unmask_delegate_ = delegate;
  dialog_->ShowCvcInputLayout();
}

void CvcUnmaskViewControllerEfl::OnUnmaskVerificationResult(
    autofill::AutofillClient::PaymentsRpcResult result) {
  switch (result) {
    case autofill::AutofillClient::NONE:
      NOTREACHED();
    case autofill::AutofillClient::SUCCESS:
      // In the success case, don't show any error and don't hide the spinner
      // because the dialog is about to close when the merchant completes the
      // transaction.
      return;
    case autofill::AutofillClient::TRY_AGAIN_FAILURE:
    case autofill::AutofillClient::PERMANENT_FAILURE:
    case autofill::AutofillClient::NETWORK_ERROR:
      NOTIMPLEMENTED();
      break;
  }
}

void CvcUnmaskViewControllerEfl::CvcConfirmed(base::string16 cvc_text) {
  base::string16& cvc = cvc_text;
  if (unmask_delegate_) {
    autofill::CardUnmaskDelegate::UnmaskResponse response;
    response.cvc = cvc;
    if (credit_card_.ShouldUpdateExpiration(autofill::AutofillClock::Now())) {
      response.exp_month =
          credit_card_.GetRawInfo(autofill::CREDIT_CARD_EXP_MONTH);
      response.exp_year =
          credit_card_.GetRawInfo(autofill::CREDIT_CARD_EXP_4_DIGIT_YEAR);
    }
    unmask_delegate_->OnUnmaskResponse(response);
  }
}

}  // namespace payments
