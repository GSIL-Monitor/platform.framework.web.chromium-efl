// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/mock_password_store.h"

#include "base/threading/sequenced_task_runner_handle.h"

namespace password_manager {

MockPasswordStore::MockPasswordStore() = default;

MockPasswordStore::~MockPasswordStore() = default;

scoped_refptr<base::SequencedTaskRunner>
MockPasswordStore::CreateBackgroundTaskRunner() const {
  return base::SequencedTaskRunnerHandle::Get();
}

#if defined(S_TERRACE_SUPPORT)
bool MockPasswordStore::GetLoginsWithBio(
    std::vector<std::unique_ptr<autofill::PasswordForm>>* forms) {
  return false;
}

bool MockPasswordStore::GetSavedPasswordData(
    std::vector<std::unique_ptr<autofill::PasswordForm>>* forms) {
  return false;
}

std::string MockPasswordStore::GetRegexpForPSLDomainMatch(
    const PasswordStore::FormDigest& form) {
  return std::string();
}

void MockPasswordStore::RemoveBiometricLogins(
    const base::Closure& completion) {}

void MockPasswordStore::RemoveBiometricLoginsImpl() {}
#endif

void MockPasswordStore::InitOnBackgroundSequence(
    const syncer::SyncableService::StartSyncFlare& flare) {}

}  // namespace password_manager
