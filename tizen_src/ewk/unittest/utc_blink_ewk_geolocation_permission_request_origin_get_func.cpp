// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/* Define those macros _before_ you include the utc_blink_ewk.h header file. */

#include "utc_blink_ewk_base.h"

class utc_blink_ewk_geolocation_permission_request_origin_get : public utc_blink_ewk_base {

protected:
 void PostSetUp() override {
   ewk_view_geolocation_permission_callback_set(GetEwkWebView(), nullptr,
                                                nullptr);
   evas_object_smart_callback_add(GetEwkWebView(),
                                  "geolocation,permission,request",
                                  request_geolocation_permisson, this);
  }

  void PreTearDown() override {
    evas_object_smart_callback_del(GetEwkWebView(),"geolocation,permission,request", request_geolocation_permisson);
  }

  void ConsoleMessage(Evas_Object* webview,
                      const Ewk_Console_Message* msg) override {
    EventLoopStop(Failure);
  }

  static void request_geolocation_permisson(void* data, Evas_Object* webview, void* event_info)
  {
    utc_message("[request_geolocation_permisson]");
    utc_blink_ewk_geolocation_permission_request_origin_get *owner = static_cast<utc_blink_ewk_geolocation_permission_request_origin_get*>(data);

    Ewk_Geolocation_Permission_Request* permission_request = (Ewk_Geolocation_Permission_Request*)event_info;
    ASSERT_NE(nullptr, permission_request);

    const Ewk_Security_Origin* security_origin = ewk_geolocation_permission_request_origin_get(permission_request);
    ASSERT_NE(nullptr, security_origin);

    // Just to be sure returned struct is a valid Ewk_Security_Origin object
    ASSERT_NE(nullptr, ewk_security_origin_host_get(security_origin));
    ASSERT_NE(nullptr, ewk_security_origin_protocol_get(security_origin));
    owner->EventLoopStop(Success);
  }
};

/**
 * @brief Positive test whether ewk_geolocation_permission_request_origin_get returns valid object
 */
TEST_F(utc_blink_ewk_geolocation_permission_request_origin_get, POS_TEST)
{
  ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(), GetResourceUrl("/common/sample_js_geolocation.html").c_str()));
  ASSERT_EQ(Success, EventLoopStart());
}

/**
 * @brief Negative test whether API behaves properly with invalid argument.
 */
TEST_F(utc_blink_ewk_geolocation_permission_request_origin_get, NEG_TEST)
{
  ASSERT_EQ(nullptr, ewk_geolocation_permission_request_origin_get(nullptr));
}
