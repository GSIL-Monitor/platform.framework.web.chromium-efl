// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ewk_notification_private_h
#define ewk_notification_private_h

#include <Evas.h>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "ewk_suspendable_object.h"
#include "private/ewk_security_origin_private.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/WebKit/public/platform/modules/permissions/permission_status.mojom.h"

class GURL;

struct Ewk_Notification {
 public:
  Ewk_Notification(const std::string& body,
                   const std::string& title,
                   const SkBitmap& icon,
                   bool silent,
                   uint64_t notificationID,
                   const GURL& securityOrigin);
  ~Ewk_Notification();

  const char* GetBody() const;
  const char* GetTitle() const;
  bool IsSilent() const;
  Evas_Object* GetIcon(Evas* evas) const;
  int GetID() const;
  const _Ewk_Security_Origin* GetSecurityOrigin() const;
  bool SaveAsPng(const char* path) const;

 private:
  std::string body_;
  SkBitmap icon_;
  std::string title_;
  bool silent_;
  uint64_t notificationID_;
  std::unique_ptr<_Ewk_Security_Origin> origin_;

  DISALLOW_COPY_AND_ASSIGN(Ewk_Notification);

};

struct Ewk_Notification_Permission_Request : public Ewk_Suspendable_Object {
 public:
  Ewk_Notification_Permission_Request(
      const base::Callback<void(blink::mojom::PermissionStatus)>& callback,
      const GURL& source_origin);
  ~Ewk_Notification_Permission_Request();

  const _Ewk_Security_Origin* GetSecurityOrigin() const;

 private:
  std::unique_ptr<_Ewk_Security_Origin> origin_;
  base::Callback<void(blink::mojom::PermissionStatus)> callback_;

  void RunCallback(bool allowed) override;

  DISALLOW_COPY_AND_ASSIGN(Ewk_Notification_Permission_Request);
};


#endif // ewk_notification_private_h
