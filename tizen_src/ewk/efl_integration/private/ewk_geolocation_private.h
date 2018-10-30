// Copyright 2013 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ewk_geolocation_private_h
#define ewk_geolocation_private_h

#include "base/callback.h"
#include "ewk_suspendable_object.h"
#include "third_party/WebKit/public/platform/modules/permissions/permission_status.mojom.h"
#include "url/gurl.h"

class _Ewk_Security_Origin;

// This holds the geolocation permission request data.
// The callback present is the direct engine callback which need
// to be called once the permission is determined by app.
class _Ewk_Geolocation_Permission_Request : public Ewk_Suspendable_Object{
 public:
  _Ewk_Geolocation_Permission_Request(
      const GURL& origin_url,
      base::Callback<void(blink::mojom::PermissionStatus)> callback);
  ~_Ewk_Geolocation_Permission_Request() override;

  _Ewk_Security_Origin* GetOrigin() const { return origin_; }

 private:
  _Ewk_Security_Origin* origin_;

  void RunCallback(bool allowed) override;

  base::Callback<void(blink::mojom::PermissionStatus)> callback_;
};

#endif // ewk_geolocation_private_h
