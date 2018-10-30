// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <atomic>
#include <cstring>
#include <string>

#include "ewk/unittest/utc_blink_ewk_base.h"

class utc_blink_ewk_intercept_request_response_write_chunk
    : public utc_blink_ewk_base {
 public:
  utc_blink_ewk_intercept_request_response_write_chunk()
      : status_set_result_(false),
        content_type_header_result_(false),
        content_length_header_result_(false),
        last_write_(true),
        null_intercept_request_(false),
        null_chunk_(false),
        data_written_(0),
        chunk_write_timer_(nullptr),
        intercept_request_(nullptr),
        callback_called_(false) {
    body_.append(kBodyPre);
    body_.append(kTitle);
    body_.append(kBodyPost);
    body_length_ = body_.length();
    data_to_write_ = body_length_;
  }

 protected:
  static const char* kInterceptURL;
  static const char* kBodyPre;
  static const char* kTitle;
  static const char* kBodyPost;
  // Chunk should be smaller than payload for this test.
  static const size_t kDefaultChunkLength;

  void LoadFinished(Evas_Object* webview) override { EventLoopStop(Success); }

  void PostSetUp() override {
    ewk_context_intercept_request_callback_set(
        ewk_view_context_get(GetEwkWebView()), intercept_request_callback,
        this);
  }

  void PreTearDown() override {
    if (chunk_write_timer_)
      ecore_timer_del(chunk_write_timer_);
  }

  void PrepareAndStartChunkWrite(MainLoopResult expected_result) {
    ASSERT_TRUE(ewk_view_url_set(GetEwkWebView(), kInterceptURL));
    ASSERT_EQ(Timeout, EventLoopStart(3.0));
    ASSERT_TRUE(callback_called_.load());
    EXPECT_TRUE(status_set_result_);
    EXPECT_TRUE(content_type_header_result_);
    EXPECT_TRUE(content_length_header_result_);
    SetTestJob(utc_blink_ewk_intercept_request_response_write_chunk::
                   job_do_chunk_write);
    ASSERT_EQ(expected_result, EventLoopStart());
  }

  static void intercept_request_callback(
      Ewk_Context* /*ctx*/,
      Ewk_Intercept_Request* intercept_request,
      void* user_data) {
    auto* owner =
        static_cast<utc_blink_ewk_intercept_request_response_write_chunk*>(
            user_data);

    const char* url = ewk_intercept_request_url_get(intercept_request);
    if (strcmp(url, kInterceptURL) == 0) {
      owner->status_set_result_ = ewk_intercept_request_response_status_set(
          intercept_request, 200, nullptr);
      owner->content_type_header_result_ =
          ewk_intercept_request_response_header_add(
              intercept_request, "Content-Type", "text/html; charset=UTF-8");
      owner->content_length_header_result_ =
          ewk_intercept_request_response_header_add(
              intercept_request, "Content-Length",
              std::to_string(owner->body_length_).c_str());
      if (owner->null_intercept_request_)
        owner->intercept_request_ = nullptr;
      else
        owner->intercept_request_ = intercept_request;
    } else {
      // Ignore any other request.
      ewk_intercept_request_ignore(intercept_request);
    }
    owner->callback_called_.store(true);
  }

  static Eina_Bool chunk_write_callback(void* data) {
    auto* owner =
        static_cast<utc_blink_ewk_intercept_request_response_write_chunk*>(
            data);
    size_t to_write = std::min(kDefaultChunkLength, owner->data_to_write_);
    if (to_write) {
      const char* chunk = owner->null_chunk_
                              ? nullptr
                              : owner->body_.c_str() + owner->data_written_;
      Eina_Bool write_success = ewk_intercept_request_response_write_chunk(
          owner->intercept_request_, chunk, to_write);
      if (!write_success) {
        owner->EventLoopStop(Failure);
        owner->chunk_write_timer_ = nullptr;
        return ECORE_CALLBACK_CANCEL;
      }
      owner->data_to_write_ -= to_write;
      owner->data_written_ += to_write;
      return ECORE_CALLBACK_RENEW;
    } else {
      owner->last_write_ = ewk_intercept_request_response_write_chunk(
          owner->intercept_request_, nullptr, 0);
      owner->chunk_write_timer_ = nullptr;
      return ECORE_CALLBACK_CANCEL;
    }
  }

  static void job_do_chunk_write(utc_blink_ewk_base* data) {
    auto owner =
        static_cast<utc_blink_ewk_intercept_request_response_write_chunk*>(
            data);
    // Writing happens outside of callback.
    // Use timer with non-zero timeout to show async nature of this API.
    owner->chunk_write_timer_ =
        ecore_timer_add(0.01, (Ecore_Task_Cb)chunk_write_callback, owner);
  }

  bool status_set_result_;
  bool content_type_header_result_;
  bool content_length_header_result_;
  bool last_write_;

  bool null_intercept_request_;
  bool null_chunk_;
  size_t data_to_write_;
  size_t data_written_;
  std::string body_;
  size_t body_length_;
  Ecore_Timer* chunk_write_timer_;
  Ewk_Intercept_Request* intercept_request_;
  std::atomic<bool> callback_called_;
};

const char*
    utc_blink_ewk_intercept_request_response_write_chunk::kInterceptURL =
        "http://request.intercept.ewk.api.test/";

const char* utc_blink_ewk_intercept_request_response_write_chunk::kBodyPre =
    "<html><head><title>";
const char* utc_blink_ewk_intercept_request_response_write_chunk::kBodyPost =
    "</title></head>"
    "<body>Hello, Request Intercept!</body></html>";
const char* utc_blink_ewk_intercept_request_response_write_chunk::kTitle =
    "CHUNKED WRITE SUCCESS";
const size_t
    utc_blink_ewk_intercept_request_response_write_chunk::kDefaultChunkLength =
        5;

/**
 * @brief Tests if writing response in chunks for intercepted request results
 *        in expected web page.
 */
TEST_F(utc_blink_ewk_intercept_request_response_write_chunk,
       POS_TEST_WRITE_RESPONSE_IN_CHUNKS) {
  PrepareAndStartChunkWrite(Success);
  EXPECT_FALSE(last_write_);
  EXPECT_STREQ(kTitle, ewk_view_title_get(GetEwkWebView()));
}

/**
 * @brief Tests if EINA_FALSE is returned for null intercept request.
 */
TEST_F(utc_blink_ewk_intercept_request_response_write_chunk,
       NEG_TEST_NULL_INTERCEPT_REQUEST) {
  null_intercept_request_ = true;
  PrepareAndStartChunkWrite(Failure);
}

/**
 * @brief Tests if EINA_FALSE is returned for null body chunk.
 */
TEST_F(utc_blink_ewk_intercept_request_response_write_chunk,
       NEG_TEST_NULL_BODY_CHUNK) {
  null_chunk_ = true;
  PrepareAndStartChunkWrite(Failure);
}
