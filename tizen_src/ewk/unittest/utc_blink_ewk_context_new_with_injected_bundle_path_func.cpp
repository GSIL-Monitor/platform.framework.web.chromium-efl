// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// See README.md how to run this test in DESKTOP mode.

#include "utc_blink_ewk_base.h"

class utc_blink_ewk_context_new_with_injected_bundle_path_func
    : public utc_blink_ewk_base {
 protected:
  utc_blink_ewk_context_new_with_injected_bundle_path_func()
      : timer(NULL), webview(NULL) {}

  Ecore_Timer* timer;
  Evas_Object* webview;
  static const std::string injectedBundlePath;

  static void getPlainText(Evas_Object* webview,
                           const char* plainText,
                           void* data) {
    ASSERT_TRUE(data);
    utc_blink_ewk_context_new_with_injected_bundle_path_func* owner =
        static_cast<utc_blink_ewk_context_new_with_injected_bundle_path_func*>(
            data);
    owner->EventLoopStop(Success);
    ASSERT_TRUE(plainText);

    ASSERT_STREQ("DynamicPluginStartSession", plainText);
  }
  static Eina_Bool timerCallback(void* data) {
    utc_blink_ewk_context_new_with_injected_bundle_path_func* owner =
        static_cast<utc_blink_ewk_context_new_with_injected_bundle_path_func*>(
            data);
    ewk_view_plain_text_get(owner->webview, getPlainText, owner);
    return ECORE_CALLBACK_CANCEL;
  }
  void runWithTimer() { timer = ecore_timer_add(1, timerCallback, this); }

  Evas_Object* CreateWindow() {
    Evas_Object* window = elm_win_add(NULL, "TC Launcher", ELM_WIN_BASIC);
    return ewk_view_add_with_context(
        evas_object_evas_get(window),
        ewk_context_new_with_injected_bundle_path(
            GetResourcePath(injectedBundlePath.c_str()).c_str()));
  }
};

const std::string utc_blink_ewk_context_new_with_injected_bundle_path_func::
    injectedBundlePath =
        "ewk_context/injected_bundle/chromium/libbundle_sample.so";

/**
 * @brief Checking whether context with injected bundle path is returned
 */
TEST_F(utc_blink_ewk_context_new_with_injected_bundle_path_func, POS_TEST) {
  ASSERT_TRUE(ewk_context_new_with_injected_bundle_path(
      GetResourcePath(injectedBundlePath.c_str()).c_str()));
}

TEST_F(utc_blink_ewk_context_new_with_injected_bundle_path_func, POS_TEST1) {
  webview = CreateWindow();
  const char simpleHTML[] =
      "<html><body><script>document.write(\"\");</script></body></html>";
  ewk_context_tizen_app_id_set(ewk_view_context_get(webview), "1");
  ASSERT_TRUE(ewk_view_html_string_load(webview, simpleHTML, NULL, NULL));
  runWithTimer();
  ASSERT_EQ(Success, EventLoopStart());
}

TEST_F(utc_blink_ewk_context_new_with_injected_bundle_path_func, NEG_TEST) {
  Ewk_Context* context = ewk_context_new_with_injected_bundle_path(0);
  ASSERT_TRUE(!context);
}
