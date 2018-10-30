// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if defined(TIZEN_AUTOFILL_SUPPORT)

#include "browser/webdata/web_data_service.h"

#include "base/bind.h"
#include "base/stl_util.h"
#include "components/webdata/common/web_database_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"

using base::Bind;
using base::Time;
using content::BrowserThread;


WebDataService::WebDataService(scoped_refptr<WebDatabaseService> wdbs,
                               const ProfileErrorCallback& callback)
    : WebDataServiceBase(wdbs, callback,
          BrowserThread::GetTaskRunnerForThread(BrowserThread::UI)) {
}

/* LCOV_EXCL_START */
WebDataService::WebDataService()
    : WebDataServiceBase(NULL, ProfileErrorCallback(),
          BrowserThread::GetTaskRunnerForThread(BrowserThread::UI)) {
}
/* LCOV_EXCL_STOP */

WebDataService::~WebDataService() {
}

#endif // TIZEN_AUTOFILL_SUPPORT
