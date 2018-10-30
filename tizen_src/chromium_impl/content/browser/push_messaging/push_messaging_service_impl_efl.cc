// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/push_messaging/push_messaging_service_impl_efl.h"

#include <vector>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/permission_type.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/push_messaging_status.mojom.h"
#include "content/public/common/push_subscription_options.h"
#include "ewk/efl_integration/browser/permission_manager_efl.h"
#include "ewk/efl_integration/browser/push_messaging/push_messaging_app_identifier_efl.h"
#include "ewk/efl_integration/browser/push_messaging/push_messaging_service_factory_efl.h"
#include "ewk/efl_integration/browser_context_efl.h"
#include "ewk/efl_integration/eweb_context.h"

namespace {

// Chrome does not yet support silent push messages, and requires websites to
// indicate that they will only send user-visible messages.
const char kSilentPushUnsupportedMessage[] =
    "Chrome currently only supports the Push API for subscriptions that will "
    "result in user-visible messages. You can indicate this by calling "
    "pushManager.subscribe({userVisibleOnly: true}) instead. See "
    "https://goo.gl/yqv4Q4 for more details.";

blink::WebPushPermissionStatus ToWebPushPermission(
    blink::mojom::PermissionStatus permission_status) {
  switch (permission_status) {
    case blink::mojom::PermissionStatus::GRANTED:
      return blink::kWebPushPermissionStatusGranted;
    case blink::mojom::PermissionStatus::DENIED:
      return blink::kWebPushPermissionStatusDenied;
    case blink::mojom::PermissionStatus::ASK:
      return blink::kWebPushPermissionStatusPrompt;
    default:
      NOTREACHED();
      return blink::kWebPushPermissionStatusDenied;
  }
}

content::mojom::PushRegistrationStatus ToPushRegistrationStatus(
    sppclient::PushRegistrationResult result) {
  switch (result) {
    case sppclient::PushRegistrationResult::SUCCESS:
      return content::mojom::PushRegistrationStatus::SUCCESS_FROM_PUSH_SERVICE;
    case sppclient::PushRegistrationResult::REGISTRATION_LIMIT_REACHED:
      return content::mojom::PushRegistrationStatus::LIMIT_REACHED;
    case sppclient::PushRegistrationResult::ERROR:
      return content::mojom::PushRegistrationStatus::SERVICE_ERROR;
    default:
      NOTREACHED();
      return content::mojom::PushRegistrationStatus::SERVICE_ERROR;
  }
}

content::mojom::PushUnregistrationStatus ToPushUnregistrationStatus(
    sppclient::PushRegistrationResult result) {
  switch (result) {
    case sppclient::PushRegistrationResult::SUCCESS:
      return content::mojom::PushUnregistrationStatus::SUCCESS_UNREGISTERED;
    case sppclient::PushRegistrationResult::NOT_REGISTERED:
      return content::mojom::PushUnregistrationStatus::
          SUCCESS_WAS_NOT_REGISTERED;
    case sppclient::PushRegistrationResult::ERROR:
      return content::mojom::PushUnregistrationStatus::PENDING_SERVICE_ERROR;
    default:
      NOTREACHED();
      return content::mojom::PushUnregistrationStatus::PENDING_SERVICE_ERROR;
  }
}

void UnregisterCallbackToClosure(
    const base::Closure& closure,
    content::mojom::PushUnregistrationStatus status) {
  closure.Run();
}

}  // namespace

// static
void PushMessagingServiceImplEfl::InitializeForContext(
    content::BrowserContext* context) {
  // * Comment copied from upstream *
  // TODO(johnme): Consider whether push should be enabled in incognito.
  if (!context || context->IsOffTheRecord())
    return;

  //  * Comment copied from upstream *
  // TODO(johnme): If push becomes enabled in incognito (and this still uses a
  // pref), be careful that this pref is read from the right profile, as prefs
  // defined in a regular profile are visible in the corresponding incognito
  // profile unless overridden.
  // TODO(johnme): Make sure this pref doesn't get out of sync after crashes.

  PushMessagingServiceImplEfl* push_service =
      PushMessagingServiceFactoryEfl::GetForContext(context);
  DCHECK(push_service);
}

