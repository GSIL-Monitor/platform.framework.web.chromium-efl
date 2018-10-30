// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"


class utc_blink_ewk_context_application_cache_delete_all : public utc_blink_ewk_base {
protected:

  utc_blink_ewk_context_application_cache_delete_all()
    : ctx(NULL)
    , origins(NULL)
  {
  }

  ~utc_blink_ewk_context_application_cache_delete_all() override {
    SetOrigins(NULL);
  }

  void PostSetUp() override {
    utc_message("[postSetUp] :: ");
    ctx = ewk_view_context_get(GetEwkWebView());
  }

  void LoadFinished(Evas_Object* webview) override {
    utc_message("[loadFinished] :: ");
    EventLoopStop(Success);
  }

  static void origins_get_cb(Eina_List* origins, void* user_data)
  {
    utc_message("origins_get_cb: %p, %p", origins, user_data);
    ASSERT_TRUE(user_data);
    utc_blink_ewk_context_application_cache_delete_all* owner;
    owner = static_cast<utc_blink_ewk_context_application_cache_delete_all*>(user_data);

    owner->SetOrigins(origins);
    owner->EventLoopStop(Success);
  }

  void SetOrigins(Eina_List* new_origins)
  {
    utc_message("[setOrigins] :: ");
    if (new_origins != origins) {
      if (origins) {
        ewk_context_origins_free(origins);
      }
      origins = new_origins;
    }
  }

  void GetOrigins(int expected_count)
  {
    // application cache operations are async, we must wait for changes to propagate
    for (int i = 0; i < 3; ++i) {
      if (EINA_TRUE != ewk_context_application_cache_origins_get(ctx, origins_get_cb, this)) {
        break;
      }

      if (Success != EventLoopStart()) {
        break;
      }

      if (expected_count == eina_list_count(origins)) {
        break;
      }

      if (!EventLoopWait(3.0)) {
        break;
      }
    }
  }

protected:
  Ewk_Context* ctx;
  Eina_List* origins;
  static const char* const appCacheURL;
  static const char* const appCacheURLOther;
};

const char* const utc_blink_ewk_context_application_cache_delete_all::appCacheURL = "http://appcache.offline.technology/demo/";
const char* const utc_blink_ewk_context_application_cache_delete_all::appCacheURLOther = "http://www.w3schools.com/html/tryhtml5_html_manifest.htm";

/**
* @brief Tests if can delete all application cache.
*/
TEST_F(utc_blink_ewk_context_application_cache_delete_all, POS_TEST)
{
  // Load page, so all related APIs are initialized
  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), "http://www.google.pl"));
  ASSERT_EQ(utc_blink_ewk_base::Success, EventLoopStart());

  // we want empty cache at test start
  ASSERT_EQ(EINA_TRUE, ewk_context_application_cache_delete_all(ctx));
  GetOrigins(0);
  ASSERT_TRUE(origins == NULL);
  ASSERT_EQ(0, eina_list_count(origins));

  utc_message("set 1st url : %s", appCacheURL);
  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), appCacheURL));
  ASSERT_EQ(utc_blink_ewk_base::Success, EventLoopStart());

  GetOrigins(1);
  ASSERT_TRUE(origins != NULL);
  ASSERT_EQ(1, eina_list_count(origins));

  utc_message("set 2nd url : %s", appCacheURLOther);
  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), appCacheURLOther));
  ASSERT_EQ(utc_blink_ewk_base::Success, EventLoopStart());

  GetOrigins(2);
  ASSERT_TRUE(origins != NULL);
  ASSERT_EQ(2, eina_list_count(origins));

  ASSERT_EQ(EINA_TRUE, ewk_context_application_cache_delete_all(ctx));
  GetOrigins(0);
  ASSERT_EQ(0, eina_list_count(origins));

  // if there are no origins for cache, the "origins" argument passed to callback is NULL
  ASSERT_TRUE(origins == NULL);
}

/**
 * @brief Tests if it returns false which context is null.
 */
TEST_F(utc_blink_ewk_context_application_cache_delete_all, NEG_CONTEXT_NULL)
{
  EXPECT_EQ(EINA_FALSE, ewk_context_application_cache_delete_all(NULL));
}
