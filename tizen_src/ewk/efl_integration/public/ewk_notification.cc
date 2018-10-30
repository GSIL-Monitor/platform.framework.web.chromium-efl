/*
 * Copyright (C) 2013-2016 Samsung Electronics. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY SAMSUNG ELECTRONICS. AND ITS CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SAMSUNG ELECTRONICS. OR ITS
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ewk_notification_internal.h"

#include <Evas.h>

#include "content_browser_client_efl.h"
#include "content_main_delegate_efl.h"
#include "eweb_view.h"
#include "ewk_global_data.h"
#include "browser_context_efl.h"
#include "browser/notification/notification_controller_efl.h"
#include "private/ewk_context_private.h"
#include "private/ewk_security_origin_private.h"
#include "private/ewk_notification_private.h"
#include "private/ewk_private.h"

using content::ContentBrowserClientEfl;
using content::ContentMainDelegateEfl;

namespace {
ContentBrowserClientEfl* GetContentBrowserClient()
{
  if (!EwkGlobalData::IsInitialized()) {
    return nullptr;
  }

  ContentMainDelegateEfl& cmde = EwkGlobalData::GetInstance()->GetContentMainDelegatEfl();
  ContentBrowserClientEfl* cbce = static_cast<ContentBrowserClientEfl*>(cmde.GetContentBrowserClient());

  return cbce;
}
}

Eina_Bool ewk_notification_callbacks_set(Ewk_Notification_Show_Callback show_callback, Ewk_Notification_Cancel_Callback cancel_callback, void* user_data)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(show_callback, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(cancel_callback, EINA_FALSE);
  ContentBrowserClientEfl* cbce = GetContentBrowserClient();
  EINA_SAFETY_ON_NULL_RETURN_VAL(cbce, EINA_FALSE);

  cbce->GetNotificationController()->SetNotificationCallbacks(show_callback, cancel_callback, user_data);
  return EINA_TRUE;
}

Eina_Bool ewk_notification_callbacks_reset()
{
  ContentBrowserClientEfl* cbce = GetContentBrowserClient();
  EINA_SAFETY_ON_NULL_RETURN_VAL(cbce, EINA_FALSE);

  cbce->GetNotificationController()->SetNotificationCallbacks(nullptr, nullptr, nullptr);
  return EINA_TRUE;
}

EXPORT_API Evas_Object* ewk_notification_icon_get(const Ewk_Notification* ewk_notification, Evas* evas)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(ewk_notification, nullptr);
  EINA_SAFETY_ON_NULL_RETURN_VAL(evas, nullptr);
  return ewk_notification->GetIcon(evas);
}

const char* ewk_notification_body_get(const Ewk_Notification* ewk_notification)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(ewk_notification, 0);
  return ewk_notification->GetBody();
}

Eina_Bool ewk_notification_icon_save_as_png(
    const Ewk_Notification* ewk_notification, const char* path)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(ewk_notification, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(path, EINA_FALSE);

  return ewk_notification->SaveAsPng(path);
}

Eina_Bool ewk_notification_clicked(uint64_t notification_id)
{
  ContentBrowserClientEfl* cbce = GetContentBrowserClient();
  EINA_SAFETY_ON_NULL_RETURN_VAL(cbce, EINA_FALSE);
  return cbce->GetNotificationController()->NotificationClicked(notification_id);
}

const char* ewk_notification_icon_url_get(const Ewk_Notification* ewk_notification)
{
  return nullptr;
}

uint64_t ewk_notification_id_get(const Ewk_Notification* ewk_notification)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(ewk_notification, 0);
  return ewk_notification->GetID();
}

Eina_Bool ewk_notification_cached_permissions_set(Eina_List* ewk_notification_permissions)
{
  ContentBrowserClientEfl* cbce = GetContentBrowserClient();
  EINA_SAFETY_ON_NULL_RETURN_VAL(cbce, EINA_FALSE);

  content::NotificationControllerEfl* notification_controller = cbce->GetNotificationController();
  notification_controller->ClearPermissions();
  Eina_List* list;
  void* data;
  EINA_LIST_FOREACH(ewk_notification_permissions, list, data) {
    Ewk_Notification_Permission* perm = static_cast<Ewk_Notification_Permission*>(data);
    notification_controller->PutPermission(GURL(perm->origin), (perm->allowed == EINA_TRUE));
  }

  return EINA_TRUE;
}

const Ewk_Security_Origin* ewk_notification_permission_request_origin_get(
    const Ewk_Notification_Permission_Request* request)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(request, 0);
  return static_cast<const Ewk_Security_Origin*>(request->GetSecurityOrigin());
}

Eina_Bool ewk_notification_permission_reply(Ewk_Notification_Permission_Request* request, Eina_Bool allow)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(request, EINA_FALSE);
  ContentBrowserClientEfl* cbce = GetContentBrowserClient();
  EINA_SAFETY_ON_NULL_RETURN_VAL(cbce, EINA_FALSE);
  cbce->GetNotificationController()->SetPermissionForNotification(request, allow);
  return request->SetDecision(allow == EINA_TRUE);
}

Eina_Bool ewk_notification_permission_request_set(Ewk_Notification_Permission_Request* request, Eina_Bool allow)
{
  return ewk_notification_permission_reply(request, allow);
}

Eina_Bool ewk_notification_permission_request_suspend(Ewk_Notification_Permission_Request* request)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(request, EINA_FALSE);
  return request->Suspend();
}

Eina_Bool ewk_notification_policies_removed(Eina_List* origins)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(origins, EINA_FALSE);
  ContentBrowserClientEfl* cbce = GetContentBrowserClient();
  EINA_SAFETY_ON_NULL_RETURN_VAL(cbce, EINA_FALSE);
  cbce->GetNotificationController()->RemovePermissions(origins);
  return EINA_TRUE;
}

const Ewk_Security_Origin* ewk_notification_security_origin_get(const Ewk_Notification* ewk_notification)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(ewk_notification, 0);
  return static_cast<const Ewk_Security_Origin*>(ewk_notification->GetSecurityOrigin());
}

Eina_Bool ewk_notification_showed(uint64_t notification_id)
{
  ContentBrowserClientEfl* cbce = GetContentBrowserClient();
  EINA_SAFETY_ON_NULL_RETURN_VAL(cbce, EINA_FALSE);
  return cbce->GetNotificationController()->NotificationDisplayed(notification_id);
}

Eina_Bool ewk_notification_closed(uint64_t notification_id, Eina_Bool by_user)
{
  ContentBrowserClientEfl* cbce = GetContentBrowserClient();
  EINA_SAFETY_ON_NULL_RETURN_VAL(cbce, EINA_FALSE);
  return cbce->GetNotificationController()->NotificationClosed(notification_id, by_user);
}

const char* ewk_notification_title_get(const Ewk_Notification* ewk_notification)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(ewk_notification, 0);
  return ewk_notification->GetTitle();
}

Eina_Bool ewk_notification_silent_get(const Ewk_Notification* ewk_notification)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(ewk_notification, EINA_FALSE);
  return ewk_notification->IsSilent();
}
