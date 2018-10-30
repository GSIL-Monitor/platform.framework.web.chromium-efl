// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_EFL_INTEGRATION_BROWSER_PASSWORD_MANAGER_PASSWORD_HELPER_EFL_H_
#define EWK_EFL_INTEGRATION_BROWSER_PASSWORD_MANAGER_PASSWORD_HELPER_EFL_H_

#if defined(TIZEN_AUTOFILL_SUPPORT)

#include <vector>

#include "base/callback.h"
#include "base/memory/ref_counted.h"

class GURL;

namespace autofill {
struct PasswordForm;
}  // namespace autofill

namespace password_manager {
class PasswordStore;
}  // namespace password_manager

namespace password_helper {

typedef base::Callback<void(
    const std::vector<std::unique_ptr<autofill::PasswordForm>>& results)>
    GetLoginsCallback;

// Get all password forms from the password store.
void GetLogins(const scoped_refptr<password_manager::PasswordStore>& store,
               const GetLoginsCallback& callback);

// Removes password form which is matched with |origin] from the password store.
void RemoveLogin(const scoped_refptr<password_manager::PasswordStore>& store,
                 const GURL& origin);

// Removes all password forms from the password store.
void RemoveLogins(const scoped_refptr<password_manager::PasswordStore>& store);

}  // namespace password_helper

#endif  // TIZEN_AUTOFILL_SUPPORT
#endif  // EWK_EFL_INTEGRATION_BROWSER_PASSWORD_MANAGER_PASSWORD_HELPER_EFL_H_
