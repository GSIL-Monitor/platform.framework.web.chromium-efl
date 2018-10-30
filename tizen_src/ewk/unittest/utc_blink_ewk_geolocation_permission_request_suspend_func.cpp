// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/* Define those macros _before_ you include the utc_blink_ewk.h header file. */

#include "utc_blink_ewk_base.h"

static const char* PERMISSION_DENIED = "EWK Geolocation PERMISSION_DENIED";
static const char* POSITION_UNAVAILABLE = "EWK Geolocation POSITION_UNAVAILABLE";
static const char* GEOLOCATION_SUCCESS = "EWK Geolocation SUCCESS";

#define SAMPLE_HTML_FILE                 ("/common/sample_js_geolocation.html")

class utc_blink_ewk_geolocation_permission_request_suspend : public utc_blink_ewk_base {

protected:

  Eina_Bool call_request_suspend;

  void PostSetUp() override {
    ewk_view_geolocation_permission_callback_set(GetEwkWebView(), nullptr,
                                                 nullptr);
    evas_object_smart_callback_add(GetEwkWebView(),"geolocation,permission,request", request_geolocation_permisson, this);
  }

  void PreTearDown() override {
    evas_object_smart_callback_del(GetEwkWebView(),"geolocation,permission,request", request_geolocation_permisson);
  }

  void ConsoleMessage(Evas_Object* webview,
                      const Ewk_Console_Message* msg) override {
    const char* message = ewk_console_message_text_get(msg);
    if (call_request_suspend) {
// Positive test
#ifdef OS_TIZEN
      if (!strcmp(GEOLOCATION_SUCCESS, message))
#else
      if (!strcmp(POSITION_UNAVAILABLE, message))
#endif
        EventLoopStop(Success);
      else
        EventLoopStop(Failure);
    } else {
      // Negative test:
      // The permission should be denied.
      if (!strcmp(PERMISSION_DENIED, message)) {
        EventLoopStop(Success);
      }
    }
  }

  static Eina_Bool geolocation_permision_request_set_wrapper(void* permission_request)
  {
     Eina_Bool ret = ewk_geolocation_permission_request_set((Ewk_Geolocation_Permission_Request*)permission_request, EINA_TRUE);

     EXPECT_TRUE(ret);

     //return ECORE_CALLBACK_CANCEL to stop calling function by timer.
     return ECORE_CALLBACK_CANCEL;
  }

  static void request_geolocation_permisson(void* data, Evas_Object* webview, void* event_info)
  {
    utc_message("[request_geolocation_permisson] :: \n");
    utc_blink_ewk_geolocation_permission_request_suspend *owner = static_cast<utc_blink_ewk_geolocation_permission_request_suspend*>(data);
    Ewk_Geolocation_Permission_Request* permission_request = (Ewk_Geolocation_Permission_Request*)event_info;

    /*
     * If call_request_suspend is equal EINA_TRUE then
     * ewk_geolocation_permission_request_suspend is invoke, otherwise is not call.
     */
    if (owner->call_request_suspend) {

      ewk_geolocation_permission_request_suspend(permission_request);
      ecore_timer_add(2, geolocation_permision_request_set_wrapper, permission_request);
    }
  }
};

/**
* @brief Positive test whether geolocation permission request is successfully suspended.
* When geolocation permission request is suspended then ewk_geolocation_permission_request_set can be invoked
* after the request_geolocation_permisson callback. If geolocation permission request will be set to EINA_TRUE
* then geolocation position should be successfully obtained or if the current GPS position is unavailable then
* geolocation position should be UNAVAILABLE.
*/
TEST_F(utc_blink_ewk_geolocation_permission_request_suspend, POS_TEST)
{
//TODO: this code should use ewk_geolocation_permission_request_suspend and check its behaviour.
//Results should be reported using one of utc_ macros */
  call_request_suspend = EINA_TRUE;

  Eina_Bool result = ewk_view_url_set(GetEwkWebView(), GetResourceUrl(SAMPLE_HTML_FILE).c_str());
  if (!result)
    FAIL();

  utc_blink_ewk_base::MainLoopResult main_result = EventLoopStart();
  if (main_result != utc_blink_ewk_base::Success)
    FAIL();
}

/**
* @brief Negative test whether geolocation errorCallback is invoked when ewk_geolocation_permission_request_suspend is NOT called.
* When call_request_suspend is equal EINA_FALSE then ewk_geolocation_permission_request_suspend is NOT invoked in request_geolocation_permisson
* callback. Because geolocation permission request is NOT suspend, thus errorCallback with PERMISSION_DENIED error code should be invoked.
*/
TEST_F(utc_blink_ewk_geolocation_permission_request_suspend, NEG_TEST)
{
//TODO: this code should use ewk_geolocation_permission_request_suspend and check its behaviour.
//Results should be reported using one of utc_ macros */

  ewk_geolocation_permission_request_suspend(NULL);
}
