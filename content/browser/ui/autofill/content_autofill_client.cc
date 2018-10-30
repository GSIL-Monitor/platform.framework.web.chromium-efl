// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/ui/autofill/content_autofill_client.h"

#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "build/build_config.h"
// BY_SCRIPT #include "content/browser/ui/autofill/personal_data_manager_factory.h"
// BY_SCRIPT #include "content/browser/autofill/risk_util.h"
// BY_SCRIPT #include "content/browser/browser_process.h"
// BY_SCRIPT #include "content/browser/password_manager/content_password_manager_client.h"
// BY_SCRIPT #include "content/browser/profiles/profile.h"
// BY_SCRIPT #include "content/browser/profiles/profile_manager.h"
// BY_SCRIPT #include "content/browser/signin/profile_oauth2_token_service_factory.h"
// BY_SCRIPT #include "content/browser/signin/signin_manager_factory.h"
// BY_SCRIPT #include "content/browser/signin/signin_promo_util.h"
// BY_SCRIPT #include "content/browser/sync/profile_sync_service_factory.h"
// BY_SCRIPT #include "content/browser/ui/autofill/create_card_unmask_prompt_view.h"
// BY_SCRIPT #include "content/browser/ui/autofill/credit_card_scanner_controller.h"
// BY_SCRIPT #include "content/browser/ui/browser_finder.h"
// BY_SCRIPT #include "content/browser/ui/content_pages.h"
// BY_SCRIPT #include "content/browser/ui/tabs/tab_strip_model_observer.h"
// BY_SCRIPT #include "content/browser/web_data_service_factory.h"
// BY_SCRIPT #include "content/common/url_constants.h"
#include "components/autofill/content/browser/content_autofill_driver.h"
#include "components/autofill/content/browser/content_autofill_driver_factory.h"
#include "components/autofill/core/browser/ui/card_unmask_prompt_view.h"
#include "components/autofill/core/common/autofill_pref_names.h"
#include "components/autofill/core/common/autofill_switches.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/password_manager/content/browser/content_password_manager_driver.h"
#include "components/prefs/pref_service.h"
#include "components/signin/core/browser/profile_identity_provider.h"
#include "components/signin/core/browser/signin_header_helper.h"
#include "components/signin/core/browser/signin_metrics.h"
#include "components/user_prefs/user_prefs.h"
#include "content/browser/ui/autofill/autofill_popup_controller_impl.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/ssl_status.h"
// FIXME: we should not refer terrace api here
#include "terrace/browser/autofill/tin_personal_data_manager_factory.h"
#include "terrace/browser/tin_browser_context.h"
#include "terrace/browser/tin_browser_process.h"
#include "terrace/browser/tin_web_data_service_factory.h"
#include "ui/gfx/geometry/rect.h"

#if defined(OS_ANDROID)
// BY_SCRIPT #include "content/browser/android/preferences/preferences_launcher.h"
// BY_SCRIPT #include "content/browser/android/signin/signin_promo_util_android.h"
// BY_SCRIPT #include "content/browser/infobars/infobar_service.h"
// BY_SCRIPT #include "content/browser/ui/android/autofill/autofill_logger_android.h"
// BY_SCRIPT #include "content/browser/ui/android/infobars/autofill_credit_card_filling_infobar.h"
#include "components/autofill/core/browser/autofill_credit_card_filling_infobar_delegate_mobile.h"
#include "components/autofill/core/browser/autofill_save_card_infobar_delegate_mobile.h"
#include "components/autofill/core/browser/autofill_save_card_infobar_mobile.h"
#include "components/infobars/core/infobar.h"
#else  // !OS_ANDROID
// BY_SCRIPT #include "content/browser/ui/autofill/save_card_bubble_controller_impl.h"
#include "components/zoom/zoom_controller.h"
#include "content/browser/ui/browser.h"
#include "content/browser/ui/browser_commands.h"
#include "content/browser/ui/webui/signin/login_ui_service_factory.h"
#endif

