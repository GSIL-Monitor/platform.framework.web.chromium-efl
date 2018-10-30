// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

class utc_blink_ewk_context_proxy_set : public utc_blink_ewk_base {
 protected:
  static const char* proxy;
  static const char* bypass_rule;
};

const char* utc_blink_ewk_context_proxy_set::proxy = "foopy:3128";
const char* utc_blink_ewk_context_proxy_set::bypass_rule = "<local>";

/**
 * @brief Checking whether interface works properly in case of NULL of context.
 */
TEST_F(utc_blink_ewk_context_proxy_set, NEG_CONTEXT_NULL) {
  ewk_context_proxy_set(NULL, proxy, bypass_rule);
  const char* proxy_url = ewk_context_proxy_uri_get(ewk_context_default_get());
  const char* bypass_rule_str =
      ewk_context_proxy_bypass_rule_get(ewk_context_default_get());

  ASSERT_STREQ(proxy_url, "");
  ASSERT_STREQ(bypass_rule_str, "");
}

/**
 * @brief Checking whether interface can be setting with bypass_rule.
 */
TEST_F(utc_blink_ewk_context_proxy_set, POS) {
  ewk_context_proxy_set(ewk_context_default_get(), proxy, bypass_rule);
  const char* proxy_url = ewk_context_proxy_uri_get(ewk_context_default_get());
  const char* bypass_rule_str =
      ewk_context_proxy_bypass_rule_get(ewk_context_default_get());

  ASSERT_STREQ(proxy_url, proxy);
  ASSERT_STREQ(bypass_rule_str, bypass_rule);
}

/**
 * @brief Checking whether interface can be setting without bypass_rule.
 */
TEST_F(utc_blink_ewk_context_proxy_set, NEG_BYPASS_NULL) {
  ewk_context_proxy_set(ewk_context_default_get(), proxy, NULL);
  const char* proxy_url = ewk_context_proxy_uri_get(ewk_context_default_get());
  const char* bypass_rule_str =
      ewk_context_proxy_bypass_rule_get(ewk_context_default_get());

  ASSERT_STREQ(proxy_url, proxy);
  ASSERT_STREQ(bypass_rule_str, "");
}

/**
 * @brief Checking whether interface works properly in case of
 *        both proxy and passby rule are NULL.
 */
TEST_F(utc_blink_ewk_context_proxy_set, NEG_BYPASS_URI_NULL) {
  ewk_context_proxy_set(ewk_context_default_get(), NULL, NULL);
  const char* proxy_url = ewk_context_proxy_uri_get(ewk_context_default_get());
  const char* bypass_rule_str =
      ewk_context_proxy_bypass_rule_get(ewk_context_default_get());

  ASSERT_STREQ(proxy_url, "");
  ASSERT_STREQ(bypass_rule_str, "");
}
