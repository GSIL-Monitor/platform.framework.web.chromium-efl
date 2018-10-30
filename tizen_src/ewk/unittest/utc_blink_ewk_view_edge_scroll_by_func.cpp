// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

class utc_blink_ewk_view_edge_scroll_by : public utc_blink_ewk_base {
 protected:
  enum OrientaionType { Left, Right, Top, Bottom };

  static void EdgeLeft(void* data, Evas_Object* o, void* data_finished) {
    utc_message("[EdgeLeft] :: \n");
    if (data) {
      auto scroll = static_cast<utc_blink_ewk_view_edge_scroll_by*>(data);
      if (scroll->orientation_ == Left)
        scroll->EventLoopStop(Success);
    }
  }

  static void EdgeRight(void* data, Evas_Object* o, void* data_finished) {
    utc_message("[EdgeRight] :: \n");
    if (data) {
      auto scroll = static_cast<utc_blink_ewk_view_edge_scroll_by*>(data);
      if (scroll->orientation_ == Right)
        scroll->EventLoopStop(Success);
    }
  }

  static void EdgeTop(void* data, Evas_Object* o, void* data_finished) {
    utc_message("[EdgeTop] :: \n");
    if (data) {
      auto scroll = static_cast<utc_blink_ewk_view_edge_scroll_by*>(data);
      if (scroll->orientation_ == Top)
        scroll->EventLoopStop(Success);
    }
  }

  static void EdgeBottom(void* data, Evas_Object* o, void* data_finished) {
    utc_message("[EdgeBottom] :: \n");
    if (data) {
      auto scroll = static_cast<utc_blink_ewk_view_edge_scroll_by*>(data);
      if (scroll->orientation_ == Bottom)
        scroll->EventLoopStop(Success);
    }
  }

  /* Startup function */
  void PostSetUp() override {
    evas_object_smart_callback_add(GetEwkWebView(), "edge,scroll,left",
                                   EdgeLeft, this);
    evas_object_smart_callback_add(GetEwkWebView(), "edge,scroll,right",
                                   EdgeRight, this);
    evas_object_smart_callback_add(GetEwkWebView(), "edge,scroll,top", EdgeTop,
                                   this);
    evas_object_smart_callback_add(GetEwkWebView(), "edge,scroll,bottom",
                                   EdgeBottom, this);
  }

  /* Cleanup function */
  void PreTearDown() override {
    evas_object_smart_callback_del(GetEwkWebView(), "edge,scroll,left",
                                   EdgeLeft);
    evas_object_smart_callback_del(GetEwkWebView(), "edge,scroll,right",
                                   EdgeRight);
    evas_object_smart_callback_del(GetEwkWebView(), "edge,scroll,top", EdgeTop);
    evas_object_smart_callback_del(GetEwkWebView(), "edge,scroll,bottom",
                                   EdgeBottom);
  }

 protected:
  OrientaionType orientation_;
  static const char* const kResource;
};

const char* const utc_blink_ewk_view_edge_scroll_by::kResource =
    "/ewk_view/index_big_red_square.html";

/**
 * @brief Positive test case of ewk_view_edge_scroll_by(),testing scroll left
 */

TEST_F(utc_blink_ewk_view_edge_scroll_by, POS_TEST_LEFT) {
  ASSERT_TRUE(
      ewk_view_url_set(GetEwkWebView(), GetResourceUrl(kResource).c_str()));

  const int kDeltaX = -10;
  const int kDeltaY = 0;
  orientation_ = Left;

  ASSERT_TRUE(ewk_view_edge_scroll_by(GetEwkWebView(), kDeltaX, kDeltaY));
  ASSERT_EQ(Success, EventLoopStart());
}

/**
 * @brief Positive test case of ewk_view_edge_scroll_by(),testing scroll right
 */

TEST_F(utc_blink_ewk_view_edge_scroll_by, POS_TEST_RIGHT) {
  ASSERT_TRUE(
      ewk_view_url_set(GetEwkWebView(), GetResourceUrl(kResource).c_str()));

  const int kDeltaX = 10;
  const int kDeltaY = 0;
  orientation_ = Right;

  ASSERT_TRUE(ewk_view_edge_scroll_by(GetEwkWebView(), kDeltaX, kDeltaY));
  ASSERT_EQ(Success, EventLoopStart());
}

/**
 * @brief Positive test case of ewk_view_edge_scroll_by(),testing scroll top
 */

TEST_F(utc_blink_ewk_view_edge_scroll_by, POS_TEST_TOP) {
  ASSERT_TRUE(
      ewk_view_url_set(GetEwkWebView(), GetResourceUrl(kResource).c_str()));

  const int kDeltaX = 0;
  const int kDeltaY = -10;
  orientation_ = Top;

  ASSERT_TRUE(ewk_view_edge_scroll_by(GetEwkWebView(), kDeltaX, kDeltaY));
  ASSERT_EQ(Success, EventLoopStart());
}

/**
 * @brief Positive test case of ewk_view_edge_scroll_by(),testing scroll bottom
 */

TEST_F(utc_blink_ewk_view_edge_scroll_by, POS_TEST_BOTTOM) {
  ASSERT_TRUE(
      ewk_view_url_set(GetEwkWebView(), GetResourceUrl(kResource).c_str()));

  const int kDeltaX = 0;
  const int kDeltaY = 10;
  orientation_ = Bottom;

  ASSERT_TRUE(ewk_view_edge_scroll_by(GetEwkWebView(), kDeltaX, kDeltaY));
  ASSERT_EQ(Success, EventLoopStart());
}

/**
 * @brief Negative test case of ewk_view_edge_scroll_by(), testing for null
 */
TEST_F(utc_blink_ewk_view_edge_scroll_by, NEG_TEST) {
  EXPECT_EQ(EINA_FALSE, ewk_view_edge_scroll_by(NULL, 1916, 1993));
}
