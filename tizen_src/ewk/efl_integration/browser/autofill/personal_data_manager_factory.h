// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PERSONAL_DATA_MANAGER_FACTORY_H
#define PERSONAL_DATA_MANAGER_FACTORY_H

#if defined(TIZEN_AUTOFILL_SUPPORT)
#include "base/compiler_specific.h"
#include "base/containers/id_map.h"
#include "base/memory/ref_counted.h"
#include "base/memory/singleton.h"
#include "base/strings/string16.h"
#include "components/autofill/core/browser/personal_data_manager_observer.h"

namespace base {
template <typename T> struct DefaultSingletonTraits;
}

namespace content {
class BrowserContext;
}

namespace autofill {

typedef void (*Ewk_Context_Form_Autofill_Profile_Changed_Callback)(void *data);

class PersonalDataManager;
class AutofillProfile;
class CreditCard;

class PersonalDataManagerFactory : public PersonalDataManagerObserver {
 public:
  static PersonalDataManagerFactory* GetInstance();

  void PersonalDataManagerRemove(content::BrowserContext* ctx);
  PersonalDataManager* PersonalDataManagerForContext(
      content::BrowserContext* ctx);

  void SetCallback(Ewk_Context_Form_Autofill_Profile_Changed_Callback callback,
      void* user_data);

  // content::BrowserContextObserver:
  void OnPersonalDataChanged() override;

 private:
  friend struct base::DefaultSingletonTraits<PersonalDataManagerFactory>;

  PersonalDataManagerFactory();
  ~PersonalDataManagerFactory();

  base::IDMap<PersonalDataManager*> personal_data_manager_id_map_;

  DISALLOW_COPY_AND_ASSIGN(PersonalDataManagerFactory);
  Ewk_Context_Form_Autofill_Profile_Changed_Callback callback_;
  void* callback_user_data_;
};

}  // namespace autofill
#endif // TIZEN_AUTOFILL_SUPPORT
#endif // PERSONAL_DATA_MANAGER_FACTORY_H
