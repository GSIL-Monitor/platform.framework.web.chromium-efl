// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

static const char* const kUrl = "common/sample.html";

class utc_blink_ewk_view_page_visibility_state_set : public utc_blink_ewk_base {
 private:
  void LoadFinished(Evas_Object* webview) override { EventLoopStop(Success); }
};

TEST_F(utc_blink_ewk_view_page_visibility_state_set, POS_STATE_HIDDEN) {
  Eina_Bool result =
      ewk_view_url_set(GetEwkWebView(), GetResourceUrl(kUrl).c_str());
  ASSERT_EQ(EINA_TRUE, result);
  ASSERT_EQ(Success, EventLoopStart());
  result = ewk_view_page_visibility_state_set(
      GetEwkWebView(), EWK_PAGE_VISIBILITY_STATE_HIDDEN, EINA_TRUE);
  ASSERT_EQ(EINA_TRUE, result);
}

TEST_F(utc_blink_ewk_view_page_visibility_state_set, POS_STATE_VISIBLE) {
  Eina_Bool result =
      ewk_view_url_set(GetEwkWebView(), GetResourceUrl(kUrl).c_str());
  ASSERT_EQ(EINA_TRUE, result);
  ASSERT_EQ(Success, EventLoopStart());
  result = ewk_view_page_visibility_state_set(
      GetEwkWebView(), EWK_PAGE_VISIBILITY_STATE_VISIBLE, EINA_FALSE);
  ASSERT_EQ(EINA_TRUE, result);
}

TEST_F(utc_blink_ewk_view_page_visibility_state_set,
       NEG_NO_CONTEXT_STATE_HIDDEN) {
  Eina_Bool result = ewk_view_page_visibility_state_set(
      GetEwkWebView(), EWK_PAGE_VISIBILITY_STATE_HIDDEN, EINA_TRUE);
  ASSERT_EQ(EINA_FALSE, result);
}

TEST_F(utc_blink_ewk_view_page_visibility_state_set,
       NEG_NO_CONTEXT_STATE_VISIBLE) {
  Eina_Bool result = ewk_view_page_visibility_state_set(
      GetEwkWebView(), EWK_PAGE_VISIBILITY_STATE_VISIBLE, EINA_TRUE);
  ASSERT_EQ(EINA_FALSE, result);
}

TEST_F(utc_blink_ewk_view_page_visibility_state_set, NEG_INVALID_WEBVIEW) {
  Eina_Bool result =
      ewk_view_url_set(GetEwkWebView(), GetResourceUrl(kUrl).c_str());
  ASSERT_EQ(EINA_TRUE, result);
  ASSERT_EQ(Success, EventLoopStart());
  result = ewk_view_page_visibility_state_set(
      NULL, EWK_PAGE_VISIBILITY_STATE_HIDDEN, EINA_TRUE);
  ASSERT_EQ(EINA_FALSE, result);
}

TEST_F(utc_blink_ewk_view_page_visibility_state_set, NEG_PRERENDER_STATE) {
  Eina_Bool result =
      ewk_view_url_set(GetEwkWebView(), GetResourceUrl(kUrl).c_str());
  ASSERT_EQ(EINA_TRUE, result);
  ASSERT_EQ(Success, EventLoopStart());
  result = ewk_view_page_visibility_state_set(
      GetEwkWebView(), EWK_PAGE_VISIBILITY_STATE_PRERENDER, EINA_TRUE);
  ASSERT_EQ(EINA_FALSE, result);
}