PushMessagingServiceImplEfl::PushMessagingServiceImplEfl(
    content::BrowserContext* context)
    : context_(context) {
  scoped_refptr<base::SequencedTaskRunner> task_runner =
      base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND,
           base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});

  driver_.reset(
      sppclient::SppClient::Create(context->GetPath(), task_runner, this));
}

PushMessagingServiceImplEfl::~PushMessagingServiceImplEfl() {}

// OnMessage methods -----------------------------------------------------------

void PushMessagingServiceImplEfl::OnMessage(const std::string& app_id,
                                            const std::string& sender_id,
                                            const std::string& data) {
  base::Closure message_handled_closure =
      message_callback_for_testing_.is_null() ? base::Bind(&base::DoNothing)
                                              : message_callback_for_testing_;

  PushMessagingAppIdentifierEfl app_identifier =
      PushMessagingAppIdentifierEfl::Generate(app_id);
  if (app_identifier.is_null()) {
    DeliverMessageCallback(app_id, GURL::EmptyGURL(), -1, data,
                           message_handled_closure,
                           content::mojom::PushDeliveryStatus::UNKNOWN_APP_ID);
    return;
  }

  // Drop message and unregister if |origin| has lost push permission.
  if (!IsPermissionSet(app_identifier.origin())) {
    DeliverMessageCallback(
        app_id, app_identifier.origin(),
        app_identifier.service_worker_registration_id(), data,
        message_handled_closure,
        content::mojom::PushDeliveryStatus::PERMISSION_DENIED);
    return;
  }

  EWebContext* context =
      static_cast<content::BrowserContextEfl*>(context_)->WebContext();
  if (context->HasPushMessageCallback()) {
    context->NotifyPushMessage(sender_id, app_identifier.push_data(data));
    return;
  }

  DeliverMessage(app_identifier, data);
}

void PushMessagingServiceImplEfl::DeliverMessage(const std::string& push_data) {
  std::string data;
  PushMessagingAppIdentifierEfl app_identifier =
      PushMessagingAppIdentifierEfl::GenerateFromPushData(push_data, data);
  DeliverMessage(app_identifier, data);
}

void PushMessagingServiceImplEfl::DeliverMessage(
    const PushMessagingAppIdentifierEfl& app_identifier,
    const std::string& data) {
  base::Closure message_handled_closure =
      message_callback_for_testing_.is_null() ? base::Bind(&base::DoNothing)
                                              : message_callback_for_testing_;

  if (app_identifier.is_null())
    return;

  content::PushEventPayload payload;
  payload.setData(data);

  content::BrowserContext::DeliverPushMessage(
      context_, app_identifier.origin(),
      app_identifier.service_worker_registration_id(), payload,
      base::Bind(&PushMessagingServiceImplEfl::DeliverMessageCallback,
                 base::Unretained(this), app_identifier.app_id(),
                 app_identifier.origin(),
                 app_identifier.service_worker_registration_id(), data,
                 message_handled_closure));
}

