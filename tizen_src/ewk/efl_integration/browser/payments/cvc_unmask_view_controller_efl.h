// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CVC_UNMASK_VIEW_CONTROLLER_EFL_H_
#define CVC_UNMASK_VIEW_CONTROLLER_EFL_H_

#include <Elementary.h>
#include <Evas.h>
#include <ecore-1/Ecore.h>
#include <string>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/observer_list.h"
#include "base/strings/string16.h"
#include "components/autofill/core/browser/payments/full_card_request.h"
#include "components/autofill/core/browser/payments/payments_client.h"
#include "components/autofill/core/browser/risk_data_loader.h"

namespace autofill {
class AutofillClient;
}

namespace content {
class BrowserContextEfl;
class WebContents;
}  // namespace content

namespace payments {

class PaymentRequestState;
class PaymentRequestDialogEfl;

class CvcUnmaskViewControllerEfl
    : public autofill::RiskDataLoader,
      public autofill::payments::PaymentsClientDelegate,
      public autofill::payments::FullCardRequest::UIDelegate {
 public:
  CvcUnmaskViewControllerEfl(
      PaymentRequestState* state,
      PaymentRequestDialogEfl* dialog,
      const autofill::CreditCard& credit_card,
      base::WeakPtr<autofill::payments::FullCardRequest::ResultDelegate>
          result_delegate,
      content::WebContents* web_contents);
  ~CvcUnmaskViewControllerEfl();

  // autofill::payments::PaymentsClientDelegate:
  IdentityProvider* GetIdentityProvider() override;
  void OnDidGetRealPan(autofill::AutofillClient::PaymentsRpcResult result,
                       const std::string& real_pan) override;
  void OnDidGetUploadDetails(
      autofill::AutofillClient::PaymentsRpcResult result,
      const base::string16& context_token,
      std::unique_ptr<base::DictionaryValue> legal_message) override;
  void OnDidUploadCard(autofill::AutofillClient::PaymentsRpcResult result,
                       const std::string& server_id) override;

  // autofill::RiskDataLoader:
  void LoadRiskData(
      const base::Callback<void(const std::string&)>& callback) override;

  // autofill::payments::FullCardRequest::UIDelegate:
  void ShowUnmaskPrompt(
      const autofill::CreditCard& card,
      autofill::AutofillClient::UnmaskCardReason reason,
      base::WeakPtr<autofill::CardUnmaskDelegate> delegate) override;
  void OnUnmaskVerificationResult(
      autofill::AutofillClient::PaymentsRpcResult result) override;

  void CvcConfirmed(base::string16 cvc_text);

 private:
  autofill::CreditCard credit_card_;
  content::WebContents* web_contents_;
  PaymentRequestDialogEfl* dialog_;
  // The identity provider, used for Payments integration.
  autofill::payments::PaymentsClient payments_client_;
  autofill::payments::FullCardRequest* full_card_request_;
  base::WeakPtr<autofill::CardUnmaskDelegate> unmask_delegate_;

  base::WeakPtrFactory<CvcUnmaskViewControllerEfl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CvcUnmaskViewControllerEfl);
};

}  // namespace payments

#endif  // CVC_UNMASK_VIEW_CONTROLLER_EFL_H_
