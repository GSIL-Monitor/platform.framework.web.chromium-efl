// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

class utc_blink_ewk_view_add_item_to_back_forward_list
    : public utc_blink_ewk_base {
 protected:
  void LoadFinished(Evas_Object*) override { EventLoopStop(Success); }

 protected:
  static const char* const TEST_URL1;
  static const char* const TEST_URL2;
  static const char* const TEST_URL3;
};

const char* const utc_blink_ewk_view_add_item_to_back_forward_list::TEST_URL1 =
    "ewk_history/page1.html";
const char* const utc_blink_ewk_view_add_item_to_back_forward_list::TEST_URL2 =
    "ewk_history/page2.html";
const char* const utc_blink_ewk_view_add_item_to_back_forward_list::TEST_URL3 =
    "ewk_history/page3.html";

/**
 * @brief Check if add item is succeeds with
 * correct webview
 */
TEST_F(utc_blink_ewk_view_add_item_to_back_forward_list, POS_ITEM_TEST) {
  // load 3 pages to get some interesting history
  ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(),
                                        GetResourceUrl(TEST_URL1).c_str()));
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(),
                                        GetResourceUrl(TEST_URL2).c_str()));
  ASSERT_EQ(Success, EventLoopStart());

  // get back-forward list
  Ewk_Back_Forward_List* list = ewk_view_back_forward_list_get(GetEwkWebView());
  ASSERT_TRUE(list);

  // get current item and check URL, original URL and title
  Ewk_Back_Forward_List_Item* item =
      ewk_back_forward_list_current_item_get(list);

  item = ewk_back_forward_list_item_ref(item);

  // clear back-forward list
  ewk_view_back_forward_list_clear(GetEwkWebView());

  // load a new page to get a new back-forward list
  ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(),
                                        GetResourceUrl(TEST_URL3).c_str()));
  ASSERT_EQ(Success, EventLoopStart());

  // add item in to back-forward list
  Eina_Bool result =
      ewk_view_add_item_to_back_forward_list(GetEwkWebView(), item);
  EXPECT_EQ(EINA_TRUE, result);

  // and unref so it can be deleted
  ewk_back_forward_list_item_unref(item);

  // get current item and check it url
  list = ewk_view_back_forward_list_get(GetEwkWebView());
  item = ewk_back_forward_list_current_item_get(list);
  ASSERT_STREQ(GetResourceUrl(TEST_URL2).c_str(),
               ewk_back_forward_list_item_url_get(item));
}

/**
 * @brief Check if null item is fails with
 * correct webview
 */
TEST_F(utc_blink_ewk_view_add_item_to_back_forward_list, POS_ITEM_NULL) {
  Eina_Bool result =
      ewk_view_add_item_to_back_forward_list(GetEwkWebView(), NULL);
  EXPECT_EQ(EINA_FALSE, result);
}

/**
 * @brief Check if null item is fails with
 * null webview
 */
TEST_F(utc_blink_ewk_view_add_item_to_back_forward_list, NEG_ITEM_NULL) {
  Eina_Bool result = ewk_view_add_item_to_back_forward_list(NULL, NULL);
  EXPECT_EQ(EINA_FALSE, result);
}
