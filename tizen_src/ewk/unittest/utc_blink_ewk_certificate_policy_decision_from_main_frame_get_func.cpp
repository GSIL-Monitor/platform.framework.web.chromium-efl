// Copyright 2016-17 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

static const char* const kUrlWithCertCompromiseOnMainFrame =
    "https://www.pcwebshop.co.uk/";

static const char* const kUrlWithCertCompromiseOnSubResource =
    "https://projects.dm.id.lv/Public-Key-Pins_test";

class utc_blink_ewk_certificate_policy_decision_from_main_frame_get
    : public utc_blink_ewk_base {
 protected:
  utc_blink_ewk_certificate_policy_decision_from_main_frame_get()
      : is_main_frame_(false) {}

  void PostSetUp() override {
    evas_object_smart_callback_add(GetEwkWebView(),
                                   "request,certificate,confirm",
                                   request_certificate_confirm, this);
  }

  void PreTearDown() override {
    evas_object_smart_callback_del(GetEwkWebView(),
                                   "request,certificate,confirm",
                                   request_certificate_confirm);
  }

  void LoadFinished(Evas_Object* webview) override { EventLoopStop(Failure); }

  static void request_certificate_confirm(void* data,
                                          Evas_Object* webview,
                                          void* event_info) {
    auto owner = static_cast<
        utc_blink_ewk_certificate_policy_decision_from_main_frame_get*>(data);

    owner->is_main_frame_ = ewk_certificate_policy_decision_from_main_frame_get(
        static_cast<Ewk_Certificate_Policy_Decision*>(event_info));

    owner->EventLoopStop(Success);
  }

  Eina_Bool is_main_frame_;
};

/**
 * @brief Checking whether ewk_certificate_policy_decision_from_main_frame_get
 *        returns EINA_TRUE if certificate compromise comes from main frame.
*/
TEST_F(utc_blink_ewk_certificate_policy_decision_from_main_frame_get,
       POS_TEST_CERT_ISSUE_FROM_MAIN_FRAME) {
  ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(),
                                        kUrlWithCertCompromiseOnMainFrame));
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_EQ(EINA_TRUE, is_main_frame_);
}

/**
 * @brief Checking whether ewk_certificate_policy_decision_from_main_frame_get
 *        returns EINA_FALSE if certificate compromise comes from sub resource.
*/
TEST_F(utc_blink_ewk_certificate_policy_decision_from_main_frame_get,
       POS_TEST_CERT_ISSUE_FROM_SUBRESOURCE) {
  ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(),
                                        kUrlWithCertCompromiseOnSubResource));
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_EQ(EINA_FALSE, is_main_frame_);
}

/**
* @brief Checking whether function works properly in case of NULL argument.
*/
TEST_F(utc_blink_ewk_certificate_policy_decision_from_main_frame_get,
       NEG_TEST_INVALID_PARAM) {
  ASSERT_EQ(EINA_FALSE,
            ewk_certificate_policy_decision_from_main_frame_get(nullptr));
}
