// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

class utc_blink_ewk_view_back_forward_list_clear : public utc_blink_ewk_base
{
protected:
 void LoadFinished(Evas_Object*) override { EventLoopStop(Success); }

protected:
  static const char* const TEST_URL1;
  static const char* const TEST_URL2;
  static const char* const TEST_URL3;
};

const char* const utc_blink_ewk_view_back_forward_list_clear::TEST_URL1 = "ewk_history/page1.html";
const char* const utc_blink_ewk_view_back_forward_list_clear::TEST_URL2 = "ewk_history/page2.html";
const char* const utc_blink_ewk_view_back_forward_list_clear::TEST_URL3 = "ewk_history/page3.html";

TEST_F(utc_blink_ewk_view_back_forward_list_clear, POS_TES)
{
  // load 3 pages to get some interesting history
  ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(), GetResourceUrl(TEST_URL1).c_str()));
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(), GetResourceUrl(TEST_URL2).c_str()));
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(), GetResourceUrl(TEST_URL3).c_str()));
  ASSERT_EQ(Success, EventLoopStart());

  // go back
  ASSERT_EQ(EINA_TRUE, ewk_view_back(GetEwkWebView()));
  ASSERT_EQ(Success, EventLoopStart());

  // check if back forward possible
  ASSERT_EQ(EINA_TRUE, ewk_view_back_possible(GetEwkWebView()));
  ASSERT_EQ(EINA_TRUE, ewk_view_forward_possible(GetEwkWebView()));

  // clear back-forward list
  ewk_view_back_forward_list_clear(GetEwkWebView());

  // check if back forward possible
  ASSERT_EQ(EINA_FALSE, ewk_view_back_possible(GetEwkWebView()));
  ASSERT_EQ(EINA_FALSE, ewk_view_forward_possible(GetEwkWebView()));
}

TEST_F(utc_blink_ewk_view_back_forward_list_clear, NEG_TEST)
{
  ewk_view_back_forward_list_clear(NULL);
}
