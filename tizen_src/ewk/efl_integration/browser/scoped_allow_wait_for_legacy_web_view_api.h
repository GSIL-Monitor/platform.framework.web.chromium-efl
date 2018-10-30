// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


#ifndef TIZEN_WEBVIEW_BROWSER_SCOPED_ALLOW_WAIT_FOR_LEGACY_WEB_VIEW_API_H
#define TIZEN_WEBVIEW_BROWSER_SCOPED_ALLOW_WAIT_FOR_LEGACY_WEB_VIEW_API_H

#include "base/threading/thread_restrictions.h"

// This class is only available when building the chromium back-end for tizen
// webview: it is required where API backward compatibility demands that the UI
// thread must block waiting on other threads e.g. to obtain a synchronous
// return value. Long term, asynchronous overloads of all such methods will be
// added in the public API, and no new uses of this will be allowed.
class ScopedAllowWaitForLegacyWebViewApi {
 private:
  base::ThreadRestrictions::ScopedAllowWait wait;
};

#endif  // TIZEN_WEBVIEW_BROWSER_SCOPED_ALLOW_WAIT_FOR_LEGACY_WEB_VIEW_API_H
