// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NotificationPermissionPopup_h
#define NotificationPermissionPopup_h

#include <string.h>

#include "permission_popup.h"
#include "private/ewk_notification_private.h"
#include "public/ewk_notification_internal.h"

class NotificationPermissionPopup : public PermissionPopup {
 public:
  NotificationPermissionPopup(Ewk_Notification_Permission_Request* request,
      const Ewk_Security_Origin* origin, const std::string& message);

  virtual void SendDecidedPermission(Evas_Object* ewk_view, bool decide);

 private:
  Ewk_Notification_Permission_Request* request_;
};

#endif // NotificationPermissionPopup_h
