// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "quota_permission_context_efl.h"

#include "common/web_contents_utils.h"
#include "content/public/browser/browser_thread.h"
#include "eweb_view.h"
#include "private/ewk_quota_permission_request_private.h"

using web_contents_utils::WebViewFromViewId;
using namespace content;

void QuotaPermissionContextEfl::RequestQuotaPermission(
    const StorageQuotaParams& params,
    int render_process_id,
    const QuotaPermissionContext::PermissionCallback& callback) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(&QuotaPermissionContextEfl::RequestQuotaPermission, this,
                   params, render_process_id, callback));
    return;
  }

  EWebView* web_view =
      web_contents_utils::WebViewFromFrameId(render_process_id, params.render_frame_id);

  if (!web_view) {
    NOTREACHED();
    DispatchCallback(callback, QUOTA_PERMISSION_RESPONSE_CANCELLED);
    return;
  }

  bool isPersistent =
      (storage::StorageType::kStorageTypePersistent == params.storage_type);

  _Ewk_Quota_Permission_Request* request = new _Ewk_Quota_Permission_Request(
      params.origin_url, params.requested_size, isPersistent);

  web_view->InvokeQuotaPermissionRequest(request, callback);
}

void QuotaPermissionContextEfl::DispatchCallback(
    const QuotaPermissionContext::PermissionCallback& callback,
    QuotaPermissionContext::QuotaPermissionResponse response) {
  DCHECK_EQ(false, callback.is_null());

  if (!content::BrowserThread::CurrentlyOn(content::BrowserThread::IO)) {
    content::BrowserThread::PostTask(
        content::BrowserThread::IO, FROM_HERE,
        base::Bind(&QuotaPermissionContextEfl::DispatchCallback, callback,
                   response));
    return;
  }

  callback.Run(response);
}
