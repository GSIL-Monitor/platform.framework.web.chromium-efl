// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if defined(TIZEN_AUTOFILL_SUPPORT)

#include "password_helper_efl.h"

#include "base/time/time.h"
#include "components/autofill/core/common/password_form.h"
#include "components/password_manager/core/browser/password_store.h"
#include "components/password_manager/core/browser/password_store_consumer.h"
#include "url/gurl.h"

using password_manager::PasswordStore;

namespace password_helper {

namespace {

class PasswordFormGetter : public password_manager::PasswordStoreConsumer {
 public:
  PasswordFormGetter(const password_helper::GetLoginsCallback& callback)
      : callback_(callback) {}

  void OnGetPasswordStoreResults(
      std::vector<std::unique_ptr<autofill::PasswordForm>> results) override {
    DCHECK(!callback_.is_null());
    callback_.Run(results);

    delete this;
  }

 private:
  password_helper::GetLoginsCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(PasswordFormGetter);
};

class PasswordFormRemover : public password_manager::PasswordStoreConsumer {
 public:
  PasswordFormRemover(scoped_refptr<PasswordStore> store, const GURL& origin)
      : store_(store), origin_(origin) {}

  void OnGetPasswordStoreResults(
      std::vector<std::unique_ptr<autofill::PasswordForm>> results) override {
    for (const auto& form : results) {
      if (form->origin == origin_) {
        store_->RemoveLogin(*form);
        break;
      }
    }

    delete this;
  }

 private:
  scoped_refptr<PasswordStore> store_;
  GURL origin_;

  DISALLOW_COPY_AND_ASSIGN(PasswordFormRemover);
};

}  // namespace

void GetLogins(const scoped_refptr<password_manager::PasswordStore>& store,
               const GetLoginsCallback& callback) {
  if (store.get())
    store->GetAutofillableLogins(new PasswordFormGetter(callback));
}

void RemoveLogin(const scoped_refptr<PasswordStore>& store,
                 const GURL& origin) {
  if (store.get())
    store->GetAutofillableLogins(new PasswordFormRemover(store, origin));
}

void RemoveLogins(const scoped_refptr<PasswordStore>& store) {
  if (store.get())
    store->RemoveLoginsCreatedBetween(base::Time(), base::Time::Max(),
                                      base::Closure());
}

}  // namespace password_helper

#endif  // TIZEN_AUTOFILL_SUPPORT