void PushMessagingServiceImplEfl::DeliverMessageCallback(
    const std::string& app_id,
    const GURL& requesting_origin,
    int64_t service_worker_registration_id,
    const std::string& data,
    const base::Closure& message_handled_closure,
    content::mojom::PushDeliveryStatus status) {
  base::Closure completion_closure =
      base::Bind(&PushMessagingServiceImplEfl::DidHandleMessage,
                 base::Unretained(this), app_id, message_handled_closure);
  // The completion_closure should run by default at the end of this function,
  // unless it is explicitly passed to another function.
  base::ScopedClosureRunner completion_closure_runner(completion_closure);

  // A reason to automatically unsubscribe. UNKNOWN means do not unsubscribe.
  content::mojom::PushUnregistrationReason unsubscribe_reason =
      content::mojom::PushUnregistrationReason::UNKNOWN;

  // TODO(mvanouwerkerk): Show a warning in the developer console of the
  // Service Worker corresponding to app_id (and/or on an internals page).
  // See https://crbug.com/508516 for options.
  switch (status) {
    case content::mojom::PushDeliveryStatus::SUCCESS:
    case content::mojom::PushDeliveryStatus::EVENT_WAITUNTIL_REJECTED:
      break;
    case content::mojom::PushDeliveryStatus::SERVICE_WORKER_ERROR:
      // Do nothing, and hope the error is transient.
      break;
    case content::mojom::PushDeliveryStatus::UNKNOWN_APP_ID:
      unsubscribe_reason =
          content::mojom::PushUnregistrationReason::DELIVERY_UNKNOWN_APP_ID;
      break;
    case content::mojom::PushDeliveryStatus::PERMISSION_DENIED:
      unsubscribe_reason =
          content::mojom::PushUnregistrationReason::DELIVERY_PERMISSION_DENIED;
      break;
    case content::mojom::PushDeliveryStatus::NO_SERVICE_WORKER:
      unsubscribe_reason =
          content::mojom::PushUnregistrationReason::DELIVERY_NO_SERVICE_WORKER;
      break;
    default:
      NOTREACHED();
      break;
  }

  if (unsubscribe_reason != content::mojom::PushUnregistrationReason::UNKNOWN) {
    UnsubscribeInternal(unsubscribe_reason, requesting_origin,
                        service_worker_registration_id, app_id,
                        base::Bind(&UnregisterCallbackToClosure,
                                   base::AdaptCallbackForRepeating(
                                       completion_closure_runner.Release())));
  }
}

void PushMessagingServiceImplEfl::DidHandleMessage(
    const std::string& app_id,
    const base::Closure& message_handled_closure) {
  message_handled_closure.Run();
}

void PushMessagingServiceImplEfl::SetMessageCallbackForTesting(
    const base::Closure& callback) {
  message_callback_for_testing_ = callback;
}

// GetEndpoint method ----------------------------------------------------------

GURL PushMessagingServiceImplEfl::GetEndpoint(bool standard_protocol) const {
  // We inherit this from upstream.
  // In this place we cannot return correct registration id
  // This dummy url is changed inside
  // push_messaging_message_filter.cc
  // for correct url and registration id
  return GURL("http://dummy_push_endpoint.com");
}

GURL PushMessagingServiceImplEfl::GetPushEndpointForRegId(
    const std::string& reg_id) {
  return GURL(SppClient::GetPushEndpointForRegId(reg_id));
}

// Subscribe and GetPermissionStatus methods -----------------------------------

void PushMessagingServiceImplEfl::SubscribeFromDocument(
    const GURL& requesting_origin,
    int64_t service_worker_registration_id,
    int renderer_id,
    int render_frame_id,
    const content::PushSubscriptionOptions& options,
    bool user_gesture,
    const RegisterCallback& callback) {
  PushMessagingAppIdentifierEfl app_identifier =
      PushMessagingAppIdentifierEfl::Generate(requesting_origin,
                                              service_worker_registration_id);

  content::RenderFrameHost* render_frame_host =
      content::RenderFrameHost::FromID(renderer_id, render_frame_id);
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  if (!web_contents)
    return;

  if (!options.user_visible_only) {
    web_contents->GetMainFrame()->AddMessageToConsole(
        content::CONSOLE_MESSAGE_LEVEL_ERROR, kSilentPushUnsupportedMessage);

    SubscribeEndWithError(
        callback, content::mojom::PushRegistrationStatus::PERMISSION_DENIED);
    return;
  }

  // Push does not allow permission requests from iframes.
  context_->GetPermissionManager()->RequestPermission(
      content::PermissionType::PUSH_MESSAGING, web_contents->GetMainFrame(),
      requesting_origin, true /* user_gesture */,
      base::Bind(&PushMessagingServiceImplEfl::DoSubscribe,
                 base::Unretained(this), app_identifier, options.sender_info,
                 callback));
}

