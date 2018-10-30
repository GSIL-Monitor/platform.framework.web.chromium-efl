// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

#include <cmath>

class utc_blink_ewk_view_scale_changed_callback_set : public utc_blink_ewk_base
{
 protected:

  void LoadFinished(Evas_Object* webview) override
  {
    EventLoopStop(Success);
  }

  static Eina_Bool mainLoopQuit(void* data)
  {
    if (data)
      static_cast<utc_blink_ewk_view_scale_changed_callback_set*>(data)->EventLoopStop(Success);
    return ECORE_CALLBACK_DONE;
  }

  static void scaleChanged(Evas_Object* webview, double scale, void* user_data)
  {
    ASSERT_LT(std::abs(expected_scale - scale), precision);
    wasCallbackCalled = true;
  }

  static const char*const sample;
  static double precision;
  static bool wasCallbackCalled;
  static double expected_scale;
};

const char*const utc_blink_ewk_view_scale_changed_callback_set::sample =
    "/common/sample.html";
bool utc_blink_ewk_view_scale_changed_callback_set::wasCallbackCalled =
    1.0;
double utc_blink_ewk_view_scale_changed_callback_set::expected_scale =
    false;
double utc_blink_ewk_view_scale_changed_callback_set::precision =
    std::numeric_limits<double>::epsilon();

TEST_F(utc_blink_ewk_view_scale_changed_callback_set, POS_TEST)
{
  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(),
              GetResourceUrl(sample).c_str()));
  ASSERT_EQ(Success, EventLoopStart());

  double min_scale = 0, max_scale = 0;
  ewk_view_scale_range_get(GetEwkWebView(), &min_scale, &max_scale);
  double scale_factor = (max_scale + min_scale) / 2;

  // Set the scale within the allowed range and expect this value while
  // calling the callback.
  expected_scale = scale_factor;
  ewk_view_scale_changed_callback_set(GetEwkWebView(),
      utc_blink_ewk_view_scale_changed_callback_set::scaleChanged,
      nullptr /* user_data */);

  ASSERT_TRUE(ewk_view_scale_set(GetEwkWebView(), scale_factor, 0, 0));
  ecore_timer_add(0.5f, mainLoopQuit, this);
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_TRUE(wasCallbackCalled);

  // Set the scale grather than the maximum allowed value and expect the
  // scale change callback with the value clamped to maxium allowed scale.
  wasCallbackCalled = false;
  scale_factor = max_scale * 2;
  expected_scale = max_scale;

  ASSERT_TRUE(ewk_view_scale_set(GetEwkWebView(), scale_factor, 0, 0));
  ecore_timer_add(0.5f, mainLoopQuit, this);
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_TRUE(wasCallbackCalled);

  // Set the scale lower than the minimum allowed value and expect the
  // scale change callback with the value clamped to minimum allowed scale.
  wasCallbackCalled = false;
  scale_factor = min_scale / 2;
  expected_scale = min_scale;

  ASSERT_TRUE(ewk_view_scale_set(GetEwkWebView(), scale_factor, 0, 0));
  ecore_timer_add(0.5f, mainLoopQuit, this);
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_TRUE(wasCallbackCalled);
}

TEST_F(utc_blink_ewk_view_scale_changed_callback_set, NEG_TEST)
{
  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(),
              GetResourceUrl(sample).c_str()));
  ASSERT_EQ(Success, EventLoopStart());

  double min_scale = 0, max_scale = 0;
  ewk_view_scale_range_get(GetEwkWebView(), &min_scale, &max_scale);

  // Set the scale within the allowed range and expect this value to be set
  // but does not specify the callback for scale changes. The code must not crash.
  double scale_factor = (max_scale + min_scale) / 2;
  wasCallbackCalled = false;
  ewk_view_scale_changed_callback_set(GetEwkWebView(),
      nullptr /* scaleChanged callback */,
      nullptr /* user_data */);

  ASSERT_TRUE(ewk_view_scale_set(GetEwkWebView(), scale_factor, 0, 0));
  ecore_timer_add(0.5f, mainLoopQuit, this);
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_FALSE(wasCallbackCalled);
  ASSERT_LT(std::abs(scale_factor - ewk_view_scale_get(GetEwkWebView())),
            precision);

  // Set the current scale again. The callback responsible for scale change
  // should not be called.
  wasCallbackCalled = false;
  ewk_view_scale_changed_callback_set(GetEwkWebView(),
      utc_blink_ewk_view_scale_changed_callback_set::scaleChanged,
      nullptr /* user_data */);

  ASSERT_TRUE(ewk_view_scale_set(GetEwkWebView(),
      ewk_view_scale_get(GetEwkWebView()), 0, 0));
  ecore_timer_add(0.5f, mainLoopQuit, this);
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_FALSE(wasCallbackCalled);
}
