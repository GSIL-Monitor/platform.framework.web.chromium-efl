// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

#include <time.h>

class utc_blink_ewk_context_time_offset_set_func : public utc_blink_ewk_base {
 protected:
  utc_blink_ewk_context_time_offset_set_func() : timer_(NULL), webview_(NULL) {}

  Ecore_Timer* timer_;
  Evas_Object* webview_;
  static const std::string injected_bundle_path_;
  static const double time_offset_;

  static void GetPlainText(Evas_Object* webview,
                           const char* plainText,
                           void* data) {
    ASSERT_TRUE(data);
    utc_blink_ewk_context_time_offset_set_func* owner =
        static_cast<utc_blink_ewk_context_time_offset_set_func*>(data);
    owner->EventLoopStop(Success);
    ASSERT_TRUE(plainText);
    long new_time = atol(plainText);
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long ori_time = (tv.tv_sec * 1000000 + tv.tv_usec) / 1000;
    ASSERT_EQ(0, (int)(new_time - ori_time - time_offset_) / 1000);
  }

  static Eina_Bool TimerCallback(void* data) {
    utc_blink_ewk_context_time_offset_set_func* owner =
        static_cast<utc_blink_ewk_context_time_offset_set_func*>(data);
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
};

const std::string
    utc_blink_ewk_context_time_offset_set_func::injected_bundle_path_ =
        "ewk_context/injected_bundle/chromium/libbundle_sample.so";
const double utc_blink_ewk_context_time_offset_set_func::time_offset_ =
    3600000.000;

/**
 * @brief Checking whether time offset is set
 */
TEST_F(utc_blink_ewk_context_time_offset_set_func, POS_TEST) {
  webview = CreateWindow();
  const char simple_html[] =
      "<html><body><script>document.write(Date.now());</script></body></html>";
  ewk_context_tizen_app_id_set(ewk_view_context_get(webview), "hbbtv");
  ewk_context_time_offset_set(ewk_view_context_get(webview), time_offset);
  ASSERT_TRUE(ewk_view_html_string_load(webview, simple_html, NULL, NULL));
  RunWithTimer();
  ASSERT_EQ(Success, EventLoopStart());
}
