// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "permission_manager_efl.h"

#include "base/callback.h"
#include "browser_context_efl.h"
#include "browser/geolocation/geolocation_permission_context_efl.h"
#include "browser/notification/notification_controller_efl.h"
#include "content_browser_client_efl.h"
#include "content_main_delegate_efl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/permission_type.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "ewk_global_data.h"

namespace content {

struct PermissionManagerEfl::PendingRequest {
  PendingRequest(const std::vector<PermissionType>& permission_types,
                 content::RenderFrameHost* render_frame_host,
                 const PermissionStatusVectorCallback& callback)
      : permission_types_(permission_types),
        render_process_id_(render_frame_host->GetProcess()->GetID()),
        render_frame_id_(render_frame_host->GetRoutingID()),
        callback_(callback),
        results_(permission_types.size(), PermissionStatus::DENIED),
        remaining_results_(permission_types.size()) {}

  void SetPermissionStatus(int permission_id,
                           PermissionStatus permission_status) {
    DCHECK(!IsComplete());

    results_[permission_id] = permission_status;
    --remaining_results_;
  }

  bool IsComplete() const { return remaining_results_ == 0; }

  const PermissionStatusVectorCallback callback() const { return callback_; }

  std::vector<PermissionStatus> results() const { return results_; }

  std::vector<PermissionType> permission_types_;
  int render_process_id_;
  int render_frame_id_;
  const PermissionStatusVectorCallback callback_;
  std::vector<PermissionStatus> results_;
  size_t remaining_results_;
};

PermissionManagerEfl::PermissionManagerEfl() : weak_ptr_factory_(this) {}

PermissionManagerEfl::~PermissionManagerEfl() {
}

void PermissionStatusCallbackWrapper(
    const PermissionStatusCallback& callback,
    const std::vector<PermissionStatus>& permission_status_list) {
  DCHECK_EQ(1ul, permission_status_list.size());
  callback.Run(permission_status_list[0]);
}

int PermissionManagerEfl::RequestPermission(
    PermissionType permission,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    bool user_gesture,
    const PermissionStatusCallback& callback) {
  return RequestPermissions(
      std::vector<PermissionType>(1, permission), render_frame_host,
      requesting_origin, user_gesture,
      base::Bind(&PermissionStatusCallbackWrapper, callback));
}

int PermissionManagerEfl::RequestPermissions(
    const std::vector<PermissionType>& permissions,
    RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    bool user_gesture,
    const PermissionStatusVectorCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  if (permissions.empty()) {
    callback.Run(std::vector<PermissionStatus>());
    return kNoPendingOperation;
  }

  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);

  PendingRequest* pending_request =
      new PendingRequest(permissions, render_frame_host, callback);
  int request_id = pending_requests_.Add(pending_request);

  for (size_t i = 0; i < permissions.size(); ++i) {
    const PermissionType permission = permissions[i];

    switch (permission) {
      case PermissionType::GEOLOCATION: {
        BrowserContextEfl* browser_context =
            static_cast<BrowserContextEfl*>(web_contents->GetBrowserContext());
        if (!browser_context) {
          LOG(ERROR) << "Dropping GeolocationPermission request";
          OnPermissionRequestResponse(request_id, i, PermissionStatus::DENIED);
          continue;
        }

        const GeolocationPermissionContextEfl& geolocation_permission_context =
            browser_context->GetGeolocationPermissionContext();

        geolocation_permission_context.RequestPermission(
            pending_request->render_process_id_,
            pending_request->render_frame_id_, requesting_origin,
            base::Bind(&PermissionManagerEfl::OnPermissionRequestResponse,
                       weak_ptr_factory_.GetWeakPtr(), request_id, i));
        break;
      }
      case PermissionType::NOTIFICATIONS:
      case PermissionType::PUSH_MESSAGING: {
        ContentMainDelegateEfl& cmde =
            EwkGlobalData::GetInstance()->GetContentMainDelegatEfl();
        ContentBrowserClientEfl* cbce = static_cast<ContentBrowserClientEfl*>(
            cmde.GetContentBrowserClient());
        if (!cbce) {
          LOG(ERROR) << "Dropping NotificationPermission request";
          OnPermissionRequestResponse(request_id, i, PermissionStatus::DENIED);
          continue;
        }

        cbce->GetNotificationController()->RequestPermission(
            web_contents, requesting_origin,
            base::Bind(&PermissionManagerEfl::OnPermissionRequestResponse,
                       weak_ptr_factory_.GetWeakPtr(), request_id, i));
        break;
      }
      case PermissionType::PROTECTED_MEDIA_IDENTIFIER:
      case PermissionType::MIDI_SYSEX: {
        NOTIMPLEMENTED() << "RequestPermission not implemented for "
                         << static_cast<int>(permission);
        OnPermissionRequestResponse(request_id, i, PermissionStatus::DENIED);
        break;
      }
      default: {
        NOTREACHED() << "Invalid RequestPermission for "
                     << static_cast<int>(permission);
      }
    }
  }

  return request_id;
}

void PermissionManagerEfl::OnPermissionRequestResponse(
    int request_id,
    int permission_id,
    PermissionStatus permission_status) {
  PendingRequest* pending_request = pending_requests_.Lookup(request_id);
  if (!pending_request)
    return;

  pending_request->SetPermissionStatus(permission_id, permission_status);

  if (!pending_request->IsComplete())
    return;

  pending_requests_.Remove(request_id);
  pending_request->callback().Run(pending_request->results());
}

void PermissionManagerEfl::CancelPermissionRequest(int request_id) {
  pending_requests_.Remove(request_id);
}

PermissionStatus PermissionManagerEfl::GetPermissionStatus(
                                                 PermissionType permission,
                                                 const GURL& requesting_origin,
                                                 const GURL& embedding_origin) {
  switch (permission) {
    case PermissionType::PUSH_MESSAGING: {
      // requesting_origin should be the same as embedding_origin
      // using push API from iframes are not allowed
      if (requesting_origin != embedding_origin)
        return PermissionStatus::DENIED;
    }
    // Fall through
    case PermissionType::NOTIFICATIONS: {
      ContentMainDelegateEfl& cmde =
          EwkGlobalData::GetInstance()->GetContentMainDelegatEfl();
      ContentBrowserClientEfl* cbce =
          static_cast<ContentBrowserClientEfl*>(cmde.GetContentBrowserClient());
      if (!cbce) {
        LOG(ERROR) << "Dropping NotificationPermission request";
        return PermissionStatus::DENIED;
      }

      PermissionStatus notification_status =
          cbce->GetNotificationController()->CheckPermissionOnIOThread(
              nullptr, requesting_origin, 0);

      LOG(INFO) << "GetPermissionStatus() type:" << static_cast<int>(permission)
                << ", status: " << notification_status;

      return notification_status;
    }
    default:
      return PermissionStatus::DENIED;
  }
}

void PermissionManagerEfl::ResetPermission(PermissionType permission,
                                           const GURL& requesting_origin,
                                           const GURL& embedding_origin) {
  NOTIMPLEMENTED();
}

int PermissionManagerEfl::SubscribePermissionStatusChange(
    PermissionType permission,
    const GURL& requesting_origin,
    const GURL& embedding_origin,
    const PermissionStatusCallback& callback) {
  NOTIMPLEMENTED();
  return -1;
}

void PermissionManagerEfl::UnsubscribePermissionStatusChange(
                                              int subscription_id) {
  NOTIMPLEMENTED();
}

}  // namespace content
