// Copyright 2014-2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

class utc_blink_ewk_context_did_start_download_callback_set
    : public utc_blink_ewk_base {
 protected:
  static const char* download_address;

  static void did_start_download_cb(const char* download_url, void* user_data) {
    if (user_data) {
      auto owner =
        static_cast<utc_blink_ewk_context_did_start_download_callback_set*>(user_data);
      owner->EventLoopStop(download_url ? Success : Failure);
    }
  }
};

const char* utc_blink_ewk_context_did_start_download_callback_set::download_address =
    "http://download.thinkbroadband.com/5MB.zip";

/**
 * @brief Checking whether callback function for started download is called.
 */
TEST_F(utc_blink_ewk_context_did_start_download_callback_set,
    POS_DOWNLOAD_CALLBACK_CALL) {
  ewk_context_did_start_download_callback_set(
      ewk_view_context_get(GetEwkWebView()), did_start_download_cb, this);

  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), download_address));
  ASSERT_EQ(Success, EventLoopStart());
}

/**
 * @brief Checking whether function works properly in case of NULL of a context.
 */
TEST_F(utc_blink_ewk_context_did_start_download_callback_set,
    NEG_EWK_CONTEXT_NULL) {
  ewk_context_did_start_download_callback_set(NULL, did_start_download_cb, this);
}

/**
 * @brief Checking whether function works properly in case of NULL of a callback
 *        for started download.
 */
TEST_F(utc_blink_ewk_context_did_start_download_callback_set,
    NEG_DOWNLOAD_CALLBACK_NULL) {
  ewk_context_did_start_download_callback_set(
      ewk_view_context_get(GetEwkWebView()), NULL, this);

  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), download_address));
  ASSERT_EQ(Timeout, EventLoopStart(10));
}
