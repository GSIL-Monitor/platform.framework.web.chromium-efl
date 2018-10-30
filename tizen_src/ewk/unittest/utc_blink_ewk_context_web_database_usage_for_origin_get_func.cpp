// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

#define SAMPLE_ORIGIN "dwieleverece.github.io"
const char* const WEB_DATABASE_SAMPLE_URL =
    "https://" SAMPLE_ORIGIN "/tests/web_database_test/";

class utc_blink_ewk_context_web_database_usage_for_origin_get
    : public utc_blink_ewk_base {
 protected:
  utc_blink_ewk_context_web_database_usage_for_origin_get()
      : context_(nullptr),
        usage_(0),
        neg_test_(false),
        neg_result_1_(false),
        neg_result_2_(false),
        neg_result_3_(false),
        usage_get_result_(false) {}

  void PostSetUp() override {
    ASSERT_TRUE(context_ = ewk_view_context_get(GetEwkWebView()));
    // we want empty web database at test start
    ASSERT_TRUE(ewk_context_web_database_delete_all(context_));
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
    neg_result_1_ = ewk_context_web_database_usage_for_origin_get(
        nullptr, UsageGetCb, this, origin);
    neg_result_2_ = ewk_context_web_database_usage_for_origin_get(
        context_, nullptr, this, origin);
    neg_result_3_ = ewk_context_web_database_usage_for_origin_get(
        context_, UsageGetCb, this, nullptr);
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
        usage_get_result_ = ewk_context_web_database_usage_for_origin_get(
            context_, UsageGetCb, this, origin);
        EventLoopStop(Success);
      }
    }
    EventLoopStop(Failure);
  }

  static void OriginsGetCb(Eina_List* origins, void* user_data) {
    utc_message("OriginsGetCb: %p, %p", origins, user_data);
    if (!user_data) {
      ewk_context_origins_free(origins);
      return;
    }

    auto self =
        static_cast<utc_blink_ewk_context_web_database_usage_for_origin_get*>(
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

  static void UsageGetCb(uint64_t usage, void* user_data) {
    utc_message("UsageGetCb: %lu, %p", usage, user_data);
    if (!user_data)
      return;

    auto self =
        static_cast<utc_blink_ewk_context_web_database_usage_for_origin_get*>(
            user_data);

    self->usage_ = usage;
    self->EventLoopStop(Success);
  }

 protected:
  Ewk_Context* context_;
  uint64_t usage_;
  bool neg_test_;
  bool neg_result_1_;
  bool neg_result_2_;
  bool neg_result_3_;
  bool usage_get_result_;
};

/**
* @brief Tests if usage can be obtained when websql is used
*/
TEST_F(utc_blink_ewk_context_web_database_usage_for_origin_get, POS_TEST) {
  utc_message("Loading web page: %s", WEB_DATABASE_SAMPLE_URL);
  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), WEB_DATABASE_SAMPLE_URL));
  ASSERT_EQ(Success, EventLoopStart());

  // testcase is hosted on github pages (github.io) for free, so it can be slow
  EventLoopWait(10.0);
  ASSERT_TRUE(
      ewk_context_web_database_origins_get(context_, OriginsGetCb, this));
  ASSERT_EQ(Success, EventLoopStart());

  ASSERT_TRUE(usage_get_result_);
  ASSERT_GT(usage_, 0);
}

/**
* @brief Tests if returns false when parameters are null.
*/
TEST_F(utc_blink_ewk_context_web_database_usage_for_origin_get, NEG_TEST) {
  utc_message("Loading web page: %s", WEB_DATABASE_SAMPLE_URL);
  ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), WEB_DATABASE_SAMPLE_URL));
  ASSERT_EQ(Success, EventLoopStart());

  // testcase is hosted on github pages (github.io) for free, so it can be slow
  EventLoopWait(10.0);
  neg_test_ = true;
  ASSERT_TRUE(
      ewk_context_web_database_origins_get(context_, OriginsGetCb, this));
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_FALSE(neg_result_1_);
  ASSERT_FALSE(neg_result_2_);
  ASSERT_FALSE(neg_result_3_);
}
