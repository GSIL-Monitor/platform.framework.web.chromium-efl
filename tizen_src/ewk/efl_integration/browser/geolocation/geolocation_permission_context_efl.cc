// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/geolocation/geolocation_permission_context_efl.h"

#include "common/web_contents_utils.h"
#include "content/public/browser/browser_thread.h"
#include "eweb_view.h"
#include "eweb_view_callbacks.h"
#include "private/ewk_geolocation_private.h"

using web_contents_utils::WebContentsFromFrameID;
using web_contents_utils::WebViewFromWebContents;
using namespace blink::mojom;

namespace content {

GeolocationPermissionContextEfl::GeolocationPermissionContextEfl()
    : weak_ptr_factory_(this) {
}

GeolocationPermissionContextEfl::~GeolocationPermissionContextEfl() {}

void GeolocationPermissionContextEfl::RequestPermissionOnUIThread(
    int render_process_id,
    int render_frame_id,
    const GURL& requesting_frame,
    base::Callback<void(PermissionStatus)> callback) const {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
  WebContents* web_contents = WebContentsFromFrameID(render_process_id, render_frame_id);
  if (!web_contents) {
    callback.Run(PermissionStatus::DENIED);
    return;
  }

  EWebView* web_view = WebViewFromWebContents(web_contents);

  // Permission request is sent before even creating location provider -
  // it's the only place where we can clearly ask certain view if
  // we can enable it.

  // We treat geolocation_valid as opt-out not opt-in - default behaviour is to allow
  // geolocation permission callbacks. It may be blocked by geolocation,valid smart
  // callback handlers.
  Eina_Bool geolocation_valid = EINA_TRUE;
  web_view->SmartCallback<EWebViewCallbacks::GeoLocationValid>().call(
      &geolocation_valid);

  if (geolocation_valid) {
    std::unique_ptr<_Ewk_Geolocation_Permission_Request> request(
        new _Ewk_Geolocation_Permission_Request(requesting_frame, callback));

    // 'callback_result' is currently unused in webkit-efl implementation.
    Eina_Bool callback_result = EINA_FALSE;
    if (!web_view->InvokeViewGeolocationPermissionCallback(request.get(), &callback_result)) {
      web_view->SmartCallback<EWebViewCallbacks::GeoLocationPermissionRequest>().call(
          request.get());
    }

    // if request is suspended, the API takes over the request object lifetime
    // and request will be deleted after decision is made
    if (request->IsSuspended())
      ignore_result(request.release());
    else if (!request->IsDecided())  // Reject permission if request is not suspended and not decided
      callback.Run(PermissionStatus::DENIED);
  }
}

void GeolocationPermissionContextEfl::RequestPermission(
    int render_process_id,
    int render_frame_id,
    const GURL& requesting_frame,
    base::Callback<void(PermissionStatus)> callback) const {
  content::BrowserThread::PostTask(
      content::BrowserThread::UI,
      FROM_HERE,
      base::Bind(
          &GeolocationPermissionContextEfl::RequestPermissionOnUIThread,
          weak_ptr_factory_.GetWeakPtr(),
          render_process_id,
          render_frame_id,
          requesting_frame,
          callback));
}

void GeolocationPermissionContextEfl::CancelPermissionRequest(
    int /*render_process_id*/,
    int /*render_frame_id*/,
    int /*bridge_id*/,
    const GURL& /*requesting_frame*/) const {
  // There is currently no mechanism to inform the application
  // that a permission request should be canceled.
  // To be implemented in the future.
}

}  // namespace content
