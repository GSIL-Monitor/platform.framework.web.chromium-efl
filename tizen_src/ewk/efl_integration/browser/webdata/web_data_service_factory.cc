// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if defined(TIZEN_AUTOFILL_SUPPORT)

#include "browser/webdata/web_data_service_factory.h"

#include "eweb_view.h"
#include "base/bind.h"
#include "base/path_service.h"
#include "base/files/file_path.h"
#include "components/autofill/core/browser/autofill_country.h"
#include "components/autofill/core/browser/webdata/autofill_table.h"
#include "components/autofill/core/browser/webdata/autofill_webdata_service.h"
#include "components/webdata/common/webdata_constants.h"
#include "content/common/paths_efl.h"
#include "content/public/browser/browser_thread.h"

using autofill::AutofillWebDataService;
using content::BrowserThread;

namespace {

/* LCOV_EXCL_START */
// Callback to show error dialog on profile load error.
void ProfileErrorCallback(sql::InitStatus status, const std::string& str) {
  //TODO:Need to check what type of error to show
  NOTIMPLEMENTED();
}
/* LCOV_EXCL_STOP */

void InitSyncableServicesOnDBThread(
    scoped_refptr<AutofillWebDataService> autofill_web_data,
    const base::FilePath& profile_path,
    const std::string& app_locale,
    autofill::AutofillWebDataBackend* autofill_backend) {
  //TODO:Need to check if syncable service is needed
}

}  // namespace

WebDataServiceWrapper* WebDataServiceWrapper::GetInstance() {  // LCOV_EXCL_LINE
  return base::Singleton<WebDataServiceWrapper>::get();
}

WebDataServiceWrapper::WebDataServiceWrapper() {
  base::FilePath db_path;
  PathService::Get(PathsEfl::WEB_DATABASE_DIR, &db_path);
  base::FilePath path = db_path.Append(FILE_PATH_LITERAL(".FormData.db"));

  web_database_ =
    new WebDatabaseService(
        path,
        BrowserThread::GetTaskRunnerForThread(BrowserThread::UI),
        BrowserThread::GetTaskRunnerForThread(BrowserThread::DB));

  // All tables objects that participate in managing the database must
  // be added here.
  web_database_->AddTable(base::WrapUnique(new autofill::AutofillTable));
  web_database_->LoadDatabase();

  autofill_web_data_ = new AutofillWebDataService(
      web_database_, BrowserThread::GetTaskRunnerForThread(BrowserThread::UI),
      BrowserThread::GetTaskRunnerForThread(BrowserThread::DB),
      base::Bind(&ProfileErrorCallback));
  autofill_web_data_->Init();

  web_data_ =
      new WebDataService(web_database_, base::Bind(&ProfileErrorCallback));
  web_data_->Init();

  autofill_web_data_->GetAutofillBackend(
         base::Bind(&InitSyncableServicesOnDBThread,
                    autofill_web_data_,
                    db_path,
                    EWebView::GetPlatformLocale()));
}

WebDataServiceWrapper::~WebDataServiceWrapper() {
}

scoped_refptr<AutofillWebDataService>
WebDataServiceWrapper::GetAutofillWebData() {
  return autofill_web_data_.get();
}

/* LCOV_EXCL_START */
scoped_refptr<WebDataService> WebDataServiceWrapper::GetWebData() {
  return web_data_.get();
}


// static
scoped_refptr<WebDataService> WebDataService::FromBrowserContext(
    content::BrowserContext* context) {
  WebDataServiceWrapper* wrapper = WebDataServiceFactory::GetDataService();
  if (wrapper)
    return wrapper->GetWebData();
  // |wrapper| can be NULL in Incognito mode.
  return scoped_refptr<WebDataService>(NULL);
}
/* LCOV_EXCL_STOP */

WebDataServiceFactory::WebDataServiceFactory(){
  // WebDataServiceFactory has no dependecies.
}  // LCOV_EXCL_LINE

WebDataServiceFactory::~WebDataServiceFactory() {}

/* LCOV_EXCL_START */
// static
WebDataServiceWrapper* WebDataServiceFactory::GetDataService() {
  return WebDataServiceWrapper::GetInstance();
}
/* LCOV_EXCL_STOP */

// static
scoped_refptr<AutofillWebDataService>
WebDataServiceFactory::GetAutofillWebDataForProfile() {
  WebDataServiceWrapper* wrapper =
      WebDataServiceFactory::GetDataService();
  // |wrapper| can be NULL in Incognito mode.
  return wrapper ?
      wrapper->GetAutofillWebData() :
      scoped_refptr<AutofillWebDataService>(NULL);
}

// static
WebDataServiceFactory* WebDataServiceFactory::GetInstance() {
  return base::Singleton<WebDataServiceFactory>::get();
}

#endif // TIZEN_AUTOFILL_SUPPORT
