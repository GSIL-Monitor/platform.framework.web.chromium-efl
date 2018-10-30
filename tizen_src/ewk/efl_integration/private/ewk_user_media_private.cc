// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ewk_user_media_private.h"

using content::BrowserThread;

/* LCOV_EXCL_START */
_Ewk_User_Media_Permission_Request::_Ewk_User_Media_Permission_Request(
    content::WebContentsDelegateEfl* web_contents,
    const content::MediaStreamRequest& media_request,
    const content::MediaResponseCallback& callback)
    : web_contents_(web_contents),
      request_(media_request),
      callback_(callback),
      decided_(false),
      suspended_(false),
      origin_(new _Ewk_Security_Origin(media_request.security_origin)) {}

void _Ewk_User_Media_Permission_Request::ProceedPermissionCallback(bool allowed) {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

#if defined(TIZEN_MULTIMEDIA_SUPPORT)
  Decide(allowed);
  if(allowed)
    web_contents_->RequestMediaAccessAllow(request_, callback_);
  else
    web_contents_->RequestMediaAccessDeny(request_, callback_);
#endif

  if (suspended_) {
    // If decision was suspended, then it was not deleted by the creator
    // Deletion of this object must be done after decision was made, as
    // this object will no longer be valid. But if decision was not suspended
    // it will be deleted right after permission callbacks are executed.
    BrowserThread::DeleteSoon(BrowserThread::UI, FROM_HERE, this);
  }
}
/* LCOV_EXCL_STOP */
