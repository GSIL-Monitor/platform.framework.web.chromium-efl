// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/* Define those macros _before_ you include the utc_blink_ewk.h header file. */

#include "utc_blink_ewk_base.h"

#define SAMPLE_ORIGIN "jmajnert.github.io"
const char* const WEB_STORAGE_SAMPLE_URL =
    "http://" SAMPLE_ORIGIN
    "/tests/"
    "localStorage_test/web_storage_create_simple.html";

class utc_blink_ewk_context_web_storage_usage_for_origin_get
    : public utc_blink_ewk_base {
 protected:
  utc_blink_ewk_context_web_storage_usage_for_origin_get()
      : utc_blink_ewk_base(), context_(nullptr), usage_(0), neg_test_(false) {}

  void PostSetUp() override {
    ASSERT_TRUE(context_ = ewk_view_context_get(GetEwkWebView()));
    // we want empty web storage at test start
    ASSERT_TRUE(ewk_context_web_storage_delete_all(context_));
  }

  void LoadFinished(Evas_Object*) override { EventLoopStop(Success); }

  void NegTest(Eina_List* origins) {
    utc_message("NegTest: %p", origins);

    Ewk_Security_Origin* origin = nullptr;
    Ewk_Security_Origin* chosen_origin = nullptr;
    Eina_List* it;
    void* vorigin = nullptr;
    EINA_LIST_FOREACH(origins, it, vorigin) {
      origin = static_cast<Ewk_Security_Origin*>(vorigin);

      const char* host = ewk_security_origin_host_get(origin);
      if (host && !strcmp(host, SAMPLE_ORIGIN)) {
        chosen_origin = origin;
        break;
      }
    }
    if (!chosen_origin)
      EventLoopStop(Failure);
    ASSERT_FALSE(ewk_context_web_storage_usage_for_origin_get(
        nullptr, origin, usage_get_cb, this));
    ASSERT_FALSE(ewk_context_web_storage_usage_for_origin_get(
        context_, nullptr, usage_get_cb, this));
    ASSERT_FALSE(ewk_context_web_storage_usage_for_origin_get(context_, origin,
                                                              nullptr, this));
    EventLoopStop(Success);
  }

  void FindSecurityOrigin(Eina_List* origins) {
    utc_message("FindSecurityOrigin: %p", origins);

    Ewk_Security_Origin* origin = nullptr;
    Eina_List* it;
    void* vorigin = nullptr;
    EINA_LIST_FOREACH(origins, it, vorigin) {
      origin = static_cast<Ewk_Security_Origin*>(vorigin);

      const char* host = ewk_security_origin_host_get(origin);
      if (host && !strcmp(host, SAMPLE_ORIGIN)) {
        ASSERT_TRUE(ewk_context_web_storage_usage_for_origin_get(
            context_, origin, usage_get_cb, this));
        return;
      }
    }
    EventLoopStop(Failure);
  }

  static void origins_get_cb(Eina_List* origins, void* user_data) {
    utc_message("origins_get_cb: %p, %p", origins, user_data);
    if (!user_data) {
      ewk_context_origins_free(origins);
      FAIL();
    }

    auto* self =
        static_cast<utc_blink_ewk_context_web_storage_usage_for_origin_get*>(
            user_data);

    if (!origins) {
      self->EventLoopStop(Failure);
      return;
    }

    if (self->neg_test_)
      self->NegTest(origins);
    else
      self->FindSecurityOrigin(origins);
  }

  static void usage_get_cb(uint64_t usage, void* user_data) {
    utc_message("usage_get_cb: %lu, %p", usage, user_data);
    if (!user_data) {
      FAIL();
      return;
    }

    auto* self =
        static_cast<utc_blink_ewk_context_web_storage_usage_for_origin_get*>(
            user_data);

    self->usage_ = usage;
    self->EventLoopStop(Success);
  }

 protected:
  Ewk_Context* context_;
  uint64_t usage_;
  bool neg_test_;
};

/**
* @brief Tests if usage can be obtained when localstorage is used
*/
TEST_F(utc_blink_ewk_context_web_storage_usage_for_origin_get, POS_TEST) {
  utc_message("Loading web page: %s", WEB_STORAGE_SAMPLE_URL);
  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), WEB_STORAGE_SAMPLE_URL));
  ASSERT_EQ(Success, EventLoopStart());

  EventLoopWait(10.0);
  ASSERT_TRUE(
      ewk_context_web_storage_origins_get(context_, origins_get_cb, this));
  ASSERT_EQ(Success, EventLoopStart());

  ASSERT_TRUE(usage_);
}

/**
* @brief Tests if returns false when parameters are null.
*/
TEST_F(utc_blink_ewk_context_web_storage_usage_for_origin_get, NEG_TEST) {
  utc_message("Loading web page: %s", WEB_STORAGE_SAMPLE_URL);
  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), WEB_STORAGE_SAMPLE_URL));
  ASSERT_EQ(Success, EventLoopStart());

  EventLoopWait(10.0);
  neg_test_ = true;
  ASSERT_TRUE(
      ewk_context_web_storage_origins_get(context_, origins_get_cb, this));
  ASSERT_EQ(Success, EventLoopStart());
}
