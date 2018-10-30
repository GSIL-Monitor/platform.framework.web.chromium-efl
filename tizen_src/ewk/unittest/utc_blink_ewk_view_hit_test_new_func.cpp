// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

class utc_blink_ewk_view_hit_test_new: public utc_blink_ewk_base
{
protected:
 void LoadFinished(Evas_Object* webview) override {
   EventLoopStop(utc_blink_ewk_base::Success);
  }

protected:
  static const char*const url;
};

const char*const utc_blink_ewk_view_hit_test_new::url="http://m.naver.com";

/**
 * @brief Checking whether hit test instance is created properly by hit test mode.
 */
TEST_F(utc_blink_ewk_view_hit_test_new, POS_TEST1)
{
  if (!ewk_view_url_set(GetEwkWebView(), url))
    utc_fail();

  if (utc_blink_ewk_base::Success != EventLoopStart())
    utc_fail();

  Ewk_Hit_Test* hit_test = ewk_view_hit_test_new(GetEwkWebView(), 200, 200, EWK_HIT_TEST_MODE_DEFAULT);
  if (!hit_test)
    utc_fail();
  ewk_hit_test_free(hit_test);

  hit_test = ewk_view_hit_test_new(GetEwkWebView(), 200, 200, EWK_HIT_TEST_MODE_NODE_DATA);
  if (!hit_test)
    utc_fail();
  ewk_hit_test_free(hit_test);

  hit_test = ewk_view_hit_test_new(GetEwkWebView(), 200, 200, EWK_HIT_TEST_MODE_IMAGE_DATA);
  if (!hit_test)
    utc_fail();
  ewk_hit_test_free(hit_test);

  hit_test = ewk_view_hit_test_new(GetEwkWebView(), 200, 200, EWK_HIT_TEST_MODE_ALL);
  if (!hit_test)
    utc_fail();
  ewk_hit_test_free(hit_test);

  utc_pass();
}

/**
 * @brief Checking whether function works properly in case of NULL of a webview.
 */
TEST_F(utc_blink_ewk_view_hit_test_new, NEG_TEST1)
{
  Eina_Bool result = EINA_TRUE;

  Ewk_Hit_Test* hit_test = ewk_view_hit_test_new(NULL, 200, 200, EWK_HIT_TEST_MODE_DEFAULT);
  if (hit_test) {
    ewk_hit_test_free(hit_test);
    utc_fail();
  }

  hit_test = ewk_view_hit_test_new(NULL, 200, 200, EWK_HIT_TEST_MODE_NODE_DATA);
  if (hit_test) {
    ewk_hit_test_free(hit_test);
    utc_fail();
  }

  hit_test = ewk_view_hit_test_new(NULL, 200, 200, EWK_HIT_TEST_MODE_IMAGE_DATA);
  if (hit_test) {
    ewk_hit_test_free(hit_test);
    utc_fail();
  }

  hit_test = ewk_view_hit_test_new(NULL, 200, 200, EWK_HIT_TEST_MODE_ALL);
  if (hit_test) {
    ewk_hit_test_free(hit_test);
    utc_fail();
  }

  utc_check_eq(result, EINA_TRUE);
}
