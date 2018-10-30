// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "geolocation_permission_popup.h"

GeolocationPermissionPopup::GeolocationPermissionPopup(
    Ewk_Geolocation_Permission_Request* request,
    const Ewk_Security_Origin* origin,
    const std::string& message)
  : PermissionPopup(origin, message),
    request_(request) {
}

void GeolocationPermissionPopup::SendDecidedPermission(
    Evas_Object* ewk_view, bool decide) {
  ewk_geolocation_permission_reply(request_, decide);
}