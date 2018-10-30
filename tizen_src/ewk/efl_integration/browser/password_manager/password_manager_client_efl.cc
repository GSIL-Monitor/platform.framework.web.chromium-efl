// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if defined(TIZEN_AUTOFILL_SUPPORT)

#include "browser/password_manager/password_manager_client_efl.h"
#include "autofill_popup_view_efl.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/memory/singleton.h"
#include "base/metrics/histogram.h"
#include "browser/autofill/autofill_client_efl.h"
#include "browser/password_manager/password_store_factory.h"
#include "components/password_manager/core/browser/password_form_manager.h"
#include "components/password_manager/core/browser/password_manager.h"
#include "components/password_manager/core/browser/password_manager_metrics_util.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/web_contents.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(password_manager::PasswordManagerClientEfl);

namespace password_manager {

// static
void PasswordManagerClientEfl::CreateForWebContentsWithAutofillClient(
    content::WebContents* contents,
    autofill::AutofillClient* autofill_client) {
  DCHECK(contents);
  if (FromWebContents(contents))
    return;

  contents->SetUserData(UserDataKey(),
                        base::WrapUnique(new PasswordManagerClientEfl(
                            contents, autofill_client)));
}

PasswordManagerClientEfl::PasswordManagerClientEfl(
    content::WebContents* web_contents, autofill::AutofillClient* autofill_client)
    : web_contents_(web_contents),
      password_manager_(this),
      driver_factory_(nullptr),
      weak_factory_(this) {
  ContentPasswordManagerDriverFactory::CreateForWebContents(web_contents, this,
                                                            autofill_client);
  PasswordStoreFactory::GetInstance()->Init(
      user_prefs::UserPrefs::Get(web_contents_->GetBrowserContext()));
  driver_factory_ =
      ContentPasswordManagerDriverFactory::FromWebContents(web_contents);
}

PasswordManagerClientEfl::~PasswordManagerClientEfl() {
}

/* LCOV_EXCL_START */
bool PasswordManagerClientEfl::PromptUserToSaveOrUpdatePassword(
    std::unique_ptr<PasswordFormManager> form_to_save,
    bool update_password) {
  autofill::AutofillClientEfl * autofill_manager =
      autofill::AutofillClientEfl::FromWebContents(web_contents_);
  if (autofill_manager) {
    autofill_manager->ShowSavePasswordPopup(std::move(form_to_save));
    return true;
  }
  return false;
}

void PasswordManagerClientEfl::ShowManualFallbackForSaving(
    std::unique_ptr<password_manager::PasswordFormManager> form_to_save,
    bool has_generated_password,
    bool is_update) {
  NOTIMPLEMENTED();
}

void PasswordManagerClientEfl::HideManualFallbackForSaving() {
  NOTIMPLEMENTED();
}

PasswordStore* PasswordManagerClientEfl::GetPasswordStore() const {
  return PasswordStoreFactory::GetPasswordStore().get();
}
/* LCOV_EXCL_STOP */

const PasswordManager* PasswordManagerClientEfl::GetPasswordManager() const {
  return &password_manager_;
}

/* LCOV_EXCL_START */
bool PasswordManagerClientEfl::IsSavingAndFillingEnabledForCurrentPage()
    const {
  return !IsIncognito() && IsFillingEnabledForCurrentPage();
}

bool PasswordManagerClientEfl::IsFillingEnabledForCurrentPage() const {
  PrefService* prefs =
      user_prefs::UserPrefs::Get(web_contents_->GetBrowserContext());
  return prefs->GetBoolean(password_manager::prefs::kCredentialsEnableService);
}

bool PasswordManagerClientEfl::PromptUserToChooseCredentials(
    std::vector<std::unique_ptr<autofill::PasswordForm>> local_forms,
    const GURL& origin,
    const CredentialsCallback& callback) {
  NOTIMPLEMENTED();
  return false;
}

void PasswordManagerClientEfl::NotifyUserAutoSignin(
    std::vector<std::unique_ptr<autofill::PasswordForm>> local_forms,
    const GURL& origin) {
  DCHECK(!local_forms.empty());
  NOTIMPLEMENTED();
}

void PasswordManagerClientEfl::NotifyUserCouldBeAutoSignedIn(
    std::unique_ptr<autofill::PasswordForm> form) {
  NOTIMPLEMENTED();
}

void PasswordManagerClientEfl::NotifySuccessfulLoginWithExistingPassword(
    const autofill::PasswordForm& form) {
  NOTIMPLEMENTED();
}

void PasswordManagerClientEfl::NotifyStorePasswordCalled() {
  NOTIMPLEMENTED();
}

void PasswordManagerClientEfl::AutomaticPasswordSave(
    std::unique_ptr<PasswordFormManager> saved_form_manager) {
  NOTIMPLEMENTED();
}

const GURL& PasswordManagerClientEfl::GetLastCommittedEntryURL() const {
  DCHECK(web_contents_);
  content::NavigationEntry* entry =
      web_contents_->GetController().GetLastCommittedEntry();
  if (!entry)
    return GURL::EmptyGURL();

  return entry->GetURL();
}

const CredentialsFilter* PasswordManagerClientEfl::GetStoreResultFilter() const {
  return &credentials_filter_;
}
/* LCOV_EXCL_STOP */

PrefService* PasswordManagerClientEfl::GetPrefs() {
  return user_prefs::UserPrefs::Get(web_contents_->GetBrowserContext());
}

/* LCOV_EXCL_START */
std::vector<std::unique_ptr<autofill::PasswordForm>>
PasswordManagerClientEfl::PassThroughCredentialsFilter::FilterResults(
    std::vector<std::unique_ptr<autofill::PasswordForm>> results) const {
  return results;
}

bool PasswordManagerClientEfl::PassThroughCredentialsFilter::ShouldSave(
    const autofill::PasswordForm& form) const {
  return true;
}
#if defined(SAFE_BROWSING_DB_LOCAL)
safe_browsing::PasswordProtectionService*
PasswordManagerClientEfl::GetPasswordProtectionService() const {
  NOTIMPLEMENTED();
  return nullptr;
}

void PasswordManagerClientEfl::CheckSafeBrowsingReputation(
    const GURL& form_action,
    const GURL& frame_url) {
  NOTIMPLEMENTED();
}

void PasswordManagerClientEfl::CheckProtectedPasswordEntry(
    bool matches_sync_password,
    const std::vector<std::string>& matching_domains,
    bool password_field_exists) {
  NOTIMPLEMENTED();
}

void PasswordManagerClientEfl::LogPasswordReuseDetectedEvent() {
  NOTIMPLEMENTED();
}
#endif

ukm::UkmRecorder* PasswordManagerClientEfl::GetUkmRecorder() {
  NOTIMPLEMENTED();
  return nullptr;
}

ukm::SourceId PasswordManagerClientEfl::GetUkmSourceId() {
  NOTIMPLEMENTED();
  return 0;
}

password_manager::PasswordManagerMetricsRecorder&
PasswordManagerClientEfl::GetMetricsRecorder() {
  NOTIMPLEMENTED();
  base::Optional<PasswordManagerMetricsRecorder> recorder;
  recorder.emplace(GetUkmRecorder(), GetUkmSourceId(), GURL::EmptyGURL());
  return recorder.value();
}
/* LCOV_EXCL_STOP */
}

#endif // TIZEN_AUTOFILL_SUPPORT