void PushMessagingServiceImplEfl::SubscribeFromWorker(
    const GURL& requesting_origin,
    int64_t service_worker_registration_id,
    const content::PushSubscriptionOptions& options,
    const RegisterCallback& register_callback) {
  PushMessagingAppIdentifierEfl app_identifier =
      PushMessagingAppIdentifierEfl::Generate(requesting_origin,
                                              service_worker_registration_id);

  blink::WebPushPermissionStatus permission_status =
      GetPermissionStatus(requesting_origin, options.user_visible_only);
  if (permission_status != blink::kWebPushPermissionStatusGranted) {
    SubscribeEndWithError(
        register_callback,
        content::mojom::PushRegistrationStatus::PERMISSION_DENIED);
    return;
  }

  DoSubscribe(app_identifier, options.sender_info, register_callback,
              blink::mojom::PermissionStatus::GRANTED);
}

blink::WebPushPermissionStatus PushMessagingServiceImplEfl::GetPermissionStatus(
    const GURL& origin,
    bool user_visible) {
  if (!user_visible)
    return blink::kWebPushPermissionStatusDenied;

  return ToWebPushPermission(
      context_->GetPermissionManager()->GetPermissionStatus(
          content::PermissionType::PUSH_MESSAGING, origin, origin));
}

bool PushMessagingServiceImplEfl::SupportNonVisibleMessages() {
  return false;
};

void PushMessagingServiceImplEfl::DoSubscribe(
    const PushMessagingAppIdentifierEfl& app_identifier,
    const std::string& sender_id,
    const RegisterCallback& register_callback,
    blink::mojom::PermissionStatus permission_status) {
  if (permission_status != blink::mojom::PermissionStatus::GRANTED) {
    SubscribeEndWithError(
        register_callback,
        content::mojom::PushRegistrationStatus::PERMISSION_DENIED);
    return;
  }

  GetSppDriver()->Register(
      app_identifier.app_id(), sender_id,
      base::Bind(&PushMessagingServiceImplEfl::DidSubscribe,
                 base::Unretained(this), app_identifier, register_callback));
}

void PushMessagingServiceImplEfl::SubscribeEnd(
    const RegisterCallback& callback,
    const std::string& subscription_id,
    const std::vector<uint8_t>& p256dh,
    const std::vector<uint8_t>& auth,
    content::mojom::PushRegistrationStatus status) {
  callback.Run(subscription_id, p256dh, auth, status);
}

void PushMessagingServiceImplEfl::SubscribeEndWithError(
    const RegisterCallback& callback,
    content::mojom::PushRegistrationStatus status) {
  SubscribeEnd(callback, std::string() /* subscription_id */,
               std::vector<uint8_t>() /* p256dh */,
               std::vector<uint8_t>() /* auth */, status);
}

void PushMessagingServiceImplEfl::DidSubscribe(
    const PushMessagingAppIdentifierEfl& app_identifier,
    const RegisterCallback& callback,
    const std::string& subscription_id,
    sppclient::PushRegistrationResult result) {
  if (result != sppclient::PushRegistrationResult::SUCCESS) {
    SubscribeEndWithError(callback, ToPushRegistrationStatus(result));
    return;
  }

  std::string p256dh;
  std::string auth_secret;

  GetSppDriver()->GetEncryptionInfo(
      app_identifier.app_id(),
      base::Bind(&PushMessagingServiceImplEfl::DidSubscribeWithEncryptionInfo,
                 base::Unretained(this), callback, subscription_id));
}

