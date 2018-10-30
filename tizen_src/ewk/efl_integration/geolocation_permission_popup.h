// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GeolocationPermissionPopup_h
#define GeolocationPermissionPopup_h

#include <string.h>

#include "permission_popup.h"
#include "private/ewk_geolocation_private.h"
#include "public/ewk_geolocation_internal.h"

class GeolocationPermissionPopup : public PermissionPopup {
 public:
  GeolocationPermissionPopup(Ewk_Geolocation_Permission_Request* request,
      const Ewk_Security_Origin* origin, const std::string& message);

  virtual void SendDecidedPermission(Evas_Object* ewk_view, bool decide);

 private:
  Ewk_Geolocation_Permission_Request* request_;
};

#endif // GeolocationPermissionPopup_h
