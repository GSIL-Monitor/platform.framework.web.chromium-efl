// Copyright 2017 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PAYMENT_REQUEST_DELEGATE_EFL_H_
#define PAYMENT_REQUEST_DELEGATE_EFL_H_

#include "base/macros.h"
#include "components/autofill/core/browser/address_normalizer_impl.h"
#include "components/payments/content/content_payment_request_delegate.h"

namespace content {
class WebContents;
}

namespace payments {

class PaymentRequest;
class PaymentRequestDialog;

class PaymentRequestDelegateEfl : public ContentPaymentRequestDelegate {
 public:
  explicit PaymentRequestDelegateEfl(content::WebContents* web_contents);
  ~PaymentRequestDelegateEfl() override;

  void ShowDialog(PaymentRequest* request) override;
  void CloseDialog() override;
  void ShowErrorMessage() override;
  autofill::PersonalDataManager* GetPersonalDataManager() override;

  const std::string& GetApplicationLocale() const override;
  bool IsBrowserWindowActive() const override;
  bool IsIncognito() const override;
  bool IsSslCertificateValid() override;
  std::string GetAuthenticatedEmail() const override;
  const GURL& GetLastCommittedURL() const override;
  void DoFullCardRequest(
      const autofill::CreditCard& credit_card,
      base::WeakPtr<autofill::payments::FullCardRequest::ResultDelegate>
          result_delegate) override;
  autofill::AddressNormalizer* GetAddressNormalizer() override;
  autofill::RegionDataLoader* GetRegionDataLoader() override;
  ukm::UkmRecorder* GetUkmRecorder() override;
  PrefService* GetPrefService() override;
  scoped_refptr<PaymentManifestWebDataService>
  GetPaymentManifestWebDataService() const override;

 private:
  // Reference to the dialog so that we can satisfy calls to CloseDialog(). This
  // reference is invalid once CloseDialog() has been called on it, because the
  // dialog will be destroyed.
  PaymentRequestDialog* dialog_;
  // Not owned but outlives the PaymentRequest object that owns this.
  content::WebContents* web_contents_;
  // The address normalizer to use for the duration of the Payment Request.
  autofill::AddressNormalizerImpl address_normalizer_;
  const std::string app_locale_;

  DISALLOW_COPY_AND_ASSIGN(PaymentRequestDelegateEfl);
};

}  // namespace payments

#endif  // PAYMENT_REQUEST_DELEGATE_EFL_H_