void PushMessagingServiceImplEfl::DidSubscribeWithEncryptionInfo(
    const RegisterCallback& callback,
    const std::string& subscription_id,
    const std::string& p256dh,
    const std::string& auth_secret,
    sppclient::PushRegistrationResult result) {
  if (result != sppclient::PushRegistrationResult::SUCCESS) {
    SubscribeEndWithError(
        callback,
        content::mojom::PushRegistrationStatus::PUBLIC_KEY_UNAVAILABLE);
    return;
  }

  SubscribeEnd(
      callback, subscription_id,
      std::vector<uint8_t>(p256dh.begin(), p256dh.end()),
      std::vector<uint8_t>(auth_secret.begin(), auth_secret.end()),
      content::mojom::PushRegistrationStatus::SUCCESS_FROM_PUSH_SERVICE);
}

// GetEncryptionInfo methods ---------------------------------------------------

void PushMessagingServiceImplEfl::GetSubscriptionInfo(
    const GURL& origin,
    int64_t service_worker_registration_id,
    const std::string& sender_id,
    const std::string& subscription_id,
    const SubscriptionInfoCallback& callback) {
  PushMessagingAppIdentifierEfl app_identifier =
      PushMessagingAppIdentifierEfl::Generate(origin,
                                              service_worker_registration_id);
  DCHECK(!app_identifier.is_null());

  GetSppDriver()->GetEncryptionInfo(
      app_identifier.app_id(),
      base::Bind(&PushMessagingServiceImplEfl::DidGetEncryptionInfo,
                 base::Unretained(this), app_identifier, callback));
}

void PushMessagingServiceImplEfl::DidGetEncryptionInfo(
    const PushMessagingAppIdentifierEfl& app_identifier,
    const SubscriptionInfoCallback& callback,
    const std::string& p256dh,
    const std::string& auth_secret,
    sppclient::PushRegistrationResult result) {
#if 0
  // TODO(is46.kim) : needs to check https://review.tizen.org/gerrit/#/c/115290/
  if (result == sppclient::PushRegistrationResult::NOT_REGISTERED) {
    base::Closure unsubscribe_closure = base::Bind(
        callback, false, std::vector<uint8_t>(), std::vector<uint8_t>());
    UnsubscribeInternal(
        content::mojom::PushUnregistrationReason::GCM_STORE_RESET,
        app_identifier.origin(),
        app_identifier.service_worker_registration_id(),
        app_identifier.is_null() ? std::string() : app_identifier.app_id(),
        base::Bind(&UnregisterCallbackToClosure, unsubscribe_closure));
    return;
  }
#endif

  const bool success = (result == sppclient::PushRegistrationResult::SUCCESS);
  callback.Run(success, std::vector<uint8_t>(p256dh.begin(), p256dh.end()),
               std::vector<uint8_t>(auth_secret.begin(), auth_secret.end()));
}

// Unsubscribe methods ---------------------------------------------------------

void PushMessagingServiceImplEfl::Unsubscribe(
    content::mojom::PushUnregistrationReason reason,
    const GURL& requesting_origin,
    int64_t service_worker_registration_id,
    const std::string& sender_id,
    const UnregisterCallback& callback) {
  PushMessagingAppIdentifierEfl app_identifier =
      PushMessagingAppIdentifierEfl::Generate(requesting_origin,
                                              service_worker_registration_id);

  UnsubscribeInternal(
      content::mojom::PushUnregistrationReason::JAVASCRIPT_API,
      requesting_origin, service_worker_registration_id,
      app_identifier.is_null() ? std::string() : app_identifier.app_id(),
      callback);
}

void PushMessagingServiceImplEfl::UnsubscribeInternal(
    content::mojom::PushUnregistrationReason reason,
    const GURL& origin,
    int64_t service_worker_registration_id,
    const std::string& app_id,
    const UnregisterCallback& callback) {
  DCHECK(!app_id.empty() || (!origin.is_empty() &&
                             service_worker_registration_id !=
                                 -1 /* kInvalidServiceWorkerRegistrationId */))
      << "Need an app_id and/or origin+service_worker_registration_id";

  if (origin.is_empty() || service_worker_registration_id ==
                               -1 /* kInvalidServiceWorkerRegistrationId */) {
    // Can't clear Service Worker database.
    DidClearPushSubscriptionId(reason, app_id, callback);
    return;
  }
  ClearPushSubscriptionId(
      context_, origin, service_worker_registration_id,
      base::Bind(&PushMessagingServiceImplEfl::DidClearPushSubscriptionId,
                 base::Unretained(this), reason, app_id, callback));
}

