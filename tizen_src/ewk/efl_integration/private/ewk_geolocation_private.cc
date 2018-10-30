// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ewk_geolocation_private.h"
#include "private/ewk_security_origin_private.h"

#include "content/public/browser/browser_thread.h"
#include "device/geolocation/geolocation_provider.h"

using content::BrowserThread;
using namespace blink::mojom;

_Ewk_Geolocation_Permission_Request::_Ewk_Geolocation_Permission_Request(
    const GURL& origin_url,
    base::Callback<void(PermissionStatus)> callback)
    : origin_(new _Ewk_Security_Origin(origin_url)), callback_(callback) {}

_Ewk_Geolocation_Permission_Request::~_Ewk_Geolocation_Permission_Request() {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  delete origin_;
}

void _Ewk_Geolocation_Permission_Request::RunCallback(bool allowed) {
  if (allowed)
    device::GeolocationProvider::GetInstance()
        ->UserDidOptIntoLocationServices();
  callback_.Run(allowed ? PermissionStatus::GRANTED : PermissionStatus::DENIED);
}
