// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

#define SAMPLE_LOG_HTML_FILE        ("/ewk_view/console_log.html")
#define SAMPLE_WARN_HTML_FILE        ("/ewk_view/console_warn.html")
#define SAMPLE_ERROR_HTML_FILE        ("/ewk_view/console_error.html")
#define SAMPLE_DEBUG_HTML_FILE        ("/ewk_view/console_debug.html")
#define SAMPLE_INFO_HTML_FILE        ("/ewk_view/console_info.html")

class utc_blink_ewk_console_message_level_get : public utc_blink_ewk_base
{

protected:

  int gResult;

  void PostSetUp() override {
    evas_object_smart_callback_add(GetEwkWebView(),"console,message",console_message_cb, this);
  }

  void PreTearDown() override {
    evas_object_smart_callback_del(GetEwkWebView(),"console,message",console_message_cb);
  }

  /* Callback for console message */
  static void console_message_cb(void* data, Evas_Object* webview, void* event_info)
  {
    utc_message("[console,message] :: \n");
    utc_blink_ewk_console_message_level_get* owner = static_cast<utc_blink_ewk_console_message_level_get*>(data);

    Ewk_Console_Message* console = (Ewk_Console_Message*)event_info;
    if(console) {

      owner->gResult = ewk_console_message_level_get(console);   
      owner->EventLoopStop(utc_blink_ewk_base::Success);
    }
  }
};

/**
 * @brief Tests ewk_console_message_level_get incase Ewk_Console_Message-log message exists
 */
TEST_F(utc_blink_ewk_console_message_level_get, POS_TEST1)
{
  gResult = -1;
  Eina_Bool result = ewk_view_url_set(GetEwkWebView(), GetResourceUrl(SAMPLE_LOG_HTML_FILE).c_str());
  if (!result)
    FAIL();

  utc_blink_ewk_base::MainLoopResult main_result = EventLoopStart();
  if (main_result != utc_blink_ewk_base::Success)
    FAIL();

  utc_check_eq(gResult, 0);
}

/**
 * @brief Tests ewk_console_message_level_get incase Ewk_Console_Message-warning message exists
 */
TEST_F(utc_blink_ewk_console_message_level_get, POS_TEST2)
{
  gResult = 0;
  Eina_Bool result = ewk_view_url_set(GetEwkWebView(), GetResourceUrl(SAMPLE_WARN_HTML_FILE).c_str());
  if (!result)
    FAIL();

  utc_blink_ewk_base::MainLoopResult main_result = EventLoopStart();
  if (main_result != utc_blink_ewk_base::Success)
    FAIL();

  utc_check_eq(gResult, 1);
}

/**
 * @brief Tests ewk_console_message_level_get incase Ewk_Console_Message-error message exists
 */
TEST_F(utc_blink_ewk_console_message_level_get, POS_TEST3)
{
  gResult = 0;
  Eina_Bool result = ewk_view_url_set(GetEwkWebView(), GetResourceUrl(SAMPLE_ERROR_HTML_FILE).c_str());
  if (!result)
    FAIL();
  
  utc_blink_ewk_base::MainLoopResult main_result = EventLoopStart();
  if (main_result != utc_blink_ewk_base::Success)
    FAIL();

  utc_check_eq(gResult, 2);
}

/**
 * @brief Tests ewk_console_message_level_get incase Ewk_Console_Message-debug message exists
 */
TEST_F(utc_blink_ewk_console_message_level_get, POS_TEST4)
{
  gResult = 0;
  Eina_Bool result = ewk_view_url_set(GetEwkWebView(), GetResourceUrl(SAMPLE_DEBUG_HTML_FILE).c_str());
  if (!result)
    FAIL();

  utc_blink_ewk_base::MainLoopResult main_result = EventLoopStart();
  if (main_result != utc_blink_ewk_base::Success)
    FAIL();

  utc_check_eq(gResult, -1);
}

/**
 * @brief Tests ewk_console_message_level_get incase Ewk_Console_Message-info message exists
 */
TEST_F(utc_blink_ewk_console_message_level_get, POS_TEST5)
{
  gResult = -1;
  Eina_Bool result = ewk_view_url_set(GetEwkWebView(), GetResourceUrl(SAMPLE_INFO_HTML_FILE).c_str());
  if (!result)
    FAIL();

  utc_blink_ewk_base::MainLoopResult main_result = EventLoopStart();
  if (main_result != utc_blink_ewk_base::Success)
    FAIL();

  utc_check_eq(gResult, 0);
}

/**
 * @brief Tests ewk_console_message_level_get incase Ewk_Console_Message does not exist
 */
TEST_F(utc_blink_ewk_console_message_level_get, NEG_TEST)
{
  utc_check_eq(ewk_console_message_level_get(NULL), 0);
}
