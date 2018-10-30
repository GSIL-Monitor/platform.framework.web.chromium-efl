// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

class utc_blink_ewk_view_history_get : public utc_blink_ewk_base
{
protected:
 void LoadFinished(Evas_Object*) override { EventLoopStop(Success); }

protected:
  static const char* const TEST_URL1;
  static const char* const TEST_URL2;
  static const char* const TEST_URL3;
};

const char* const utc_blink_ewk_view_history_get::TEST_URL1 = "ewk_history/page1.html";
const char* const utc_blink_ewk_view_history_get::TEST_URL2 = "ewk_history/page2.html";
const char* const utc_blink_ewk_view_history_get::TEST_URL3 = "ewk_history/page3.html";

TEST_F(utc_blink_ewk_view_history_get, POS_TEST)
{
  // load 3 pages to get some interesting history
  ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(), GetResourceUrl(TEST_URL1).c_str()));
  ASSERT_EQ(Success, EventLoopStart());
  std::string expectedTitle1(ewk_view_title_get(GetEwkWebView()));
  ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(), GetResourceUrl(TEST_URL2).c_str()));
  ASSERT_EQ(Success, EventLoopStart());
  std::string expectedTitle2(ewk_view_title_get(GetEwkWebView()));
  ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(), GetResourceUrl(TEST_URL3).c_str()));
  ASSERT_EQ(Success, EventLoopStart());
  std::string expectedTitle3(ewk_view_title_get(GetEwkWebView()));

  // get history
  Ewk_History *history = ewk_view_history_get(GetEwkWebView());
  ASSERT_TRUE(history);

  // check back & forward count
  ASSERT_EQ(2, ewk_history_back_list_length_get(history));
  ASSERT_EQ(0, ewk_history_forward_list_length_get(history));

  // get current item and chack URL and title
  Ewk_History_Item *item = ewk_history_nth_item_get(history, 0);

  ASSERT_STREQ(GetResourceUrl(TEST_URL3).c_str(), ewk_history_item_uri_get(item));
  ASSERT_STREQ(expectedTitle3.c_str(), ewk_history_item_title_get(item));

  // free history
  ewk_history_free(history);

  // go back
  ASSERT_EQ(EINA_TRUE, ewk_view_back(GetEwkWebView()));
  ASSERT_EQ(Success, EventLoopStart());

  // get history
  history = ewk_view_history_get(GetEwkWebView());
  ASSERT_TRUE(history);

  // check back & forward count
  ASSERT_EQ(1, ewk_history_back_list_length_get(history));
  ASSERT_EQ(1, ewk_history_forward_list_length_get(history));

  // get current item and chack URL and title
  item = ewk_history_nth_item_get(history, 0);

  ASSERT_STREQ(GetResourceUrl(TEST_URL2).c_str(), ewk_history_item_uri_get(item));
  ASSERT_STREQ(expectedTitle2.c_str(), ewk_history_item_title_get(item));

  // get forward item and check URL and title
  item = ewk_history_nth_item_get(history, 1);

  ASSERT_STREQ(GetResourceUrl(TEST_URL3).c_str(), ewk_history_item_uri_get(item));
  ASSERT_STREQ(expectedTitle3.c_str(), ewk_history_item_title_get(item));

  // get back item and check URL and title
  item = ewk_history_nth_item_get(history, -1);

  ASSERT_STREQ(GetResourceUrl(TEST_URL1).c_str(), ewk_history_item_uri_get(item));
  ASSERT_STREQ(expectedTitle1.c_str(), ewk_history_item_title_get(item));

  // free history
  ewk_history_free(history);

  // go back
  ewk_view_back(GetEwkWebView());
  ASSERT_EQ(Success, EventLoopStart());

  // get history
  history = ewk_view_history_get(GetEwkWebView());
  ASSERT_TRUE(history);

  // check back & forward count
  ASSERT_EQ(0, ewk_history_back_list_length_get(history));
  ASSERT_EQ(2, ewk_history_forward_list_length_get(history));

  // get current item and chack URL and title
  item = ewk_history_nth_item_get(history, 0);

  ASSERT_STREQ(GetResourceUrl(TEST_URL1).c_str(), ewk_history_item_uri_get(item));
  ASSERT_STREQ(expectedTitle1.c_str(), ewk_history_item_title_get(item));

  // free history
  ewk_history_free(history);
}

TEST_F(utc_blink_ewk_view_history_get, EMPTY_TEST)
{
  Ewk_History *history = ewk_view_history_get(GetEwkWebView());
  ASSERT_TRUE(history);
  ASSERT_EQ(0, ewk_history_back_list_length_get(history));
  ASSERT_EQ(0, ewk_history_forward_list_length_get(history));

  // free history
  ewk_history_free(history);
}

TEST_F(utc_blink_ewk_view_history_get, NULL_TEST)
{
  Ewk_History *history = ewk_view_history_get(NULL);
  ASSERT_FALSE(history);
}
