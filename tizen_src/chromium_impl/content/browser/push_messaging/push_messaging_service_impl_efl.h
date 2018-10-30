// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMIUM_IMPL_CONTENT_PUSH_MESSAGING_PUSH_MESSAGING_SERVICE_IMPL_EFL_H_
#define CHROMIUM_IMPL_CONTENT_PUSH_MESSAGING_PUSH_MESSAGING_SERVICE_IMPL_EFL_H_

#include <set>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/content_settings/core/browser/content_settings_observer.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/spp_client/spp_client.h"
#include "content/public/browser/push_messaging_service.h"
#include "content/public/common/push_event_payload.h"
#include "third_party/WebKit/public/platform/modules/permissions/permission_status.mojom.h"
#include "third_party/WebKit/public/platform/modules/push_messaging/WebPushPermissionStatus.h"

using sppclient::SppClient;

class PushMessagingAppIdentifierEfl;
class PushMessagingPermissionContext;

using RegisterCallback = content::PushMessagingService::RegisterCallback;
using UnregisterCallback = content::PushMessagingService::UnregisterCallback;

namespace content {
class ServiceWorkerContextWrapper;
namespace mojom {
enum class PushDeliveryStatus;
enum class PushRegistrationStatus;
}  // namespace mojom
}  // namespace content

class PushMessagingServiceImplEfl : public content::PushMessagingService,
                                    public sppclient::SppClient::Delegate,
                                    public content_settings::Observer,
                                    public KeyedService {
 public:
  // If any Service Workers are using push, starts SPP and adds an app handler.
  static void InitializeForContext(content::BrowserContext* context);

  explicit PushMessagingServiceImplEfl(content::BrowserContext* context);
  ~PushMessagingServiceImplEfl() override;

  // SppClient::Delegate implementation
  void OnMessage(const std::string& app_id,
                 const std::string& sender_id,
                 const std::string& data) override;

  // content::PushMessagingService implementation:
  GURL GetEndpoint(bool standard_protocol) const override;
  void SubscribeFromDocument(const GURL& requesting_origin,
                             int64_t service_worker_registration_id,
                             int renderer_id,
                             int render_frame_id,
                             const content::PushSubscriptionOptions& options,
                             bool user_gesture,
                             const RegisterCallback& callback) override;
  void SubscribeFromWorker(const GURL& requesting_origin,
                           int64_t service_worker_registration_id,
                           const content::PushSubscriptionOptions& options,
                           const RegisterCallback& callback) override;
  void GetSubscriptionInfo(const GURL& origin,
                           int64_t service_worker_registration_id,
                           const std::string& sender_id,
                           const std::string& subscription_id,
                           const SubscriptionInfoCallback& callback) override;
  void Unsubscribe(content::mojom::PushUnregistrationReason reason,
                   const GURL& requesting_origin,
                   int64_t service_worker_registration_id,
                   const std::string& sender_id,
                   const UnregisterCallback&) override;
  blink::WebPushPermissionStatus GetPermissionStatus(
      const GURL& origin,
      bool user_visible) override;
  bool SupportNonVisibleMessages() override;
  void DidDeleteServiceWorkerRegistration(
      const GURL& origin,
      int64_t service_worker_registration_id) override;
  void DidDeleteServiceWorkerDatabase() override;

  // content::PushMessagingService efl implementation:
  GURL GetPushEndpointForRegId(const std::string& reg_id) override;

  // content_settings::Observer implementation.
  void OnContentSettingChanged(const ContentSettingsPattern& primary_pattern,
                               const ContentSettingsPattern& secondary_pattern,
                               ContentSettingsType content_type,
                               std::string resource_identifier) override;

  void DeliverMessage(const std::string& push_data);
  void SetMessageCallbackForTesting(const base::Closure& callback);
  void SetContentSettingChangedCallbackForTesting(
      const base::Closure& callback);

 private:
  using self = PushMessagingServiceImplEfl;

  // OnMessage methods ---------------------------------------------------------

  void DeliverMessageCallback(const std::string& app_id,
                              const GURL& requesting_origin,
                              int64_t service_worker_registration_id,
                              const std::string& message,
                              const base::Closure& message_handled_closure,
                              content::mojom::PushDeliveryStatus status);

  void DidHandleMessage(const std::string& app_id,
                        const base::Closure& completion_closure);

  void DeliverMessage(const PushMessagingAppIdentifierEfl& app_identifier,
                      const std::string& data);

  // Subscribe methods ---------------------------------------------------------

  void DoSubscribe(const PushMessagingAppIdentifierEfl& app_identifier,
                   const std::string& sender_id,
                   const RegisterCallback& register_callback,
                   blink::mojom::PermissionStatus permission_status);

  void SubscribeEnd(const RegisterCallback& callback,
                    const std::string& subscription_id,
                    const std::vector<uint8_t>& p256dh,
                    const std::vector<uint8_t>& auth,
                    content::mojom::PushRegistrationStatus status);

  void SubscribeEndWithError(const RegisterCallback& callback,
                             content::mojom::PushRegistrationStatus status);

  void DidSubscribe(const PushMessagingAppIdentifierEfl& app_identifier,
                    const RegisterCallback& callback,
                    const std::string& registration_id,
                    sppclient::PushRegistrationResult result);

  void DidSubscribeWithEncryptionInfo(const RegisterCallback& callback,
                                      const std::string& subscription_id,
                                      const std::string& p256dh,
                                      const std::string& auth_secret,
                                      sppclient::PushRegistrationResult result);

  // GetEncryptionInfo method --------------------------------------------------

  void DidGetEncryptionInfo(const PushMessagingAppIdentifierEfl& app_identifier,
                            const SubscriptionInfoCallback& callback,
                            const std::string& p256dh,
                            const std::string& auth_secret,
                            sppclient::PushRegistrationResult result);

  // Unsubscribe methods -------------------------------------------------------

  // |origin|, |service_worker_registration_id| and |app_id| should be provided
  // whenever they can be obtained. It's valid for |origin| to be empty and
  // |service_worker_registration_id| to be kInvalidServiceWorkerRegistrationId,
  // or for app_id to be empty, but not both at once.
  void UnsubscribeInternal(content::mojom::PushUnregistrationReason reason,
                           const GURL& origin,
                           int64_t service_worker_registration_id,
                           const std::string& app_id,
                           const UnregisterCallback& callback);

  void DidClearPushSubscriptionId(
      content::mojom::PushUnregistrationReason reason,
      const std::string& app_id,
      const UnregisterCallback& callback);

  void DidUnsubscribe(const UnregisterCallback& callback,
                      sppclient::PushRegistrationResult result);

  // OnContentSettingChanged methods -------------------------------------------

  void OnContentSettingChangedForRegistration(
      const ContentSettingsPattern& primary_pattern,
      const std::string& app_id,
      const base::Closure& barrier_closure);

  // Helper methods -----------------------------------------------------------

  // Checks if a given origin is allowed to use Push.
  bool IsPermissionSet(const GURL& origin);

  sppclient::SppClient* GetSppDriver() const { return driver_.get(); }

  std::unique_ptr<sppclient::SppClient> driver_;

  content::BrowserContext* context_;

  base::Closure message_callback_for_testing_;
  base::Closure content_setting_changed_callback_for_testing_;

  DISALLOW_COPY_AND_ASSIGN(PushMessagingServiceImplEfl);
};

#endif  // CHROMIUM_IMPL_CONTENT_PUSH_MESSAGING_PUSH_MESSAGING_SERVICE_IMPL_EFL_H_
