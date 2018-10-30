// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if defined(TIZEN_AUTOFILL_SUPPORT)

#include "browser/autofill/personal_data_manager_factory.h"

#include "browser/webdata/web_data_service_factory.h"
#include "components/autofill/core/browser/personal_data_manager.h"
#include "components/autofill/core/browser/webdata/autofill_webdata_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/common/content_client_export.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/browser_context.h"
#include "content/public/common/content_client.h"

namespace autofill {

// static
PersonalDataManagerFactory* PersonalDataManagerFactory::GetInstance() {
  return base::Singleton<PersonalDataManagerFactory>::get();
}

void PersonalDataManagerFactory::PersonalDataManagerRemove(
    content::BrowserContext* ctx) {
  uint64_t uniqueId = reinterpret_cast<uint64_t>(ctx);
  personal_data_manager_id_map_.Remove(uniqueId);
}

PersonalDataManager* PersonalDataManagerFactory::PersonalDataManagerForContext(
    content::BrowserContext* ctx) {
  DCHECK(ctx);
  PersonalDataManager* mgr = nullptr;

  if (ctx) {
    uint64_t uniqueId = reinterpret_cast<uint64_t>(ctx);
    mgr = personal_data_manager_id_map_.Lookup(uniqueId);
    if (!mgr) {
      // TODO: LOCALE!
      PrefService* srv = user_prefs::UserPrefs::Get(ctx);
      CHECK(srv);
      content::ContentBrowserClient* client = content::GetContentClientExport()->browser();
      mgr = new PersonalDataManager(client->GetApplicationLocale());
      mgr->Init(WebDataServiceFactory::GetAutofillWebDataForProfile(),
                srv,
                NULL,
                NULL,
                ctx->IsOffTheRecord());
      mgr->AddObserver(this);
      personal_data_manager_id_map_.AddWithID(mgr, uniqueId);
    }
  }

  return mgr;
}

PersonalDataManagerFactory::PersonalDataManagerFactory()
  : callback_(NULL)
  , callback_user_data_(NULL) {
}

PersonalDataManagerFactory::~PersonalDataManagerFactory() {
  if (!personal_data_manager_id_map_.IsEmpty())
    LOG(ERROR) << "Client didn't destroy all WebView objects"  // LCOV_EXCL_LINE
               << " before calling ewk_shutdown";              // LCOV_EXCL_LINE
}

/* LCOV_EXCL_START */
void PersonalDataManagerFactory::SetCallback(
    Ewk_Context_Form_Autofill_Profile_Changed_Callback callback,
    void* user_data) {
  callback_ = callback;
  callback_user_data_ = user_data;
}
/* LCOV_EXCL_STOP */

void PersonalDataManagerFactory::OnPersonalDataChanged() {
  if(!callback_)
    return;
  callback_(callback_user_data_);  // LCOV_EXCL_LINE
}

}  // namespace autofill

#endif // TIZEN_AUTOFILL_SUPPORT
