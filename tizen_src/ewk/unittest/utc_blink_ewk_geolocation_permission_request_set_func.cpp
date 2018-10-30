// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/* Define those macros _before_ you include the utc_blink_ewk.h header file. */

#include "utc_blink_ewk_base.h"

class utc_blink_ewk_geolocation_permission_request_set : public utc_blink_ewk_base {

protected:
 void PostSetUp() override {
   ewk_view_geolocation_permission_callback_set(GetEwkWebView(), nullptr,
                                                nullptr);
   permissionDecision = false;
   evas_object_smart_callback_add(GetEwkWebView(),
                                  "geolocation,permission,request",
                                  request_geolocation_permisson, this);
  }

  void PreTearDown() override {
    evas_object_smart_callback_del(GetEwkWebView(),"geolocation,permission,request", request_geolocation_permisson);
  }

  void ConsoleMessage(Evas_Object* webview,
                      const Ewk_Console_Message* msg) override {
    utc_blink_ewk_base::ConsoleMessage(webview, msg);

    const char* message = ewk_console_message_text_get(msg);
    consoleMessage = message ? message : "";
    EventLoopStop(Failure);
  }

  static void request_geolocation_permisson(void* data, Evas_Object* webview, void* event_info)
  {
    utc_message("[request_geolocation_permisson]");
    utc_blink_ewk_geolocation_permission_request_set *owner = static_cast<utc_blink_ewk_geolocation_permission_request_set*>(data);

    Ewk_Geolocation_Permission_Request* permission_request = (Ewk_Geolocation_Permission_Request*)event_info;
    owner->EventLoopStop(permission_request
                         && ewk_geolocation_permission_request_set(permission_request, owner->permissionDecision) ?
                             Success : Failure);
  }

protected:
  std::string consoleMessage;
  bool permissionDecision;
};

/**
 * @brief Positive test whether ewk_geolocation_permission_request_set works.
 */
TEST_F(utc_blink_ewk_geolocation_permission_request_set, POS_TEST)
{
  ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(), GetResourceUrl("/common/sample_js_geolocation.html").c_str()));

  permissionDecision = true;
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_EQ(Failure, EventLoopStart(5));
#ifdef OS_TIZEN
  ASSERT_STREQ("EWK Geolocation SUCCESS", consoleMessage.c_str());
#else
  ASSERT_STREQ("EWK Geolocation POSITION_UNAVAILABLE", consoleMessage.c_str());
#endif
  ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(), GetResourceUrl("/common/sample_js_geolocation.html").c_str()));

  permissionDecision = false;
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_EQ(Failure, EventLoopStart(5));
  ASSERT_STREQ("EWK Geolocation PERMISSION_DENIED", consoleMessage.c_str());
}

/**
 * @brief Negative test whether ewk_geolocation_permission_request_set behaves correctly for invalid arguments.
 */
TEST_F(utc_blink_ewk_geolocation_permission_request_set, NEG_TEST)
{
  ASSERT_EQ(EINA_FALSE, ewk_geolocation_permission_request_set(nullptr, EINA_TRUE));
  ASSERT_EQ(EINA_FALSE, ewk_geolocation_permission_request_set(nullptr, EINA_FALSE));
}
