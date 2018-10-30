// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

class utc_blink_cb_uri_changed : public utc_blink_ewk_base
{
protected:
 void PostSetUp() override {
   evas_object_smart_callback_add(GetEwkWebView(), "uri,changed", uri_changed,
                                  this);
  }

  void PreTearDown() override {
    evas_object_smart_callback_del(GetEwkWebView(), "uri,changed", uri_changed);
  }

  static void uri_changed(void* data, Evas_Object* webview, void* arg)
  {
    ASSERT_TRUE(data != NULL);
    ASSERT_TRUE(arg != NULL);
    const char* url = static_cast<char*>(arg);

    utc_message("[uri_changed]\t%s", url);
    utc_blink_cb_uri_changed* owner = static_cast<utc_blink_cb_uri_changed*>(data);
    if (owner->GetResourceUrl("common/sample.html") == url)
      owner->EventLoopStop(Success);
  }
};

/**
 * @brief Tests load page with refresh meta tag.
 */
TEST_F(utc_blink_cb_uri_changed, REFRESH) {
  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), GetResourceUrl("ewk_view/redirect.html").c_str()));
  EXPECT_EQ(Success, EventLoopStart()); // Wait for "uri,changed" to .../sample.html
}

/**
 * @brief Tests load page without refresh meta tag even if previously page with
 *        refresh meta tag was loaded.
 */
TEST_F(utc_blink_cb_uri_changed, NO_REFRESH) {
  ASSERT_TRUE(ewk_view_url_set(
      GetEwkWebView(), GetResourceUrl("ewk_view/redirect.html").c_str()));
  EXPECT_EQ(Success,
            EventLoopStart());  // Wait for "uri,changed" to .../sample.html
  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), GetResourceUrl("common/sample_1.html").c_str()));
  EXPECT_EQ(Timeout, EventLoopStart(5));  // Check for not redirect
}
