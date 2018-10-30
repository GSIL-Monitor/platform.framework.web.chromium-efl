// Copyright 2017 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PAYMENT_DATA_EFL_H_
#define PAYMENT_DATA_EFL_H_

#include "browser/payments/payment_request_dialog_efl.h"
#include "components/payments/content/payment_request.h"

namespace payments {

class PaymentDataEfl {
 public:
  PaymentDataEfl(PaymentRequestDialogEfl* dialog) {
    dialog_ = dialog;
    ResetFlags();
  }

  void ResetFlags() {
    shipping_profile_selected_ = payer_phone_selected_ = payer_email_selected_ =
        credit_card_selected_ = false;
  }

  bool IsPaymentDataReady() {
    bool ready = credit_card_selected_;
    if (dialog_->request_->spec()->request_shipping())
      ready = ready && shipping_profile_selected_;
    if (dialog_->request_->spec()->request_payer_phone())
      ready = ready && payer_phone_selected_;
    if (dialog_->request_->spec()->request_payer_email())
      ready = ready && payer_email_selected_;
    return ready;
  }

  void SetSelectedPhone(autofill::AutofillProfile* profile) {
    dialog_->request_->state()->SetSelectedContactProfile(profile);
    payer_phone_selected_ = true;
    CheckAndEnablePayButton();
  }

  void SetSelectedEmail(autofill::AutofillProfile* profile) {
    dialog_->request_->state()->SetSelectedContactProfile(profile);
    payer_email_selected_ = true;
    CheckAndEnablePayButton();
  }

  void SetSelectedShippingAddress(autofill::AutofillProfile* profile) {
    dialog_->request_->state()->SetSelectedShippingProfile(profile);
    shipping_profile_selected_ = true;
    CheckAndEnablePayButton();
  }

  void SetSelectedCreditCard(autofill::CreditCard* card) {
    dialog_->request_->state()->AddAutofillPaymentInstrument(true, *card);
    credit_card_selected_ = true;
    CheckAndEnablePayButton();
  }

  void SetSelectedShippingOption(const std::string& id) {
    dialog_->request_->state()->SetSelectedShippingOption(id);
  }

  void CheckAndEnablePayButton() {
    if (dialog_->pay_button_ && IsPaymentDataReady())
      elm_object_disabled_set(dialog_->pay_button_, false);
  }

 private:
  // Flags that are enabled when respective profiles are selected.
  bool shipping_profile_selected_;
  bool payer_phone_selected_;
  bool payer_email_selected_;
  bool credit_card_selected_;

  PaymentRequestDialogEfl* dialog_;
};

}  // namespace payments

#endif  // PAYMENT_DATA_EFL_H_
