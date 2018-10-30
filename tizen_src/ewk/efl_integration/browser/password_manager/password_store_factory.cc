// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if defined(TIZEN_AUTOFILL_SUPPORT)

#include "browser/password_manager/password_store_factory.h"

#include "base/command_line.h"
#include "base/environment.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "browser/webdata/web_data_service.h"
#include "browser/webdata/web_data_service_factory.h"
#include "components/password_manager/core/browser/login_database.h"
#include "components/password_manager/core/browser/password_store.h"
#include "components/password_manager/core/browser/password_store_default.h"
#include "content/common/paths_efl.h"
#include "content/public/browser/browser_thread.h"

using namespace password_manager;
using password_manager::PasswordStore;

PasswordStoreService::PasswordStoreService(
    scoped_refptr<PasswordStore> password_store)
    : password_store_(password_store)
{
}

/* LCOV_EXCL_START */
PasswordStoreService::~PasswordStoreService()
{
}
/* LCOV_EXCL_STOP */

scoped_refptr<PasswordStore> PasswordStoreService::GetPasswordStore()
{
  return password_store_;
}

// static
scoped_refptr<PasswordStore> PasswordStoreFactory::GetPasswordStore()
{
  PasswordStoreService* service = GetInstance()->GetService();
  if (!service)
    return NULL;
  return service->GetPasswordStore();
}

// static
PasswordStoreFactory* PasswordStoreFactory::GetInstance()
{
  return base::Singleton<PasswordStoreFactory>::get();
}

PasswordStoreFactory::PasswordStoreFactory()
    : service_(NULL) {
  WebDataServiceFactory::GetInstance();
}  // LCOV_EXCL_LINE

PasswordStoreFactory::~PasswordStoreFactory()
{
}

void PasswordStoreFactory::Init(PrefService* prefs) {
  base::FilePath db_path;
  if (!PathService::Get(PathsEfl::WEB_DATABASE_DIR, &db_path)) {
    /* LCOV_EXCL_START */
    LOG(ERROR) << "Could not access login database path.";
    return;
    /* LCOV_EXCL_STOP */
  }

  base::FilePath login_db_file_path =
      db_path.Append(FILE_PATH_LITERAL(".LoginData.db"));
  std::unique_ptr<LoginDatabase> login_db(
      new LoginDatabase(login_db_file_path));

  scoped_refptr<PasswordStore> ps =
      new PasswordStoreDefault(std::move(login_db));
  if (!ps.get() ||
      !ps->Init(syncer::SyncableService::StartSyncFlare(), prefs)) {
    NOTREACHED() << "Could not initialize password manager.";
    return;
  }

  service_ = new PasswordStoreService(ps);
}

#endif // TIZEN_AUTOFILL_SUPPORT
