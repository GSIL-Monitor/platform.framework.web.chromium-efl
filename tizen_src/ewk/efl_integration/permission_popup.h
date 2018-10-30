// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PermissionPopup_h
#define PermissionPopup_h

#include <Evas.h>
#include <string.h>

#include "base/strings/utf_string_conversions.h"
#include "private/ewk_security_origin_private.h"
#include "public/ewk_security_origin.h"

class PermissionPopup {
 public:
  /* LCOV_EXCL_START */
  PermissionPopup(const Ewk_Security_Origin* origin, const std::string& message) {
    origin_ = origin;
    popup_message_ = message;
  }

  virtual ~PermissionPopup() { }

  std::string GetMessage() const { return popup_message_; }
  std::string GetOrigin() const {
    return origin_->GetURL().spec();
  }
  /* LCOV_EXCL_STOP */
  virtual void SendDecidedPermission(Evas_Object*, bool decide) = 0;

 private:
  const Ewk_Security_Origin* origin_;
  std::string popup_message_;
};

#endif // PermissionPopup_h