DEFINE_WEB_CONTENTS_USER_DATA_KEY(autofill::ContentAutofillClient);

namespace autofill {

ContentAutofillClient::ContentAutofillClient(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      unmask_controller_(TinBrowserContext::FromBrowserContext(
                             web_contents->GetBrowserContext())
                             ->GetPrefs(),
                         TinBrowserContext::FromBrowserContext(
                             web_contents->GetBrowserContext())
                             ->IsOffTheRecord()),
      use_credit_card_helper_(false), // SBROWSER
      use_upi_helper_(false), // SBROWSER
      autofill_client_(this) {
  DCHECK(web_contents);

#if !defined(OS_ANDROID)
  // Since ZoomController is also a WebContentsObserver, we need to be careful
  // about disconnecting from it since the relative order of destruction of
  // WebContentsObservers is not guaranteed. ZoomController silently clears
  // its ZoomObserver list during WebContentsDestroyed() so there's no need
  // to explicitly remove ourselves on destruction.
  zoom::ZoomController* zoom_controller =
      zoom::ZoomController::FromWebContents(web_contents);
  // There may not always be a ZoomController, e.g. in tests.
  if (zoom_controller)
    zoom_controller->AddObserver(this);
#endif
}

ContentAutofillClient::~ContentAutofillClient() {
  // NOTE: It is too late to clean up the autofill popup; that cleanup process
  // requires that the WebContents instance still be valid and it is not at
  // this point (in particular, the WebContentsImpl destructor has already
  // finished running and we are now in the base class destructor).
  DCHECK(!popup_controller_);
}

PersonalDataManager* ContentAutofillClient::GetPersonalDataManager() {
/* SBROWSER Profile* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  return PersonalDataManagerFactory::GetForProfile(
      profile->GetOriginalProfile());*/
  return TinPersonalDataManagerFactory::GetForContext(
      g_browser_process->browser_context());
}

scoped_refptr<AutofillWebDataService> ContentAutofillClient::GetDatabase() {
  return TinWebDataServiceFactory::GetAutofillWebDataForContext(
      web_contents()->GetBrowserContext(), ServiceAccessType::EXPLICIT_ACCESS);
}

PrefService* ContentAutofillClient::GetPrefs() {
// BY_SCRIPT  return Profile::FromBrowserContext(web_contents()->GetBrowserContext())
// BY_SCRIPT      ->GetPrefs();
  return g_browser_process->browser_context()->GetPrefs();
}

syncer::SyncService* ContentAutofillClient::GetSyncService() {
  return nullptr;
// BY_SCRIPT   Profile* profile =
// BY_SCRIPT       Profile::FromBrowserContext(web_contents()->GetBrowserContext());
// BY_SCRIPT   return ProfileSyncServiceFactory::GetInstance()->GetForProfile(profile);
}

IdentityProvider* ContentAutofillClient::GetIdentityProvider() {
  return nullptr;
// BY_SCRIPT   if (!identity_provider_) {
// BY_SCRIPT     Profile* profile =
// BY_SCRIPT         Profile::FromBrowserContext(web_contents()->GetBrowserContext())
// BY_SCRIPT             ->GetOriginalProfile();
// BY_SCRIPT     base::Closure login_callback;
// BY_SCRIPT #if !defined(OS_ANDROID)
// BY_SCRIPT     login_callback =
// BY_SCRIPT         LoginUIServiceFactory::GetShowLoginPopupCallbackForProfile(profile);
// BY_SCRIPT #endif
// BY_SCRIPT     identity_provider_.reset(new ProfileIdentityProvider(
// BY_SCRIPT         SigninManagerFactory::GetForProfile(profile),
// BY_SCRIPT         ProfileOAuth2TokenServiceFactory::GetForProfile(profile),
// BY_SCRIPT         login_callback));
// BY_SCRIPT   }
// BY_SCRIPT
// BY_SCRIPT   return identity_provider_.get();
}

ukm::UkmRecorder* ContentAutofillClient::GetUkmRecorder() {
  return nullptr;
// BY_SCRIPT   return g_browser_process->ukm_reorder();
}

SaveCardBubbleController* ContentAutofillClient::GetSaveCardBubbleController() {
#if defined(OS_ANDROID)
  return nullptr;
#else
  return SaveCardBubbleControllerImpl::FromWebContents(web_contents());
#endif
}

void ContentAutofillClient::ShowAutofillSettings() { return;
// BY_SCRIPT #if defined(OS_ANDROID)
// BY_SCRIPT   content::android::PreferencesLauncher::ShowAutofillSettings();
// BY_SCRIPT #else
// BY_SCRIPT   Browser* browser = content::FindBrowserWithWebContents(web_contents());
// BY_SCRIPT   if (browser)
// BY_SCRIPT     content::ShowSettingsSubPage(browser, content::kAutofillSubPage);
// BY_SCRIPT #endif  // #if defined(OS_ANDROID)
}

void ContentAutofillClient::ShowUnmaskPrompt(
    const CreditCard& card,
    UnmaskCardReason reason,
    base::WeakPtr<CardUnmaskDelegate> delegate) {
  unmask_controller_.ShowPrompt(
      CreateCardUnmaskPromptView(&unmask_controller_, web_contents()),
      card, reason, delegate);
}

void ContentAutofillClient::OnUnmaskVerificationResult(
    PaymentsRpcResult result) {
  unmask_controller_.OnVerificationResult(result);
}

void ContentAutofillClient::ConfirmSaveCreditCardLocally(
    const CreditCard& card,
    const base::Closure& callback) {
  ConfirmSaveCreditCardLocallyInternal(card, callback);
// BY_SCRIPT #if defined(OS_ANDROID)
// BY_SCRIPT   InfoBarService::FromWebContents(web_contents())
// BY_SCRIPT       ->AddInfoBar(CreateSaveCardInfoBarMobile(
// BY_SCRIPT           base::MakeUnique<AutofillSaveCardInfoBarDelegateMobile>(
// BY_SCRIPT               false, card, std::unique_ptr<base::DictionaryValue>(nullptr),
// BY_SCRIPT               callback)));
// BY_SCRIPT #else
// BY_SCRIPT   // Do lazy initialization of SaveCardBubbleControllerImpl.
// BY_SCRIPT   autofill::SaveCardBubbleControllerImpl::CreateForWebContents(
// BY_SCRIPT       web_contents());
// BY_SCRIPT   autofill::SaveCardBubbleControllerImpl* controller =
// BY_SCRIPT       autofill::SaveCardBubbleControllerImpl::FromWebContents(web_contents());
// BY_SCRIPT   controller->ShowBubbleForLocalSave(card, callback);
// BY_SCRIPT #endif
}

void ContentAutofillClient::ConfirmSaveCreditCardToCloud(
    const CreditCard& card,
    std::unique_ptr<base::DictionaryValue> legal_message,
    bool should_cvc_be_requested,
    const base::Closure& callback) { return;
// BY_SCRIPT #if defined(OS_ANDROID)
// BY_SCRIPT   InfoBarService::FromWebContents(web_contents())
// BY_SCRIPT       ->AddInfoBar(CreateSaveCardInfoBarMobile(
// BY_SCRIPT           base::MakeUnique<AutofillSaveCardInfoBarDelegateMobile>(
// BY_SCRIPT               true, card, std::move(legal_message), callback)));
// BY_SCRIPT #else
// BY_SCRIPT   // Do lazy initialization of SaveCardBubbleControllerImpl.
// BY_SCRIPT   autofill::SaveCardBubbleControllerImpl::CreateForWebContents(web_contents());
// BY_SCRIPT   autofill::SaveCardBubbleControllerImpl* controller =
// BY_SCRIPT       autofill::SaveCardBubbleControllerImpl::FromWebContents(web_contents());
// BY_SCRIPT   controller->ShowBubbleForUpload(card, std::move(legal_message),
// BY_SCRIPT                                   should_cvc_be_requested, callback);
// BY_SCRIPT #endif
}

void ContentAutofillClient::ConfirmCreditCardFillAssist(
    const CreditCard& card,
    const base::Closure& callback) { return;
// BY_SCRIPT #if defined(OS_ANDROID)
// BY_SCRIPT   auto infobar_delegate =
// BY_SCRIPT       base::MakeUnique<AutofillCreditCardFillingInfoBarDelegateMobile>(
// BY_SCRIPT           card, callback);
// BY_SCRIPT   auto* raw_delegate = infobar_delegate.get();
// BY_SCRIPT   if (InfoBarService::FromWebContents(web_contents())->AddInfoBar(
// BY_SCRIPT           base::MakeUnique<AutofillCreditCardFillingInfoBar>(
// BY_SCRIPT               std::move(infobar_delegate)))) {
// BY_SCRIPT     raw_delegate->set_was_shown();
// BY_SCRIPT   }
// BY_SCRIPT #endif
}

void ContentAutofillClient::LoadRiskData(
    const base::Callback<void(const std::string&)>& callback) { return;
// BY_SCRIPT   ::autofill::LoadRiskData(0, web_contents(), callback);
}

bool ContentAutofillClient::HasCreditCardScanFeature() {
  return false;
// BY_SCRIPT   return CreditCardScannerController::HasCreditCardScanFeature();
}

void ContentAutofillClient::ScanCreditCard(
    const CreditCardScanCallback& callback) { return;
// BY_SCRIPT   CreditCardScannerController::ScanCreditCard(web_contents(), callback);
}

void ContentAutofillClient::ShowAutofillPopup(
    const gfx::RectF& element_bounds,
    base::i18n::TextDirection text_direction,
    const std::vector<autofill::Suggestion>& suggestions,
    base::WeakPtr<AutofillPopupDelegate> delegate) {
  // Convert element_bounds to be in screen space.
  gfx::Rect client_area = web_contents()->GetContainerBounds();
  gfx::RectF element_bounds_in_screen_space =
      element_bounds + client_area.OffsetFromOrigin();

  // Will delete or reuse the old |popup_controller_|.
  popup_controller_ =
      AutofillPopupControllerImpl::GetOrCreate(popup_controller_,
                                               delegate,
                                               web_contents(),
                                               web_contents()->GetNativeView(),
                                               element_bounds_in_screen_space,
                                               text_direction);

  popup_controller_->UseCreditCardHelper(use_credit_card_helper_);
  popup_controller_->UseUpiHelper(use_upi_helper_);
  popup_controller_->Show(suggestions);
  popup_controller_->ShowSuggestionPopup(suggestions.empty());
}

void ContentAutofillClient::UpdateAutofillPopupDataListValues(
    const std::vector<base::string16>& values,
    const std::vector<base::string16>& labels) {
  if (popup_controller_.get())
    popup_controller_->UpdateDataListValues(values, labels);
}

void ContentAutofillClient::HideAutofillPopup() {
  if (popup_controller_.get())
    popup_controller_->Hide();

  // Password generation popups behave in the same fashion and should also
  // be hidden.
// BY_SCRIPT  ContentPasswordManagerClient* password_client =
// BY_SCRIPT      ContentPasswordManagerClient::FromWebContents(web_contents());
// BY_SCRIPT  if (password_client)
// BY_SCRIPT    password_client->HidePasswordGenerationPopup();
}

bool ContentAutofillClient::IsAutocompleteEnabled() {
  // For browser, Autocomplete is always enabled as part of Autofill.
  return GetPrefs()->GetBoolean(prefs::kAutofillEnabled);
}

void ContentAutofillClient::MainFrameWasResized(bool width_changed) {
#if defined(OS_ANDROID)
  // Ignore virtual keyboard showing and hiding a strip of suggestions.
  if (!width_changed)
    return;
#endif

  HideAutofillPopup();
}

void ContentAutofillClient::WebContentsDestroyed() {
  HideAutofillPopup();
  DestroyPopupController();
}

void ContentAutofillClient::OnZoomChanged(
    const zoom::ZoomController::ZoomChangedEventData& data) {
  HideAutofillPopup();
  DestroyPopupController();
}

// FIXME: m61_3163 bringup, needs to be checked by author
bool ContentAutofillClient::IsAutofillSupported() {
  return false;
}

void ContentAutofillClient::PropagateAutofillPredictions(
    content::RenderFrameHost* rfh,
    const std::vector<autofill::FormStructure*>& forms) { return;
// BY_SCRIPT   password_manager::ContentPasswordManagerDriver* driver =
// BY_SCRIPT       password_manager::ContentPasswordManagerDriver::GetForRenderFrameHost(
// BY_SCRIPT           rfh);
// BY_SCRIPT   if (driver) {
// BY_SCRIPT     driver->GetPasswordGenerationManager()->DetectFormsEligibleForGeneration(
// BY_SCRIPT         forms);
// BY_SCRIPT     driver->GetPasswordManager()->ProcessAutofillPredictions(driver, forms);
// BY_SCRIPT   }
}

void ContentAutofillClient::DidFillOrPreviewField(
    const base::string16& autofilled_value,
    const base::string16& profile_full_name) { return;
// BY_SCRIPT #if defined(OS_ANDROID)
// BY_SCRIPT   AutofillLoggerAndroid::DidFillOrPreviewField(autofilled_value,
// BY_SCRIPT                                                profile_full_name);
// BY_SCRIPT #endif  // defined(OS_ANDROID)
}

bool ContentAutofillClient::IsContextSecure() {
  content::SSLStatus ssl_status;
  content::NavigationEntry* navigation_entry =
      web_contents()->GetController().GetLastCommittedEntry();
  if (!navigation_entry)
     return false;

  ssl_status = navigation_entry->GetSSL();
  // Note: If changing the implementation below, also change
  // AwAutofillClient::IsContextSecure. See crbug.com/505388
  return navigation_entry->GetURL().SchemeIsCryptographic() &&
         ssl_status.certificate &&
         (!net::IsCertStatusError(ssl_status.cert_status) ||
          net::IsCertStatusMinorError(ssl_status.cert_status)) &&
         !(ssl_status.content_status &
           content::SSLStatus::RAN_INSECURE_CONTENT);
}

void ContentAutofillClient::ExecuteCommand(int id) {
  // FIXME: m62_3202 bringup, needs to be checked by author
}

bool ContentAutofillClient::ShouldShowSigninPromo() { return false;
// BY_SCRIPT #if !defined(OS_ANDROID)
// BY_SCRIPT   return false;
// BY_SCRIPT #else
// BY_SCRIPT   return signin::ShouldShowPromo(
// BY_SCRIPT       Profile::FromBrowserContext(web_contents()->GetBrowserContext()));
// BY_SCRIPT #endif
}

void ContentAutofillClient::ShowHttpNotSecureExplanation() {
#if !defined(OS_ANDROID)
  // On desktop platforms, open Page Info, which briefly explains the HTTP
  // warning message and provides a link to the Help Center for more details.
  Browser* browser = content::FindBrowserWithWebContents(web_contents());
  if (browser && content::ShowPageInfo(browser, web_contents()))
    return;
// Otherwise fall through to the section below that opens the URL directly.
#endif

  // On Android, where Page Info does not (yet) contain a link to the Help
  // Center (https://crbug.com/679532), or in corner cases where Page Info is
  // not shown (for example, no navigation entry), just launch the Help topic
  // directly.
  const GURL kSecurityIndicatorHelpCenterUrl(
      "https://support.google.com/content/?p=ui_security_indicator");
  web_contents()->OpenURL(content::OpenURLParams(
      GURL(kSecurityIndicatorHelpCenterUrl), content::Referrer(),
      WindowOpenDisposition::NEW_FOREGROUND_TAB, ui::PAGE_TRANSITION_LINK,
      false /* is_renderer_initiated */));
}

}  // namespace autofill
