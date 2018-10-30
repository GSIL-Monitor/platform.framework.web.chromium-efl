// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

static const char* kURL = "/ewk_view/index_big_red_square.html";

class utc_blink_ewk_view_horizontal_panning_hold_get
    : public utc_blink_ewk_base {
 protected:
  void LoadFinished(Evas_Object*) override { EventLoopStop(Success); }
};

TEST_F(utc_blink_ewk_view_horizontal_panning_hold_get, POS_HOLD_DEFAULT) {
  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), GetResourceUrl(kURL).c_str()));
  ASSERT_EQ(Success, EventLoopStart());

  ASSERT_FALSE(ewk_view_horizontal_panning_hold_get(GetEwkWebView()));
}

TEST_F(utc_blink_ewk_view_horizontal_panning_hold_get, POS_HOLD_TRUE) {
  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), GetResourceUrl(kURL).c_str()));
  ASSERT_EQ(Success, EventLoopStart());

  ewk_view_horizontal_panning_hold_set(GetEwkWebView(), EINA_TRUE);
  ASSERT_TRUE(ewk_view_horizontal_panning_hold_get(GetEwkWebView()));
}

TEST_F(utc_blink_ewk_view_horizontal_panning_hold_get, POS_HOLD_FALSE) {
  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), GetResourceUrl(kURL).c_str()));
  ASSERT_EQ(Success, EventLoopStart());

  ewk_view_horizontal_panning_hold_set(GetEwkWebView(), EINA_FALSE);
  ASSERT_FALSE(ewk_view_horizontal_panning_hold_get(GetEwkWebView()));
}

TEST_F(utc_blink_ewk_view_horizontal_panning_hold_get, NEG_INVALID_VIEW) {
  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), GetResourceUrl(kURL).c_str()));
  ASSERT_EQ(Success, EventLoopStart());

  ewk_view_horizontal_panning_hold_set(GetEwkWebView(), EINA_TRUE);
  ASSERT_FALSE(ewk_view_horizontal_panning_hold_get(nullptr));
}
