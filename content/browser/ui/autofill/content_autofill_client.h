// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_UI_AUTOFILL_CONTENT_AUTOFILL_CLIENT_H_
#define CONTENT_BROWSER_UI_AUTOFILL_CONTENT_AUTOFILL_CLIENT_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/i18n/rtl.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/autofill/core/browser/autofill_client.h"
#include "components/autofill/core/browser/ui/card_unmask_prompt_controller_impl.h"
#include "components/zoom/zoom_observer.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "terrace/browser/autofill/tin_card_unmask_prompt_view_android.h"

namespace content {
class WebContents;
}

namespace autofill {

class AutofillPopupControllerImpl;

// Content implementation of AutofillClient.
class ContentAutofillClient
    : public AutofillClient,
      public content::WebContentsUserData<ContentAutofillClient>,
      public content::WebContentsObserver,
      public zoom::ZoomObserver {
 public:
  ~ContentAutofillClient() override;

  // AutofillClient:
  PersonalDataManager* GetPersonalDataManager() override;
  scoped_refptr<AutofillWebDataService> GetDatabase() override;
  PrefService* GetPrefs() override;
  syncer::SyncService* GetSyncService() override;
  IdentityProvider* GetIdentityProvider() override;
  ukm::UkmRecorder* GetUkmRecorder() override;
  SaveCardBubbleController* GetSaveCardBubbleController() override;
  void ShowAutofillSettings() override;
  void ShowUnmaskPrompt(const CreditCard& card,
                        UnmaskCardReason reason,
                        base::WeakPtr<CardUnmaskDelegate> delegate) override;
  void OnUnmaskVerificationResult(PaymentsRpcResult result) override;
  void ConfirmSaveCreditCardLocally(const CreditCard& card,
                                    const base::Closure& callback) override;
  void ConfirmSaveCreditCardToCloud(
      const CreditCard& card,
      std::unique_ptr<base::DictionaryValue> legal_message,
      bool should_cvc_be_requested,
      const base::Closure& callback) override;
  void ConfirmCreditCardFillAssist(const CreditCard& card,
                                   const base::Closure& callback) override;
  void LoadRiskData(
      const base::Callback<void(const std::string&)>& callback) override;
  bool HasCreditCardScanFeature() override;
  void ScanCreditCard(const CreditCardScanCallback& callback) override;
  void ShowAutofillPopup(
      const gfx::RectF& element_bounds,
      base::i18n::TextDirection text_direction,
      const std::vector<autofill::Suggestion>& suggestions,
      base::WeakPtr<AutofillPopupDelegate> delegate) override;
  void UpdateAutofillPopupDataListValues(
      const std::vector<base::string16>& values,
      const std::vector<base::string16>& labels) override;
  void HideAutofillPopup() override;
  bool IsAutocompleteEnabled() override;
  void PropagateAutofillPredictions(
      content::RenderFrameHost* rfh,
      const std::vector<autofill::FormStructure*>& forms) override;
  void DidFillOrPreviewField(const base::string16& autofilled_value,
                             const base::string16& profile_full_name) override;
  bool IsContextSecure() override;
  // FIXME: m62_3202 bringup, needs to be checked by author
  void ExecuteCommand(int id) override;
  bool ShouldShowSigninPromo() override;
  void ShowHttpNotSecureExplanation();

  // content::WebContentsObserver implementation.
  void MainFrameWasResized(bool width_changed) override;
  void WebContentsDestroyed() override;

  // ZoomObserver implementation.
  void OnZoomChanged(
      const zoom::ZoomController::ZoomChangedEventData& data) override;

  // FIXME: m61_3163 bringup, needs to be checked by author
  bool IsAutofillSupported() override;

 private:
  explicit ContentAutofillClient(content::WebContents* web_contents);
  friend class content::WebContentsUserData<ContentAutofillClient>;

  base::WeakPtr<AutofillPopupControllerImpl> popup_controller_;
  CardUnmaskPromptControllerImpl unmask_controller_;

  // The identity provider, used for Payments integration.
  std::unique_ptr<IdentityProvider> identity_provider_;

  DISALLOW_COPY_AND_ASSIGN(ContentAutofillClient);

  #include "content_autofill_client_sec.h"
};

}  // namespace autofill

bool RegisterTinCreditCardAuthenticationManager(JNIEnv* env);

#endif  // CONTENT_BROWSER_UI_AUTOFILL_CONTENT_AUTOFILL_CLIENT_H_
