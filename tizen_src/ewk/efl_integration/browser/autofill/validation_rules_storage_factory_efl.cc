// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/autofill/validation_rules_storage_factory_efl.h"

#include "base/files/file_path.h"
#include "base/path_service.h"
#include "components/prefs/json_pref_store.h"
#include "components/prefs/pref_filter.h"
#include "content/common/paths_efl.h"
#include "third_party/libaddressinput/chromium/chrome_storage_impl.h"

namespace autofill {

using ::i18n::addressinput::Storage;

// static
std::unique_ptr<Storage> ValidationRulesStorageFactoryEfl::CreateStorage() {
  static base::LazyInstance<ValidationRulesStorageFactoryEfl>::DestructorAtExit
      instance = LAZY_INSTANCE_INITIALIZER;
  return std::unique_ptr<Storage>(
      new ChromeStorageImpl(instance.Get().json_pref_store_.get()));
}

ValidationRulesStorageFactoryEfl::ValidationRulesStorageFactoryEfl() {
  base::FilePath user_data_dir;
  bool success = PathService::Get(PathsEfl::DIR_USER_DATA, &user_data_dir);
  DCHECK(success);

  json_pref_store_ = new JsonPrefStore(
      user_data_dir.Append(FILE_PATH_LITERAL("Address Validation Rules")));
  json_pref_store_->ReadPrefsAsync(NULL);
}

ValidationRulesStorageFactoryEfl::~ValidationRulesStorageFactoryEfl() {}

}  // namespace autofill