void PushMessagingServiceImplEfl::DidClearPushSubscriptionId(
    content::mojom::PushUnregistrationReason reason,
    const std::string& app_id,
    const UnregisterCallback& callback) {
  if (app_id.empty()) {
    // Without an |app_id|, we can neither delete the subscription from the
    // PushMessagingAppIdentifier map, nor unsubscribe with the GCM Driver.
    callback.Run(
        content::mojom::PushUnregistrationStatus::SUCCESS_WAS_NOT_REGISTERED);
    return;
  }

  const auto& unregister_callback =
      base::Bind(&PushMessagingServiceImplEfl::DidUnsubscribe,
                 base::Unretained(this), callback);
  GetSppDriver()->Unregister(app_id, unregister_callback);
}

void PushMessagingServiceImplEfl::DidUnsubscribe(
    const UnregisterCallback& callback,
    sppclient::PushRegistrationResult result) {
  if (callback.is_null()) {
    // Internal unregistrations not always pass a valid callback.
    return;
  }
  callback.Run(ToPushUnregistrationStatus(result));
}

// DidDeleteServiceWorkerDatabase methods --------------------------------------

void PushMessagingServiceImplEfl::DidDeleteServiceWorkerRegistration(
    const GURL& origin,
    int64_t service_worker_registration_id) {}

void PushMessagingServiceImplEfl::DidDeleteServiceWorkerDatabase() {}

// OnContentSettingChanged methods ---------------------------------------------

void PushMessagingServiceImplEfl::OnContentSettingChanged(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsType content_type,
    std::string resource_identifier) {
  if (content_type != CONTENT_SETTINGS_TYPE_NOTIFICATIONS)
    return;

  base::Closure done_closure =
      content_setting_changed_callback_for_testing_.is_null()
          ? base::Bind(&base::DoNothing)
          : content_setting_changed_callback_for_testing_;

  GetSppDriver()->ForEachRegistration(
      base::Bind(
          &PushMessagingServiceImplEfl::OnContentSettingChangedForRegistration,
          base::Unretained(this), primary_pattern),
      done_closure);
}

void PushMessagingServiceImplEfl::OnContentSettingChangedForRegistration(
    const ContentSettingsPattern& primary_pattern,
    const std::string& app_id,
    const base::Closure& barrier_closure) {
  PushMessagingAppIdentifierEfl app_identifier =
      PushMessagingAppIdentifierEfl::Generate(app_id);

  // If |primary_pattern| is not valid, we should always check for a
  // permission change because it can happen for example when the entire
  // Push or Notifications permissions are cleared.
  // Otherwise, the permission should be checked if the pattern matches the
  // origin.
  if (primary_pattern.IsValid() &&
      !primary_pattern.Matches(app_identifier.origin())) {
    barrier_closure.Run();
    return;
  }

  if (IsPermissionSet(app_identifier.origin())) {
    barrier_closure.Run();
    return;
  }

  UnsubscribeInternal(
      content::mojom::PushUnregistrationReason::PERMISSION_REVOKED,
      app_identifier.origin(), app_identifier.service_worker_registration_id(),
      app_identifier.app_id(),
      base::Bind(&UnregisterCallbackToClosure, barrier_closure));
}

void PushMessagingServiceImplEfl::SetContentSettingChangedCallbackForTesting(
    const base::Closure& callback) {
  content_setting_changed_callback_for_testing_ = callback;
}

// Helper methods --------------------------------------------------------------

// Assumes user_visible always since this is just meant to check
// if the permission was previously granted and not revoked.
bool PushMessagingServiceImplEfl::IsPermissionSet(const GURL& origin) {
  return GetPermissionStatus(origin, true /* user_visible */) ==
         blink::kWebPushPermissionStatusGranted;
}
