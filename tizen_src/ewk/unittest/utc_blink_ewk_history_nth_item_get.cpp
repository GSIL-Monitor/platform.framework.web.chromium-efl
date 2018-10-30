// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

class utc_blink_ewk_history_nth_item_get : public utc_blink_ewk_base
{
protected:
 void LoadFinished(Evas_Object*) override { EventLoopStop(Success); }

protected:
  static const char* const TEST_URL1;
  static const char* const TEST_URL2;
  static const char* const TEST_URL3;
};

const char* const utc_blink_ewk_history_nth_item_get::TEST_URL1 = "ewk_history/page1.html";
const char* const utc_blink_ewk_history_nth_item_get::TEST_URL2 = "ewk_history/page2.html";
const char* const utc_blink_ewk_history_nth_item_get::TEST_URL3 = "ewk_history/page3.html";

TEST_F(utc_blink_ewk_history_nth_item_get, EXISTING_HISTORY)
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

  // go back
  ASSERT_EQ(EINA_TRUE, ewk_view_back(GetEwkWebView()));

  Ewk_History *history = ewk_view_history_get(GetEwkWebView());
  ASSERT_TRUE(history);

  // get current item
  Ewk_History_Item *item = ewk_history_nth_item_get(history, 0);

  ASSERT_TRUE(item);

  ASSERT_STREQ(expectedTitle2.c_str(), ewk_history_item_title_get(item));
  ASSERT_STREQ(GetResourceUrl(TEST_URL2).c_str(), ewk_history_item_uri_get(item));

  // get back item
  item = ewk_history_nth_item_get(history, -1);

  ASSERT_TRUE(item);

  ASSERT_STREQ(expectedTitle1.c_str(), ewk_history_item_title_get(item));
  ASSERT_STREQ(GetResourceUrl(TEST_URL1).c_str(), ewk_history_item_uri_get(item));

  // get forward item
  item = ewk_history_nth_item_get(history, 1);

  ASSERT_TRUE(item);

  ASSERT_STREQ(expectedTitle3.c_str(), ewk_history_item_title_get(item));
  ASSERT_STREQ(GetResourceUrl(TEST_URL3).c_str(), ewk_history_item_uri_get(item));

  // get out of bounds item (positive index)
  item = ewk_history_nth_item_get(history, 10);
  ASSERT_FALSE(item);

  // get out of bounds item (negative index)
  item = ewk_history_nth_item_get(history, -10);
  ASSERT_FALSE(item);

  ewk_history_free(history);
}

TEST_F(utc_blink_ewk_history_nth_item_get, EMPTY_HISTORY)
{
  Ewk_History *history = ewk_view_history_get(GetEwkWebView());
  ASSERT_TRUE(history);

  ASSERT_FALSE(ewk_history_nth_item_get(history, 0));

  ewk_history_free(history);
}

TEST_F(utc_blink_ewk_history_nth_item_get, NULL_HISTORY)
{
  ASSERT_FALSE(ewk_history_nth_item_get(NULL, 0));
}
