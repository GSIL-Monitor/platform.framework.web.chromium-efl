// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/ui/autofill/content_autofill_client.h"

#include <utility>

#include "base/android/jni_android.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "build/build_config.h"
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
#include "content/browser/ui/features.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/ssl_status.h"

// FIXME: we should not refer terrace api here
#include "terrace/browser/autofill/tin_autofill_capture_card_infobar_delegate.h"
#include "terrace/browser/autofill/tin_autofill_save_card_infobar_delegate.h"
#include "terrace/browser/autofill/tin_autofill_save_upi_infobar_delegate.h"
#include "terrace/browser/autofill/tin_credit_card_helper.h"
#include "terrace/browser/autofill/tin_personal_data_manager.h"
#include "terrace/browser/autofill/tin_personal_data_manager_factory.h"
#include "terrace/browser/autofill/tin_sdp_database_manager.h"
#include "terrace/browser/infobars/tin_infobar_service.h"
#include "terrace/browser/tin_browser_context.h"
#include "terrace/browser/tin_browser_process.h"
#include "terrace/browser/tin_sa_logging.h"
#include "terrace/terrace_jni_headers/terrace/jni/TerraceCreditCardAuthenticationManager_jni.h"

#include "ui/gfx/geometry/rect.h"

#if BUILDFLAG(ANDROID_JAVA_UI)
#else
#include "components/zoom/zoom_controller.h"
#include "content/browser/ui/webui/signin/login_ui_service_factory.h"
#endif

#if defined(OS_ANDROID)
#include "components/autofill/core/browser/autofill_credit_card_filling_infobar_delegate_mobile.h"
#include "components/autofill/core/browser/autofill_save_card_infobar_delegate_mobile.h"
#include "components/autofill/core/browser/autofill_save_card_infobar_mobile.h"
#include "components/infobars/core/infobar.h"
#else  // !OS_ANDROID
#include "content/browser/ui/browser.h"
#endif

using base::android::JavaParamRef;

class CreditCardAuthenticationCallback {
 public:
  explicit CreditCardAuthenticationCallback(
      const base::Closure& success_callback,
      const base::Closure& failure_callback) {
    success_callback_ = success_callback;
    failure_callback_ = failure_callback;
  }

  void OnResult(bool result) {
    if (result)
      success_callback_.Run();
    else failure_callback_.Run();
    delete this;
  }

 private:
  virtual ~CreditCardAuthenticationCallback() {
  }

  base::Closure success_callback_;
  base::Closure failure_callback_;
};

