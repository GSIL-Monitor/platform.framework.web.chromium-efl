// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

class utc_blink_ewk_cookie_manager_cookies_clear : public utc_blink_ewk_base {
protected:
  static void getHostnamesWithCookiesCallback(Eina_List* hostnames, Ewk_Error* error, void* event_info)
  {
    ASSERT_TRUE(error == 0);
    ASSERT_TRUE(event_info != NULL);

    utc_blink_ewk_cookie_manager_cookies_clear* owner = static_cast<utc_blink_ewk_cookie_manager_cookies_clear*>(event_info);

    owner->host_count = eina_list_count(hostnames);
    owner->EventLoopStop(utc_blink_ewk_base::Success);
  }

  void LoadFinished(Evas_Object* webview) override {
    EventLoopStop(utc_blink_ewk_base::Success);
  }

protected:
  int host_count;
};

/**
 * @brief Positive test case of ewk_cookie_manager_cookies_clear()
 */
TEST_F(utc_blink_ewk_cookie_manager_cookies_clear, POS_TEST)
{
  Ewk_Cookie_Manager* cookieManager = ewk_context_cookie_manager_get(ewk_view_context_get(GetEwkWebView()));
  ASSERT_TRUE(cookieManager != NULL);

  ewk_cookie_manager_accept_policy_set(cookieManager, EWK_COOKIE_ACCEPT_POLICY_ALWAYS);

  ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(), "http://www.google.com"));
  ASSERT_EQ(Success, EventLoopStart());

  ewk_cookie_manager_async_hostnames_with_cookies_get(cookieManager, getHostnamesWithCookiesCallback, this);
  ASSERT_EQ(Success, EventLoopStart());

  ASSERT_NE(host_count, 0);

  // Second page load
  ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(), "http://m.naver.com"));
  ASSERT_EQ(Success, EventLoopStart());

  ewk_cookie_manager_async_hostnames_with_cookies_get(cookieManager, getHostnamesWithCookiesCallback, this);
  ASSERT_EQ(Success, EventLoopStart());

  ASSERT_NE(host_count, 0);

  // Clear all cookies
  ewk_cookie_manager_cookies_clear(cookieManager);

  ewk_cookie_manager_async_hostnames_with_cookies_get(cookieManager, getHostnamesWithCookiesCallback, this);
  ASSERT_EQ(Success, EventLoopStart());

  EXPECT_EQ(host_count, 0);
}

/**
* @brief Checking whether function works properly in case of NULL is sent.
*/
TEST_F(utc_blink_ewk_cookie_manager_cookies_clear, NEG_TEST)
{
  ewk_cookie_manager_cookies_clear(NULL);
}

