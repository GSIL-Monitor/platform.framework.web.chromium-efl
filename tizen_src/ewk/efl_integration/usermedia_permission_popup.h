// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UserMediaPermissionPopup_h
#define UserMediaPermissionPopup_h

#include <string.h>

#include "private/ewk_user_media_private.h"
#include "permission_popup.h"

/* LCOV_EXCL_START */
class UserMediaPermissionPopup : public PermissionPopup {
 public:
  UserMediaPermissionPopup(Ewk_User_Media_Permission_Request* request,
    const Ewk_Security_Origin* origin, const std::string& message);

  virtual void SendDecidedPermission(Evas_Object* ewk_view, bool decide);

 private:
  Ewk_User_Media_Permission_Request* request_;
};
/* LCOV_EXCL_STOP */

#endif // UserMediaPermissionPopup_h