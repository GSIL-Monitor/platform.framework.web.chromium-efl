// Copyright 2016-17 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

static const char* const kUrlWithCertCompromise =
    "https://www.pcwebshop.co.uk/";

class utc_blink_ewk_certificate_policy_decision_error_get
    : public utc_blink_ewk_base
{
 protected:
   utc_blink_ewk_certificate_policy_decision_error_get()
     : error_(EWK_CERTIFICATE_POLICY_DECISION_ERROR_UNKNOWN)
   {}

   void PostSetUp() override {
     evas_object_smart_callback_add(GetEwkWebView(),
         "request,certificate,confirm", request_certificate_confirm, this);
   }

   void PreTearDown() override {
     evas_object_smart_callback_del(GetEwkWebView(),
         "request,certificate,confirm", request_certificate_confirm);
   }

   void LoadFinished(Evas_Object* webview) override {
     EventLoopStop(Failure);
   }

   static void request_certificate_confirm(void* data, Evas_Object* webview,
       void* event_info) {
     auto owner =
         static_cast<utc_blink_ewk_certificate_policy_decision_error_get*>(
             data);

     owner->error_ = ewk_certificate_policy_decision_error_get(
         static_cast<Ewk_Certificate_Policy_Decision*>(event_info));

     owner->EventLoopStop(Success);
   }

   Ewk_Certificate_Policy_Decision_Error error_;
};

/**
 * @brief Checking whether ewk_certificate_policy_decision_error_get
 *        returns know error code.
*/
TEST_F(utc_blink_ewk_certificate_policy_decision_error_get, POS_TEST) {
  ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(),
      kUrlWithCertCompromise));
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_NE(EWK_CERTIFICATE_POLICY_DECISION_ERROR_UNKNOWN, error_);
}

/**
* @brief Checking whether function works properly in case of NULL argument.
*/
TEST_F(utc_blink_ewk_certificate_policy_decision_error_get,
    NEG_TEST_INVALID_PARAM) {
  ASSERT_EQ(EWK_CERTIFICATE_POLICY_DECISION_ERROR_UNKNOWN,
      ewk_certificate_policy_decision_error_get(nullptr));
}
