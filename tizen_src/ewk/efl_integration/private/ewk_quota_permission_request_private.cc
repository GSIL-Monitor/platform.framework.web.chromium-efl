// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ewk_quota_permission_request_private.h"

#include "ewk_security_origin_private.h"

_Ewk_Quota_Permission_Request::_Ewk_Quota_Permission_Request(const GURL& url, int64_t newQuota, bool isPersistent)
  : origin_(new _Ewk_Security_Origin(url)),
    newQuota_(newQuota),
    isPersistent_(isPersistent),
    view_(NULL) {
}

void _Ewk_Quota_Permission_Request::setView(Evas_Object* v) {
  view_ = v;
}

Evas_Object *_Ewk_Quota_Permission_Request::getView() const {
  return view_;
}

Eina_Stringshare* _Ewk_Quota_Permission_Request::GetHost() const {
  return origin_->GetHost();
}

Eina_Stringshare* _Ewk_Quota_Permission_Request::GetProtocol() const {
  return origin_->GetProtocol();
}

int _Ewk_Quota_Permission_Request::GetPort() const {
  return origin_->GetPort();
}

int64_t _Ewk_Quota_Permission_Request::GetQuota() const {
  return newQuota_;
}

bool _Ewk_Quota_Permission_Request::IsPersistent() const {
  return isPersistent_;
}
