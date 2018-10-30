// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

 public:
  void AuthenticateCreditCardAutofill(AutofillManager*,
                                      int query_id,
                                      const FormData& form,
                                      const FormFieldData& field,
                                      const CreditCard& credit_card) override;
  void ConfirmSaveCreditCardLocallyInternal(const CreditCard& card,
                                    const base::Closure& callback);
  void ConfirmSaveUPILocally(const UPIName& card,
                             const base::Closure& callback) override;
  void ConfirmSaveCreditCardToSamsungPay(const CreditCard& card) override;

  bool TryAutofillForSALog();

  void CompleteAutofillForSALog();

  void SALoggingUPIAutofill(const unsigned int&) override;

  // Suggestion Popup methods
  void OpenCreditCardHelper(const CreditCard& credit_card,
                            const CreditCardHelperCallback& callback) override;
  void UseCreditCardHelper(bool use_credit_card_helper) override;
  void UseUpiHelper(bool use_upi_helper) override;
  void DestroySuggestionPopup() override;
  void DestroyPopupController();
  void DidChangeScrollOffset(const gfx::RectF& bounding_box) override;
  void HideUPISuggestionPopupFullMode(const gfx::RectF& bounding_box) override;
  std::string GetRegisteredUpiVpa() const override;

  void RenderFrameDeleted(content::RenderFrameHost* rfh) override;
  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;

 private:
  void lazyInitialize();
  void OnAuthenticateCreditCardAutofillSuccess(AutofillManager* manager_,
                                               int query_id,
                                               const FormData& form,
                                               const FormFieldData& field,
                                               const CreditCard& credit_card);
  void OnAuthenticateCreditCardAutofillFailure(AutofillManager* manager_,
                                               int query_id,
                                               const FormData& form,
                                               const FormFieldData& field,
                                               const CreditCard& credit_card);
 bool use_credit_card_helper_;
 bool use_upi_helper_;
 base::android::ScopedJavaGlobalRef<jobject> java_object_;
 base::WeakPtrFactory<ContentAutofillClient> autofill_client_;

