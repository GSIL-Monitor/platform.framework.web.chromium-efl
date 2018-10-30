// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PASSWORD_MANAGER_CLIENT_EFL_H
#define PASSWORD_MANAGER_CLIENT_EFL_H

#if defined(TIZEN_AUTOFILL_SUPPORT)

#include "base/compiler_specific.h"
#include "components/password_manager/content/browser/content_password_manager_driver.h"
#include "components/password_manager/content/browser/content_password_manager_driver_factory.h"
#include "components/password_manager/core/browser/password_manager_client.h"
#include "components/password_manager/core/browser/password_manager_metrics_recorder.h"
#include "components/password_manager/core/common/credential_manager_types.h"
#include "content/public/browser/web_contents_user_data.h"

class PasswordManager;

namespace content {
class WebContents;
}

// PasswordManagerClientEfl implements the PasswordManagerClient interface.
namespace password_manager {
class PasswordManagerClientEfl
    : public PasswordManagerClient,
      public content::WebContentsUserData<PasswordManagerClientEfl> {
public:
  virtual ~PasswordManagerClientEfl();
  using CredentialsCallback =
      base::Callback<void(const autofill::PasswordForm*)>;

  static void CreateForWebContentsWithAutofillClient(
      content::WebContents* contents,
      autofill::AutofillClient* autofill_client);

  // PasswordManagerClient implementation.
  bool PromptUserToSaveOrUpdatePassword(
      std::unique_ptr<password_manager::PasswordFormManager> form_to_save,
      bool is_update) override;
  void ShowManualFallbackForSaving(
      std::unique_ptr<password_manager::PasswordFormManager> form_to_save,
      bool has_generated_password,
      bool is_update) override;
  void HideManualFallbackForSaving() override;

  PasswordStore* GetPasswordStore() const override;

  bool PromptUserToChooseCredentials(
      std::vector<std::unique_ptr<autofill::PasswordForm>> local_forms,
      const GURL& origin,
      const CredentialsCallback& callback) override;
  void NotifyUserAutoSignin(
      std::vector<std::unique_ptr<autofill::PasswordForm>> local_forms,
      const GURL& origin) override;
  void NotifyUserCouldBeAutoSignedIn(
      std::unique_ptr<autofill::PasswordForm> form) override;
  void NotifySuccessfulLoginWithExistingPassword(
      const autofill::PasswordForm& form) override;
  void NotifyStorePasswordCalled() override;
  void AutomaticPasswordSave(
      std::unique_ptr<PasswordFormManager> saved_form_manager) override;

  const PasswordManager* GetPasswordManager() const override;
  const CredentialsFilter* GetStoreResultFilter() const override;
  const GURL& GetLastCommittedEntryURL() const override;

  PrefService* GetPrefs() override;

  bool IsFillingEnabledForCurrentPage() const override;
  bool IsSavingAndFillingEnabledForCurrentPage() const override;

#if defined(SAFE_BROWSING_DB_LOCAL)
  safe_browsing::PasswordProtectionService* GetPasswordProtectionService()
      const override;
  void CheckSafeBrowsingReputation(const GURL& form_action,
                                   const GURL& frame_url) override;

  void CheckProtectedPasswordEntry(
      bool matches_sync_password,
      const std::vector<std::string>& matching_domains,
      bool password_field_exists) override;

  void LogPasswordReuseDetectedEvent() override;
#endif

  ukm::UkmRecorder* GetUkmRecorder() override;
  ukm::SourceId GetUkmSourceId() override;
  password_manager::PasswordManagerMetricsRecorder& GetMetricsRecorder()
      override;

 private:
  explicit PasswordManagerClientEfl(content::WebContents* web_contents,
                                    autofill::AutofillClient* autofill_client);
  friend class content::WebContentsUserData<PasswordManagerClientEfl>;

  // This filter does not filter out anything, it is a dummy implementation of
  // the filter interface.
  class PassThroughCredentialsFilter : public CredentialsFilter {
   public:
    PassThroughCredentialsFilter() {}

    // CredentialsFilter:
    std::vector<std::unique_ptr<autofill::PasswordForm>> FilterResults(
        std::vector<std::unique_ptr<autofill::PasswordForm>> results)
        const override;
    bool ShouldSave(const autofill::PasswordForm& form) const override;
  };
  PassThroughCredentialsFilter credentials_filter_;

  content::WebContents* web_contents_;

  password_manager::PasswordManager password_manager_;

  password_manager::ContentPasswordManagerDriverFactory* driver_factory_;

  // Allows authentication callbacks to be destroyed when this client is gone.
  base::WeakPtrFactory<PasswordManagerClientEfl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PasswordManagerClientEfl);
};
}

#endif // TIZEN_AUTOFILL_SUPPORT

#endif  // PASSWORD_MANAGER_CLIENT_EFL_H
