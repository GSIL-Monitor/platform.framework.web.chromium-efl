// Copyright 2017 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/payments/payment_request_delegate_efl.h"

#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "browser/autofill/personal_data_manager_factory.h"
#include "browser/autofill/validation_rules_storage_factory_efl.h"
#include "browser/payments/payment_request_dialog_efl.h"
#include "components/autofill/core/browser/region_data_loader_impl.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/payments/content/payment_manifest_web_data_service.h"
#include "components/payments/content/payment_request_dialog.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/browser/renderer_host/render_widget_host_view_efl.h"
#include "content/common/content_client_export.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/web_contents.h"
#include "third_party/libaddressinput/chromium/chrome_metadata_source.h"
#include "third_party/libaddressinput/chromium/chrome_storage_impl.h"
#include "tizen_src/ewk/efl_integration/browser_context_efl.h"

namespace {

std::unique_ptr<::i18n::addressinput::Source> GetAddressInputSource(
    net::URLRequestContextGetter* url_context_getter) {
  return std::unique_ptr<::i18n::addressinput::Source>(
      new autofill::ChromeMetadataSource(I18N_ADDRESS_VALIDATION_DATA_URL,
                                         url_context_getter));
}

std::unique_ptr<::i18n::addressinput::Storage> GetAddressInputStorage() {
  return autofill::ValidationRulesStorageFactoryEfl::CreateStorage();
}

}  // namespace

namespace payments {

PaymentRequestDelegateEfl::PaymentRequestDelegateEfl(
    content::WebContents* web_contents)
    : dialog_(nullptr),
      web_contents_(web_contents),
      app_locale_(
          content::GetContentClientExport()->browser()->GetApplicationLocale()),
      address_normalizer_(
          GetAddressInputSource(
              GetPersonalDataManager()->GetURLRequestContextGetter()),
          GetAddressInputStorage()) {}

PaymentRequestDelegateEfl::~PaymentRequestDelegateEfl() {
  if (dialog_)
    delete dialog_;
}

void PaymentRequestDelegateEfl::ShowDialog(PaymentRequest* request) {
  if (!dialog_)
    dialog_ = new PaymentRequestDialogEfl(request, web_contents_);
  dialog_->ShowDialog();
}

void PaymentRequestDelegateEfl::CloseDialog() {
  if (dialog_)
    dialog_->CloseDialog();
}

void PaymentRequestDelegateEfl::ShowErrorMessage() {
  if (dialog_)
    dialog_->ShowErrorMessage();
}

autofill::PersonalDataManager*
PaymentRequestDelegateEfl::GetPersonalDataManager() {
#if defined(TIZEN_AUTOFILL_SUPPORT)
  autofill::PersonalDataManagerFactory* factory =
      autofill::PersonalDataManagerFactory::GetInstance();

  return factory->PersonalDataManagerForContext(
      web_contents_->GetBrowserContext());
#endif
  return nullptr;
}

const std::string& PaymentRequestDelegateEfl::GetApplicationLocale() const {
  return app_locale_;
}

bool PaymentRequestDelegateEfl::IsBrowserWindowActive() const {
  return web_contents_->GetRenderWidgetHostView()->IsShowing();
}

bool PaymentRequestDelegateEfl::IsIncognito() const {
  return static_cast<content::BrowserContextEfl*>(
             web_contents_->GetBrowserContext())
      ->IsOffTheRecord();
}

bool PaymentRequestDelegateEfl::IsSslCertificateValid() {
  NOTIMPLEMENTED();
  return true;
}

std::string PaymentRequestDelegateEfl::GetAuthenticatedEmail() const {
  NOTIMPLEMENTED();
  return std::string();
}

const GURL& PaymentRequestDelegateEfl::GetLastCommittedURL() const {
  return web_contents_->GetLastCommittedURL();
}

void PaymentRequestDelegateEfl::DoFullCardRequest(
    const autofill::CreditCard& credit_card,
    base::WeakPtr<autofill::payments::FullCardRequest::ResultDelegate>
        result_delegate) {
  dialog_->ShowCvcUnmaskPrompt(credit_card, result_delegate, web_contents_);
}

autofill::RegionDataLoader* PaymentRequestDelegateEfl::GetRegionDataLoader() {
  return new autofill::RegionDataLoaderImpl(
      GetAddressInputSource(
          GetPersonalDataManager()->GetURLRequestContextGetter())
          .release(),
      GetAddressInputStorage().release(), GetApplicationLocale());
}

ukm::UkmRecorder* PaymentRequestDelegateEfl::GetUkmRecorder() {
  NOTIMPLEMENTED();
  return nullptr;
}

PrefService* PaymentRequestDelegateEfl::GetPrefService() {
  return user_prefs::UserPrefs::Get(static_cast<content::BrowserContextEfl*>(
      web_contents_->GetBrowserContext()));
}

autofill::AddressNormalizer* PaymentRequestDelegateEfl::GetAddressNormalizer() {
  return &address_normalizer_;
}

scoped_refptr<PaymentManifestWebDataService>
PaymentRequestDelegateEfl::GetPaymentManifestWebDataService() const {
  return scoped_refptr<PaymentManifestWebDataService>();
}

}  // namespace payments
