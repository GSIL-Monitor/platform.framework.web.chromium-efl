// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_QUOTA_PERMISSION_REQUEST_PRIVATE_H
#define EWK_QUOTA_PERMISSION_REQUEST_PRIVATE_H

#include <Eina.h>
#include <Evas.h>

#include "url/gurl.h"
#include "private/ewk_security_origin_private.h"

/* LCOV_EXCL_START */
struct _Ewk_Quota_Permission_Request {
 public:
  _Ewk_Quota_Permission_Request(const GURL& url, int64_t newQuota, bool isPersistent);

  void setView(Evas_Object* view);
  Evas_Object* getView() const;
  Eina_Stringshare* GetHost() const;
  Eina_Stringshare* GetProtocol() const;
  int GetPort() const;

  int64_t GetQuota() const;
  bool IsPersistent() const;

 private:
  const std::unique_ptr<_Ewk_Security_Origin> origin_;

  int64_t newQuota_;
  bool isPersistent_;
  Evas_Object* view_;

  DISALLOW_COPY_AND_ASSIGN(_Ewk_Quota_Permission_Request);
};
/* LCOV_EXCL_STOP */

#endif // EWK_QUOTA_PERMISSION_REQUEST_PRIVATE_H
