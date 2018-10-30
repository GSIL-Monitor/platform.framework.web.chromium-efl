// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PASSWORD_STORE_FACTORY_H
#define PASSWORD_STORE_FACTORY_H

#if defined(TIZEN_AUTOFILL_SUPPORT)

#include "base/memory/singleton.h"
#include "components/password_manager/core/browser/password_manager_driver.h"
#include "components/prefs/pref_service.h"

namespace password_manager {
class PasswordStore;

// A wrapper of PasswordStore so we can use it as a profiled keyed service.
class PasswordStoreService {
 public:
  // |password_store| needs to be not-NULL, and the constructor expects that
  // Init() was already called successfully on it.
  explicit PasswordStoreService(scoped_refptr<PasswordStore> password_store);
  virtual ~PasswordStoreService();

  scoped_refptr<PasswordStore> GetPasswordStore();

 private:
  scoped_refptr<PasswordStore> password_store_;
  DISALLOW_COPY_AND_ASSIGN(PasswordStoreService);
};

// Singleton that owns all PasswordStores and associates them with
// Profiles.
class PasswordStoreFactory {
 public:
  static scoped_refptr<PasswordStore> GetPasswordStore();

  static PasswordStoreFactory* GetInstance();
  PasswordStoreService* GetService() { return service_; }
  void Init(PrefService* prefs);

 private:
  friend struct base::DefaultSingletonTraits<PasswordStoreFactory>;
  PasswordStoreFactory();
  virtual ~PasswordStoreFactory();

  PasswordStoreService* service_;

  DISALLOW_COPY_AND_ASSIGN(PasswordStoreFactory);
};
}
#endif // TIZEN_AUTOFILL_SUPPORT

#endif  // PASSWORD_STORE_FACTORY_H
