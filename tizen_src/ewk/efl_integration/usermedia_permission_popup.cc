// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "usermedia_permission_popup.h"

/* LCOV_EXCL_START */
UserMediaPermissionPopup::UserMediaPermissionPopup(
    Ewk_User_Media_Permission_Request* request,
    const Ewk_Security_Origin* origin,
    const std::string& message)
  : PermissionPopup(origin, message),
    request_(request) {
}

void UserMediaPermissionPopup::SendDecidedPermission(
    Evas_Object* ewk_view, bool decide) {
  ewk_user_media_permission_reply(request_, decide);
}
/* LCOV_EXCL_STOP */