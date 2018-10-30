// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can b
// found in the LICENSE file.

#ifndef EWK_UNITTEST_UTC_BLINK_EWK_INTERCEPT_REQUEST_RESPONSE_BASE_H_
#define EWK_UNITTEST_UTC_BLINK_EWK_INTERCEPT_REQUEST_RESPONSE_BASE_H_

#include "utc_blink_ewk_base.h"

#include <string>

#include "utc_blink_api_result_locked.h"

class utc_blink_ewk_intercept_request_response_base
    : public utc_blink_ewk_base {
 protected:
  utc_blink_ewk_intercept_request_response_base();
  ~utc_blink_ewk_intercept_request_response_base() override;

  virtual void pos_func(Ewk_Intercept_Request* intercept_request) = 0;
  virtual bool neg_func(Ewk_Intercept_Request* intercept_request) = 0;
  virtual std::string get_js_title_test() = 0;

  void PostSetUp() override;
  void PreTearDown() override;
  void LoadFinished(Evas_Object* webview) override;

  void pos_test();
  void neg_test();

  const char* body_ajax_response_;
  const char* custom_response_status_text_;
  bool intercept_request_null_;

 private:
  struct ewk_callback_results {
    bool status_set_;
    bool header_add_content_type_;
    bool header_add_content_length_;
    bool header_add_access_control_;
    bool body_set_;
  };

  static const char* kInterceptURL;
  static const char* kInterceptAjaxURL;
  static const char* kBodyAjaxPart1;
  static const char* kBodyAjaxPart2;
  static const char* kBodyAjaxPart3;

  void check_results(ApiResultLocked<ewk_callback_results>& results);
  void construct_page();

  static void title_changed(void* data, Evas_Object* webview, void* event_info);

  static void intercept_request_callback_response_header_add(
      Ewk_Context* /*ctx*/,
      Ewk_Intercept_Request* intercept_request,
      void* user_data);

  static void intercept_request_callback_response_header_add_null(
      Ewk_Context* /*ctx*/,
      Ewk_Intercept_Request* intercept_request,
      void* user_data);

  ApiResultLocked<ewk_callback_results> page_ewk_results_;
  ApiResultLocked<ewk_callback_results> ajax_ewk_results_;
  ApiResultLocked<Eina_Bool> neg_api_result_;
  std::string first_title_;
  std::string body_ajax_;
};

#endif  // EWK_UNITTEST_UTC_BLINK_EWK_INTERCEPT_REQUEST_RESPONSE_BASE_H_
