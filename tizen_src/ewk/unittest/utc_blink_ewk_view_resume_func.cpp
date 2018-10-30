// Copyright 2014-2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

class utc_blink_ewk_view_resume : public utc_blink_ewk_base
{
protected:
  enum RunPhase {
    PageLoadStart,
    PageSuspended,
    PageResumed
  };

protected:
 utc_blink_ewk_view_resume()
     : phase(PageLoadStart), result_url_set(EINA_FALSE) {}

 /* Callback for quitting main loop */
 void LoadProgress(Evas_Object* webview, double pr) override {
   // Page started to load, after that we want to suspend right after
   if (phase == PageLoadStart) {
     phase = PageSuspended;
     EventLoopStop(Success);
   }
  }

  bool TimeOut() override {
    if (phase == PageSuspended) {
      phase = PageResumed;
      EventLoopStop(Success);
      return true;
    }

    return false;
  }

  void LoadFinished(Evas_Object* webview) override {
    if (phase == PageResumed) {
      EventLoopStop(Success);
    } else {
      EventLoopStop(Failure);
    }
  }

  static void job_url_set(utc_blink_ewk_base* data) {
    utc_blink_ewk_view_resume* owner =
        static_cast<utc_blink_ewk_view_resume*>(data);
    owner->result_url_set =
        ewk_view_url_set(owner->GetEwkWebView(), "http://www.google.pl");
  }

  RunPhase phase;
  Eina_Bool result_url_set;
};

/**
 * @brief Positive test case of ewk_view_resume()
 */
TEST_F(utc_blink_ewk_view_resume, POS_TEST)
{
  SetTestJob(utc_blink_ewk_view_resume::job_url_set);

  if (Success != EventLoopStart())
    FAIL();

  ASSERT_EQ(EINA_TRUE, result_url_set);

  utc_message("Suspend");
  ewk_view_suspend(GetEwkWebView());

  if (Success != EventLoopStart(5.0)) // Wait few seconds to see if it realy was suspended
    FAIL();

  utc_message("Resume");
  ewk_view_resume(GetEwkWebView());

  if (Success != EventLoopStart())
    FAIL();

  evas_object_show(GetEwkWebView());
  evas_object_show(GetEwkWindow());
}

/**
 * @brief Negative test case of ewk_view_resume()
 */
TEST_F(utc_blink_ewk_view_resume, NEG_TEST)
{
  // This test is pointless. ewk_view_resume has no return value so we have nothing to test here.
  ewk_view_resume(NULL);
  evas_object_show(GetEwkWebView());
  evas_object_show(GetEwkWindow());
}