namespace autofill {
// constants for SALogging
const std::string& kScreen_main_screen = "201";
const std::string& kEvent_card_form = "9126";
const std::string& kEvent_card_form_autofill_enable = "9127";
const std::string& kEvent_card_form_autofill_complete = "9139";

void ContentAutofillClient::lazyInitialize() {
  use_credit_card_helper_ = false;
  use_upi_helper_ = false;
}

void ContentAutofillClient::ConfirmSaveCreditCardLocallyInternal(
    const CreditCard& card,
    const base::Closure& callback) {
  if (TinSdpDatabaseManager::GetInstance()->WasBroken())
    return;

// FIXME: m62_3202 bringup, needs to be checked by author
// https://chromium-review.googlesource.com/c/chromium/src/+/558004
/*  JNIEnv* env = base::android::AttachCurrentThread();
  java_object_.Reset(
      Java_TerraceCreditCardAuthenticationManager_getInstance(env));
  if (!Java_TerraceCreditCardAuthenticationManager_isFingerprintSupported(
          env, java_object_.obj()) &&
      !Java_TerraceCreditCardAuthenticationManager_isIrisSupported(
          env, java_object_.obj()))
    return;

  if (!TinPersonalDataManager::GetInstance()->IsCreditCardBlacklisted(card)) {
    TinInfoBarService* infobar_service =
        TinInfoBarService::FromWebContents(web_contents());
    if (!infobar_service)
      return;
    TinAutofillSaveCardInfoBarDelegate::Create(infobar_service, card, callback);
  }*/
}

void ContentAutofillClient::ConfirmSaveUPILocally(
    const UPIName& upi_name,
    const base::Closure& callback) {
  if (TinSdpDatabaseManager::GetInstance()->WasBroken())
    return;

  // TODO(abhishek.a21)
  // Below code for checking fingerprint and iris support before storing CC
  // details. Need to have similar functionality for UPI storing. Also need
  // to check if we can reuse it.
// FIXME: m62_3202 bringup, needs to be checked by author
// https://chromium-review.googlesource.com/c/chromium/src/+/558004
/*  JNIEnv* env = base::android::AttachCurrentThread();
  java_object_.Reset(
      Java_TerraceCreditCardAuthenticationManager_getInstance(env));
  if (!Java_TerraceCreditCardAuthenticationManager_isFingerprintSupported(
          env, java_object_.obj()) &&
      !Java_TerraceCreditCardAuthenticationManager_isIrisSupported(
          env, java_object_.obj()))
    return;

  if (!TinPersonalDataManager::GetInstance()->IsUPILocallyExists(upi_name)) {
    TinInfoBarService* infobar_service =
        TinInfoBarService::FromWebContents(web_contents());
    if (!infobar_service)
      return;
    TinAutofillSaveUPIInfoBarDelegate::Create(infobar_service, upi_name,
                                              callback);
  }*/
}

void ContentAutofillClient::AuthenticateCreditCardAutofill(
    AutofillManager* manager_,
    int query_id,
    const FormData& form,
    const FormFieldData& field,
    const CreditCard& credit_card) {
// FIXME: m62_3202 bringup, needs to be checked by author
// https://chromium-review.googlesource.com/c/chromium/src/+/558004
/*  JNIEnv* env = base::android::AttachCurrentThread();
  const base::Closure& success_callback = base::Bind(
        &ContentAutofillClient::OnAuthenticateCreditCardAutofillSuccess,
        autofill_client_.GetWeakPtr(),
        manager_,
        query_id, form, field, credit_card);
  const base::Closure& failure_callback = base::Bind(
        &ContentAutofillClient::OnAuthenticateCreditCardAutofillFailure,
        autofill_client_.GetWeakPtr(),
        manager_,
        query_id, form, field, credit_card);
  CreditCardAuthenticationCallback* auth_callback =
      new CreditCardAuthenticationCallback(success_callback, failure_callback);
  java_object_.Reset(Java_TerraceCreditCardAuthenticationManager_getInstance(env));
  Java_TerraceCreditCardAuthenticationManager_requestAuthentication(
      env, java_object_.obj(),
      Java_TerraceCCAuthenticationNativeCallback_create(
          env, reinterpret_cast<intptr_t>(auth_callback))
          .obj());*/
}

void ContentAutofillClient::ConfirmSaveCreditCardToSamsungPay(
    const CreditCard& card) {
  if (TinSdpDatabaseManager::GetInstance()->WasBroken())
    return;
// FIXME: m62_3202 bringup, needs to be checked by author
// https://chromium-review.googlesource.com/c/chromium/src/+/558004
/*  JNIEnv* env = base::android::AttachCurrentThread();
  java_object_.Reset(
      Java_TerraceCreditCardAuthenticationManager_getInstance(env));
  if (!Java_TerraceCreditCardAuthenticationManager_isFingerprintSupported(
          env, java_object_.obj()) &&
      !Java_TerraceCreditCardAuthenticationManager_isIrisSupported(
          env, java_object_.obj()))
    return;
  if (!TinPersonalDataManager::GetInstance()->IsSPayBlacklisted(card)) {
    TinInfoBarService* infobar_service =
        TinInfoBarService::FromWebContents(web_contents());
    if (!infobar_service)
      return;
    TinAutofillCaptureCardInfoBarDelegate::Create(infobar_service, card);
  }*/
}

void ContentAutofillClient::OnAuthenticateCreditCardAutofillSuccess(
    AutofillManager* manager_,
    int query_id,
    const FormData& form,
    const FormFieldData& field,
    const CreditCard& credit_card) {
  manager_->FillOrPreviewDataModelForm(AutofillDriver::FORM_DATA_ACTION_FILL,
                                       query_id, form, field, credit_card,
                                       CREDITCARD, base::string16());
  CompleteAutofillForSALog();
  ConfirmSaveCreditCardToSamsungPay(credit_card);
}

void ContentAutofillClient::OnAuthenticateCreditCardAutofillFailure(
    AutofillManager* manager_,
    int query_id,
    const FormData& form,
    const FormFieldData& field,
    const CreditCard& credit_card) {
}

bool ContentAutofillClient::TryAutofillForSALog() {
  TinSALogging::SendEventLog(kScreen_main_screen, kEvent_card_form);
  if (IsAutocompleteEnabled()) {
    TinSALogging::SendEventLog(kScreen_main_screen,
                               kEvent_card_form_autofill_enable);
  }
  return false;
}

void ContentAutofillClient::SALoggingUPIAutofill(
    const unsigned int& event_type) {
  TinSALogging::SendEventLog(kScreen_main_screen,
                             TinSALogging::GetUpiEventName(event_type));
}

void ContentAutofillClient::CompleteAutofillForSALog() {
  TinSALogging::SendEventLog(kScreen_main_screen,
                             kEvent_card_form_autofill_complete);
}

void ContentAutofillClient::RenderFrameDeleted(
    content::RenderFrameHost* render_frame_host) {
  HideAutofillPopup();
  DestroyPopupController();
}

void ContentAutofillClient::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  HideAutofillPopup();
  DestroyPopupController();
}

