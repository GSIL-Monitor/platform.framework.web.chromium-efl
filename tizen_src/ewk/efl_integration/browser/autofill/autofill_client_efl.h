// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef AUTOFILL_MANAGER_DELEGATE_EFL_H
#define AUTOFILL_MANAGER_DELEGATE_EFL_H

#if defined(TIZEN_AUTOFILL_SUPPORT)

#include "eweb_view.h"
#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/i18n/rtl.h"
#include "base/memory/weak_ptr.h"
#include "components/autofill/core/browser/autofill_client.h"
#include "components/autofill/core/browser/autofill_metrics.h"
#include "components/password_manager/core/browser/password_form_manager.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "autofill_popup_view_efl.h"

namespace content {
struct FrameNavigateParams;
struct LoadCommittedDetails;
class WebContents;
}

namespace autofill {

struct FormData;

class AutofillClientEfl
    : public AutofillClient,
      public content::WebContentsUserData<AutofillClientEfl>,
      public content::WebContentsObserver {
 public:
  ~AutofillClientEfl() override;

  // AutofillClient implementation.
  PersonalDataManager* GetPersonalDataManager() override;
  scoped_refptr<AutofillWebDataService> GetDatabase() override;
  PrefService* GetPrefs() override;
  syncer::SyncService* GetSyncService() override;
  void ShowAutofillSettings() override;
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
  void ShowAutofillPopup(
      const gfx::RectF& element_bounds,
      base::i18n::TextDirection text_direction,
      const std::vector<Suggestion>& suggestions,
      base::WeakPtr<AutofillPopupDelegate> delegate) override;
  void ShowUnmaskPrompt(const CreditCard& card,
                        AutofillClient::UnmaskCardReason reason,
                        base::WeakPtr<CardUnmaskDelegate> delegate) override;
  void UpdateAutofillPopupDataListValues(
      const std::vector<base::string16>& values,
      const std::vector<base::string16>& labels) override;
  void OnUnmaskVerificationResult(PaymentsRpcResult result) override;
  bool HasCreditCardScanFeature() override;
  void ScanCreditCard(const CreditCardScanCallback& callback) override;
  bool IsContextSecure() override;

  IdentityProvider* GetIdentityProvider() override;
  ukm::UkmRecorder* GetUkmRecorder() override;
  SaveCardBubbleController* GetSaveCardBubbleController() override;
  bool IsAutofillSupported() override;
  void ExecuteCommand(int id) override;
  void HideAutofillPopup() override;
  bool IsAutocompleteEnabled() override;
  void PropagateAutofillPredictions(
      content::RenderFrameHost* rfh,
      const std::vector<FormStructure*>& forms) override;
  void DidFillOrPreviewField(
      const base::string16& autofilled_value,
      const base::string16& profile_full_name) override;
  bool ShouldShowSigninPromo() override;

  // content::WebContentsObserver implementation.
  void MainFrameWasResized(bool width_changed) override;
  void WebContentsDestroyed() override;

  void ShowSavePasswordPopup(
      std::unique_ptr<password_manager::PasswordFormManager> form_to_save);
  void SetEWebView(EWebView* view) { webview_ = view; }
  void UpdateAutofillIfRequired();
  void DidChangeFocusedNodeBounds(const gfx::RectF& node_bounds);
  bool IsAutocompleteSavingEnabled();
#if defined(OS_TIZEN_TV_PRODUCT)
  void AutoLogin(const std::string& user_name, const std::string& password);
#endif

 private:
  explicit AutofillClientEfl(content::WebContents* web_contents);
  friend class content::WebContentsUserData<AutofillClientEfl>;
  AutofillPopupViewEfl* GetOrCreatePopupController();
  gfx::RectF GetElementBoundsInScreen(const gfx::RectF& element_bounds);

  content::WebContents* const web_contents_;
  EWebView* webview_;
  scoped_refptr<AutofillWebDataService> database_;
  AutofillPopupViewEfl* popup_controller_;

  DISALLOW_COPY_AND_ASSIGN(AutofillClientEfl);
};

}  // namespace autofill

#endif // TIZEN_AUTOFILL_SUPPORT

#endif  // AUTOFILL_CLIENT_EFL_H

