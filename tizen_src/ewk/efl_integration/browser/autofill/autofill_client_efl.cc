// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if defined(TIZEN_AUTOFILL_SUPPORT)

#include "browser/autofill/autofill_client_efl.h"

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "browser/autofill/personal_data_manager_factory.h"
#include "browser/password_manager/password_manager_client_efl.h"
#include "browser/webdata/web_data_service_factory.h"
#include "common/render_messages_ewk.h"
#include "components/autofill/content/browser/content_autofill_driver.h"
#include "components/autofill/content/browser/content_autofill_driver_factory.h"
#include "components/autofill/core/browser/autofill_client.h"
#include "components/autofill/core/browser/webdata/autofill_webdata_service.h"
#include "components/autofill/core/common/autofill_pref_names.h"
#include "components/password_manager/core/browser/password_generation_manager.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/browser/renderer_host/render_widget_host_view_efl.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/browser_thread.h"
#include "private/ewk_context_private.h"
#include "tizen/system_info.h"
#include "ui/gfx/geometry/dip_util.h"
#include "ui/gfx/geometry/rect.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(autofill::AutofillClientEfl);


namespace autofill {

AutofillClientEfl::AutofillClientEfl(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents)
    , web_contents_(web_contents)
    , webview_(NULL)
    , database_(WebDataServiceFactory::GetInstance()->GetAutofillWebDataForProfile())
    , popup_controller_(NULL) {
  DCHECK(web_contents);
}

AutofillClientEfl::~AutofillClientEfl() {
  if (popup_controller_)
    delete popup_controller_;  // LCOV_EXCL_LINE
  popup_controller_ = NULL;
}

PersonalDataManager* AutofillClientEfl::GetPersonalDataManager() {
  if (webview_ && webview_->context()) {
    content::BrowserContextEfl* ctx =
        webview_->context()->GetImpl()->browser_context();

    PersonalDataManagerFactory* factory =
        PersonalDataManagerFactory::GetInstance();

    return factory->PersonalDataManagerForContext(ctx);
  }

  return NULL;
}

scoped_refptr<AutofillWebDataService> AutofillClientEfl::GetDatabase() {
  return database_;
}

/* LCOV_EXCL_START */
PrefService* AutofillClientEfl::GetPrefs() {
  if (webview_ && webview_->context()) {
    /* LCOV_EXCL_STOP */
    content::BrowserContextEfl* ctx =
        webview_->context()->GetImpl()->browser_context();  // LCOV_EXCL_LINE
    return user_prefs::UserPrefs::Get(ctx);                 // LCOV_EXCL_LINE
  }

  return NULL;
}

syncer::SyncService* AutofillClientEfl::GetSyncService() {
  NOTIMPLEMENTED();
  return nullptr;
}

/* LCOV_EXCL_START */
void AutofillClientEfl::ShowAutofillSettings() {
  NOTIMPLEMENTED();
}

void AutofillClientEfl::ConfirmSaveCreditCardLocally(
    const CreditCard& card,
    const base::Closure& callback) {
  NOTIMPLEMENTED();
}

void AutofillClientEfl::ConfirmSaveCreditCardToCloud(
    const CreditCard& card,
    std::unique_ptr<base::DictionaryValue> legal_message,
    bool should_cvc_be_requested,
    const base::Closure& callback) {
  NOTIMPLEMENTED();
}

void AutofillClientEfl::ConfirmCreditCardFillAssist(
    const CreditCard& card,
    const base::Closure& callback) {
  NOTIMPLEMENTED();
}

void AutofillClientEfl::LoadRiskData(
    const base::Callback<void(const std::string&)>& callback) {
  NOTIMPLEMENTED();
}

void AutofillClientEfl::ShowAutofillPopup(
    const gfx::RectF& element_bounds,
    base::i18n::TextDirection text_direction,
    const std::vector<Suggestion>& suggestions,
    base::WeakPtr<AutofillPopupDelegate> delegate) {
  DCHECK(web_contents_);
  // Do not show sugestions when Remember form data is disabled
  if (!IsAutocompleteSavingEnabled())
    return;

  if (GetOrCreatePopupController()) {
    popup_controller_->InitFormData(suggestions, delegate);
#if defined(OS_TIZEN)
    popup_controller_->UpdateFormDataPopup(
        GetElementBoundsInScreen(element_bounds));
#else
    popup_controller_->UpdateFormDataPopup(element_bounds);
#endif
    popup_controller_->Show();
  }
}

void AutofillClientEfl::ShowUnmaskPrompt(
    const CreditCard& card,
    AutofillClient::UnmaskCardReason reason,
    base::WeakPtr<CardUnmaskDelegate> delegate) {
  NOTIMPLEMENTED();
}

void AutofillClientEfl::UpdateAutofillPopupDataListValues(
    const std::vector<base::string16>& values,
    const std::vector<base::string16>& labels) {
  NOTIMPLEMENTED();
}

void AutofillClientEfl::OnUnmaskVerificationResult(PaymentsRpcResult result) {
  NOTIMPLEMENTED();
}

bool AutofillClientEfl::HasCreditCardScanFeature() {
  return false;
}

void AutofillClientEfl::ScanCreditCard(
    const CreditCardScanCallback& callback) {
  NOTIMPLEMENTED();
}

bool AutofillClientEfl::IsContextSecure() {
  content::SSLStatus ssl_status;
  content::NavigationEntry* navigation_entry =
      web_contents_->GetController().GetLastCommittedEntry();
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

IdentityProvider* AutofillClientEfl::GetIdentityProvider() {
  NOTIMPLEMENTED();
  return nullptr;
}

ukm::UkmRecorder* AutofillClientEfl::GetUkmRecorder() {
  NOTIMPLEMENTED();
  return nullptr;
}

SaveCardBubbleController* AutofillClientEfl::GetSaveCardBubbleController() {
  NOTIMPLEMENTED();
  return nullptr;
}

bool AutofillClientEfl::IsAutofillSupported() {
  return true;
}

void AutofillClientEfl::ExecuteCommand(int id) {
  NOTIMPLEMENTED();
}
/* LCOV_EXCL_STOP */

void AutofillClientEfl::HideAutofillPopup() {
  if (popup_controller_) {
    popup_controller_->Hide();  // LCOV_EXCL_LINE
  }
}

/* LCOV_EXCL_START */
bool AutofillClientEfl::IsAutocompleteEnabled() {
  if (webview_)
    return webview_->GetSettings()->autofillProfileForm();
  return false;
}

void AutofillClientEfl::PropagateAutofillPredictions(
    content::RenderFrameHost* rfh,
    const std::vector<FormStructure*>& forms) {
  NOTIMPLEMENTED();
}

bool AutofillClientEfl::IsAutocompleteSavingEnabled() {
  if (webview_)
    return webview_->GetSettings()->formCandidateData();
  return false;
}
/* LCOV_EXCL_STOP */

/* LCOV_EXCL_START */
void AutofillClientEfl::ShowSavePasswordPopup(
    std::unique_ptr<password_manager::PasswordFormManager> form_to_save) {
  if (GetOrCreatePopupController())
    popup_controller_->ShowSavePasswordPopup(std::move(form_to_save));
}

void AutofillClientEfl::MainFrameWasResized(bool width_changed) {
  // Ignore virtual keyboard showing and hiding a strip of suggestions.
  if (!width_changed)
    return;

  HideAutofillPopup();
}
/* LCOV_EXCL_STOP */

void AutofillClientEfl::WebContentsDestroyed() {
  HideAutofillPopup();
}

/* LCOV_EXCL_START */
AutofillPopupViewEfl * AutofillClientEfl::GetOrCreatePopupController() {
  if (webview_ && !popup_controller_)
    popup_controller_ = new AutofillPopupViewEfl(webview_);
  return popup_controller_;
}

void AutofillClientEfl::DidFillOrPreviewField(
    const base::string16& autofilled_value,
    const base::string16& profile_full_name) {
  NOTIMPLEMENTED();
}

bool AutofillClientEfl::ShouldShowSigninPromo() {
  NOTIMPLEMENTED();
  return false;
}
/* LCOV_EXCL_STOP */

void AutofillClientEfl::UpdateAutofillIfRequired() {
  if (popup_controller_ && popup_controller_->IsVisible()) {
    /* LCOV_EXCL_START */
    content::RenderViewHost* rvh = web_contents_->GetRenderViewHost();
    if (rvh)
      rvh->Send(new EwkViewMsg_UpdateFocusedNodeBounds(rvh->GetRoutingID()));
    /* LCOV_EXCL_STOP */
  }
}

/* LCOV_EXCL_START */
void AutofillClientEfl::DidChangeFocusedNodeBounds(
    const gfx::RectF& node_bounds) {
  if (popup_controller_)
    popup_controller_->UpdateLocation(GetElementBoundsInScreen(node_bounds));
}

gfx::RectF AutofillClientEfl::GetElementBoundsInScreen(
    const gfx::RectF& element_bounds) {
  auto rwhv = static_cast<content::RenderWidgetHostViewEfl*>(
      web_contents_->GetRenderWidgetHostView());
  if (!rwhv)
    return gfx::RectF();

  double scale_factor = 1.0;
  if (IsTvProfile()) {
    scale_factor = webview_->GetScale();
  } else {
    scale_factor =
        display::Screen::GetScreen()->GetPrimaryDisplay().device_scale_factor();
  }

  gfx::Vector2d view_offset = gfx::ConvertRectToDIP(scale_factor,
      rwhv->GetViewBoundsInPix()).OffsetFromOrigin();
  return element_bounds + view_offset;
}

#if defined(OS_TIZEN_TV_PRODUCT)
void AutofillClientEfl::AutoLogin(const std::string& user_name,
                                  const std::string& password) {
  content::RenderViewHost* rvh = web_contents_->GetRenderViewHost();
  if (rvh) {
    rvh->Send(
        new EwkViewMsg_AutoLogin(rvh->GetRoutingID(), user_name, password));
  }
}
#endif
/* LCOV_EXCL_STOP */

}  // namespace autofill

#endif // TIZEN_AUTOFILL_SUPPORT
