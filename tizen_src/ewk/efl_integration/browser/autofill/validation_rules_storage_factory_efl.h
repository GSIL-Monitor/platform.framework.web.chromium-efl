// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VALIDATION_RULES_STORAGE_FACTORY_EFL_H_
#define VALIDATION_RULES_STORAGE_FACTORY_EFL_H_

#include <memory>

#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"

namespace i18n {
namespace addressinput {
class Storage;
}
}  // namespace i18n

class JsonPrefStore;

namespace autofill {

// Creates Storage objects, all of which are backed by a common pref store.
class ValidationRulesStorageFactoryEfl {
 public:
  static std::unique_ptr<::i18n::addressinput::Storage> CreateStorage();

 private:
  friend struct base::LazyInstanceTraitsBase<ValidationRulesStorageFactoryEfl>;

  ValidationRulesStorageFactoryEfl();
  ~ValidationRulesStorageFactoryEfl();

  scoped_refptr<JsonPrefStore> json_pref_store_;

  DISALLOW_COPY_AND_ASSIGN(ValidationRulesStorageFactoryEfl);
};

}  // namespace autofill

#endif  // VALIDATION_RULES_STORAGE_FACTORY_EFL_H_
