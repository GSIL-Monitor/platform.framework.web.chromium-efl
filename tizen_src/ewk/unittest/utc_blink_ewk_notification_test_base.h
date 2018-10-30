// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

class utc_blink_ewk_notification_test_base : public utc_blink_ewk_base
{
 protected:
  utc_blink_ewk_notification_test_base()
    : security_origins(NULL)
    , notification_sample_1(GetResourceUrl("/common/sample_notification_1.html"))
  {}

  /* Common setup */
  void PostSetUp() override;
  void PreTearDown() override;

  void NotificationShow(Ewk_Notification* notification) override {}

  void NotificationCancel(uint64_t notificationId) override {}

  Eina_Bool NotificationPermissionRequest(
      Evas_Object* webview,
      Ewk_Notification_Permission_Request* request) override;

  static Eina_Bool notification_permission_request_callback(Evas_Object* webview, Ewk_Notification_Permission_Request* request, utc_blink_ewk_notification_test_base* owner)
  {
    utc_message("[notification_permission_request_callback] :: ");
    if (!owner) {
      return EINA_FALSE;
    }

    if (!request) {
      owner->EventLoopStop(Failure);
      return EINA_FALSE;
    }

    const Ewk_Security_Origin *perm_origin = ewk_notification_permission_request_origin_get(request);

    if (perm_origin) {
      std::string proto, host;

      if (ewk_security_origin_protocol_get(perm_origin)) {
        proto = ewk_security_origin_protocol_get(perm_origin);
      }

      if (ewk_security_origin_host_get(perm_origin)) {
        host = ewk_security_origin_host_get(perm_origin);
      }

      Ewk_Security_Origin *origin = ewk_security_origin_new_from_string((proto + "://" + host).c_str());
      owner->security_origins = eina_list_append(owner->security_origins, origin);
    }

    return owner->NotificationPermissionRequest(webview, request);
  }

  static void notification_show_callback(Ewk_Notification* notification, utc_blink_ewk_notification_test_base* owner)
  {
    utc_message("[notification_show_callback] :: ");
    ASSERT_TRUE(owner);

    if (!notification) {
      owner->EventLoopStop(Failure);
      ASSERT_TRUE(notification);
    }

    owner->NotificationShow(notification);
  }

  static void notification_cancel_callback(uint64_t notificationId, utc_blink_ewk_notification_test_base* owner)
  {
    utc_message("[notification_cancel_callback] :: ");
    ASSERT_TRUE(owner);

    if (!notificationId) {
      owner->EventLoopStop(Failure);
      ASSERT_TRUE(notificationId);
    }

    owner->NotificationCancel(notificationId);
  }

 protected:
  Eina_List* security_origins;
  const std::string notification_sample_1;
};
