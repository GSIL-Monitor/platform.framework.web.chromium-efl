// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_notification_test_base.h"

void utc_blink_ewk_notification_test_base::PostSetUp() {
  ewk_view_notification_permission_callback_set(
      GetEwkWebView(),
      reinterpret_cast<Ewk_View_Notification_Permission_Callback>(
          notification_permission_request_callback),
      this);
  ewk_notification_callbacks_set(
      reinterpret_cast<Ewk_Notification_Show_Callback>(
          notification_show_callback),
      reinterpret_cast<Ewk_Notification_Cancel_Callback>(
          notification_cancel_callback),
      this);
}

void utc_blink_ewk_notification_test_base::PreTearDown() {
  ewk_view_notification_permission_callback_set(GetEwkWebView(), NULL, NULL);
  ewk_notification_callbacks_reset();

  if (security_origins) {
    // if it fails, than other TCs can produce false negatives/positives
    EXPECT_EQ(EINA_TRUE, ewk_notification_policies_removed(security_origins));

    void* data = NULL;

    EINA_LIST_FREE(security_origins, data) {
      Ewk_Security_Origin* origin = static_cast<Ewk_Security_Origin*>(data);
      ewk_security_origin_free(origin);
    }
  }
}

Eina_Bool utc_blink_ewk_notification_test_base::NotificationPermissionRequest(
    Evas_Object* webview,
    Ewk_Notification_Permission_Request* request) {
  // allow the notification by default
  EXPECT_EQ(EINA_TRUE, ewk_notification_permission_reply(request, EINA_TRUE));
  return EINA_TRUE;
}
