// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PERMISSION_MANAGER_EFL_H_
#define PERMISSION_MANAGER_EFL_H_

#include "base/callback_forward.h"
#include "base/containers/id_map.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/permission_manager.h"

using namespace blink::mojom;

namespace content {

using PermissionStatusCallback = base::Callback<void(PermissionStatus)>;
using PermissionStatusVectorCallback =
    base::Callback<void(const std::vector<PermissionStatus>&)>;

class CONTENT_EXPORT PermissionManagerEfl : public PermissionManager {
 public:

  PermissionManagerEfl();
  ~PermissionManagerEfl() override;

  int RequestPermission(PermissionType permission,
                        content::RenderFrameHost* render_frame_host,
                        const GURL& requesting_origin,
                        bool user_gesture,
                        const PermissionStatusCallback& callback) override;

  int RequestPermissions(
      const std::vector<PermissionType>& permissions,
      RenderFrameHost* render_frame_host,
      const GURL& requesting_origin,
      bool user_gesture,
      const PermissionStatusVectorCallback& callback) override;

  void CancelPermissionRequest(int request_id) override;

  PermissionStatus GetPermissionStatus(PermissionType permission,
                                       const GURL& requesting_origin,
                                       const GURL& embedding_origin) override;

  void ResetPermission(PermissionType permission,
                       const GURL& requesting_origin,
                       const GURL& embedding_origin) override;

  // Runs the given |callback| whenever the |permission| associated with the
  // pair { requesting_origin, embedding_origin } changes.
  // Returns the subscription_id to be used to unsubscribe.
  int SubscribePermissionStatusChange(
      PermissionType permission,
      const GURL& requesting_origin,
      const GURL& embedding_origin,
      const PermissionStatusCallback& callback) override;

  void UnsubscribePermissionStatusChange(int subscription_id) override;

 private:
  struct PendingRequest;
  using PendingRequestsMap = base::IDMap<PendingRequest*>;

  void OnPermissionRequestResponse(int request_id,
                                   int permission_id,
                                   PermissionStatus permission_status);

  PendingRequestsMap pending_requests_;
  base::WeakPtrFactory<PermissionManagerEfl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(PermissionManagerEfl);
};

}

#endif // PERMISSION_MANAGER_EFL_H_
