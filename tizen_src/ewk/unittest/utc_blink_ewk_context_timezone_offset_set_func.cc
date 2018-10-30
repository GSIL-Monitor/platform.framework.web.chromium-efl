// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

#include <time.h>

class utc_blink_ewk_context_timezone_offset_set_func
    : public utc_blink_ewk_base {
 protected:
  utc_blink_ewk_context_timezone_offset_set_func()
      : timer_(NULL), webview_(NULL) {}

  Ecore_Timer* timer_;
  Evas_Object* webview_;
  static const std::string injected_bundle_path_;
  static const double time_zone_offset_;
  static double daylight_saving_time_;

  static void GetPlainText(Evas_Object* webview,
                           const char* plainText,
                           void* data) {
    ASSERT_TRUE(data);
    utc_blink_ewk_context_timezone_offset_set_func* owner =
        static_cast<utc_blink_ewk_context_timezone_offset_set_func*>(data);
    owner->EventLoopStop(Success);
    ASSERT_TRUE(plainText);
    ASSERT_EQ((0 - atol(plainText)),
              (long)(time_zone_offset_ + daylight_saving_time_) / 1000 / 60);
  }

  static Eina_Bool TimerCallback(void* data) {
    utc_blink_ewk_context_timezone_offset_set_func* owner =
        static_cast<utc_blink_ewk_context_timezone_offset_set_func*>(data);
    ewk_view_plain_text_get(owner->webview_, GetPlainText, owner);
    return ECORE_CALLBACK_CANCEL;
  }

  void RunWithTimer() { timer_ = ecore_timer_add(1, TimerCallback, this); }

  Evas_Object* CreateWindow() {
    Evas_Object* window = elm_win_add(NULL, "TC Launcher", ELM_WIN_BASIC);
    return ewk_view_add_with_context(
        evas_object_evas_get(window),
        ewk_context_new_with_injected_bundle_path(
            GetResourcePath(injected_bundle_path_.c_str()).c_str()));
  }

  void DoTest() {
    webview_ = CreateWindow();
    const char simple_html[] =
        "<html><body><script>document.write(new "
        "Date().getTimezoneOffset());</script></body></html>";
    ewk_context_tizen_app_id_set(ewk_view_context_get(webview_), "hbbtv");
    ewk_context_timezone_offset_set(ewk_view_context_get(webview_),
                                    time_zone_offset_, daylight_saving_time_);
    ASSERT_TRUE(ewk_view_html_string_load(webview_, simple_html, NULL, NULL));
    RunWithTimer();
    ASSERT_EQ(Success, EventLoopStart());
  }
};

const std::string
    utc_blink_ewk_context_timezone_offset_set_func::injected_bundle_path_ =
        "ewk_context/injected_bundle/chromium/libbundle_sample.so";
const double utc_blink_ewk_context_timezone_offset_set_func::time_zone_offset_ =
    7200000.000;
double utc_blink_ewk_context_timezone_offset_set_func::daylight_saving_time_ =
    0.0;

/**
 * @brief Checking whether time zone offset is set, no DST
 */
TEST_F(utc_blink_ewk_context_timezone_offset_set_func, POS_TEST_NO_DST) {
  DoTest();
}

/**
 * @brief Checking whether time zone offset is set, with DST 1 hour
 */
TEST_F(utc_blink_ewk_context_timezone_offset_set_func, POS_TEST_DST_1H) {
  daylight_saving_time_ = 3600000.000;
  DoTest();
}
