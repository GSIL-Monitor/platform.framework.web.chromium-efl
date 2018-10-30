// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef QUOTA_PERMISSION_CONTEXT_EFL_
#define QUOTA_PERMISSION_CONTEXT_EFL_

#include "content/public/browser/quota_permission_context.h"

class QuotaPermissionContextEfl : public content::QuotaPermissionContext {
 public:
  void RequestQuotaPermission(const content::StorageQuotaParams& params,
                              int render_process_id,
                              const PermissionCallback& callback) override;

  static void DispatchCallback(const PermissionCallback& callback,
                               QuotaPermissionResponse response);
};

#endif /* QUOTA_PERMISSION_CONTEXT_EFL_ */
