// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

static const char* kURL = "/ewk_view/index_big_red_square.html";
static const int kFirstTouchPosition = 500;
static const double kTouchEventInterval = 0.012;

class utc_blink_ewk_view_vertical_panning_hold_set : public utc_blink_ewk_base {
 protected:
  void LoadFinished(Evas_Object*) override { EventLoopStop(Success); }

  // Move touch point from (500, 500) to (400, 400).
  void Panning() {
    Ewk_Touch_Point* touch_point = new Ewk_Touch_Point;
    touch_point->id = 0;
    touch_point->x = kFirstTouchPosition;
    touch_point->y = kFirstTouchPosition;
    touch_point->state = EVAS_TOUCH_POINT_DOWN;

    Eina_List* list = nullptr;
    list = eina_list_append(list, touch_point);

    ewk_view_feed_touch_event(GetEwkWebView(), EWK_TOUCH_START, list, nullptr);

    for (int i = 1; i <= 10; ++i) {
      touch_point->x = kFirstTouchPosition - i * 10;
      touch_point->y = kFirstTouchPosition - i * 10;
      touch_point->state = EVAS_TOUCH_POINT_MOVE;

      ewk_view_feed_touch_event(GetEwkWebView(), EWK_TOUCH_MOVE, list, nullptr);
      EventLoopWait(kTouchEventInterval);
    }

    eina_list_free(list);
    delete touch_point;
  }
};

TEST_F(utc_blink_ewk_view_vertical_panning_hold_set, POS_HOLD_TRUE) {
  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), GetResourceUrl(kURL).c_str()));
  ASSERT_EQ(Success, EventLoopStart());

  ewk_view_touch_events_enabled_set(GetEwkWebView(), EINA_TRUE);
  ewk_view_vertical_panning_hold_set(GetEwkWebView(), EINA_TRUE);
  Panning();
  EventLoopWait(1.0);

  int x, y;
  ASSERT_TRUE(ewk_view_scroll_pos_get(GetEwkWebView(), &x, &y));
  EXPECT_NE(0, x);
  EXPECT_EQ(0, y);
}

TEST_F(utc_blink_ewk_view_vertical_panning_hold_set, POS_HOLD_FALSE) {
  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), GetResourceUrl(kURL).c_str()));
  ASSERT_EQ(Success, EventLoopStart());

  ewk_view_touch_events_enabled_set(GetEwkWebView(), EINA_TRUE);
  ewk_view_vertical_panning_hold_set(GetEwkWebView(), EINA_FALSE);
  Panning();
  EventLoopWait(1.0);

  int x, y;
  ASSERT_TRUE(ewk_view_scroll_pos_get(GetEwkWebView(), &x, &y));
  EXPECT_NE(0, x);
  EXPECT_NE(0, y);
}

TEST_F(utc_blink_ewk_view_vertical_panning_hold_set, NEG_INVALID_VIEW) {
  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), GetResourceUrl(kURL).c_str()));
  ASSERT_EQ(Success, EventLoopStart());

  ewk_view_touch_events_enabled_set(GetEwkWebView(), EINA_TRUE);
  ewk_view_vertical_panning_hold_set(nullptr, EINA_TRUE);
  Panning();
  EventLoopWait(1.0);

  int x, y;
  ASSERT_TRUE(ewk_view_scroll_pos_get(GetEwkWebView(), &x, &y));
  EXPECT_NE(0, x);
  EXPECT_NE(0, y);
}