// Suggestion Popup methods
void ContentAutofillClient::OpenCreditCardHelper(
    const CreditCard& credit_card,
    const CreditCardHelperCallback& callback) {
  TinCreditCardHelper::GetInstance()->Open(credit_card, callback);
}

void ContentAutofillClient::UseCreditCardHelper(bool use_credit_card_helper) {
  if (use_credit_card_helper)
    TryAutofillForSALog();
  use_credit_card_helper_ = use_credit_card_helper;
}

void ContentAutofillClient::UseUpiHelper(bool use_upi_helper) {
  use_upi_helper_ = use_upi_helper;
}

void ContentAutofillClient::DestroySuggestionPopup() {
  if (popup_controller_.get()) {
    use_credit_card_helper_ = false;
    use_upi_helper_ = false;
    popup_controller_->DestroySuggestionPopup();
  }
}

void ContentAutofillClient::DestroyPopupController() {
  if (popup_controller_.get()) {
    use_credit_card_helper_ = false;
    use_upi_helper_ = false;
    popup_controller_->DestroyPopupControllerImpl();
    popup_controller_.reset();
  }
}

void ContentAutofillClient::DidChangeScrollOffset(
    const gfx::RectF& bounding_box) {
  if (popup_controller_.get())
    popup_controller_->UpdateSuggestionPopupAnchor(
        bounding_box.x(), bounding_box.y(), bounding_box.width(),
        bounding_box.height(), false, true);
}

void ContentAutofillClient::HideUPISuggestionPopupFullMode(
    const gfx::RectF& bounding_box) {
  if (popup_controller_.get())
    popup_controller_->UpdateSuggestionPopupAnchor(
        bounding_box.x(), bounding_box.y(), bounding_box.width(),
        bounding_box.height(), false, false);
}

std::string ContentAutofillClient::GetRegisteredUpiVpa() const {
  if (popup_controller_.get())
    return popup_controller_->GetRegisteredUpiVpa();
  return std::string();
}

} // namespace autofill

void OnResult(JNIEnv* env,
              const JavaParamRef<jclass>& jcaller,
              jlong callback_ptr,
              jboolean authenticated) {
  CreditCardAuthenticationCallback* callback =
      reinterpret_cast<CreditCardAuthenticationCallback*>(callback_ptr);
  callback->OnResult(authenticated);
}
