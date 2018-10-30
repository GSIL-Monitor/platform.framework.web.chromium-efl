// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_UNITTEST_UTC_BLINK_EWK_INTERCEPT_REQUEST_GET_BASE_H_
#define EWK_UNITTEST_UTC_BLINK_EWK_INTERCEPT_REQUEST_GET_BASE_H_

#include "utc_blink_ewk_base.h"

#include <atomic>

class utc_blink_ewk_intercept_request_get_base : public utc_blink_ewk_base {
 protected:
  static const std::string kInterceptURL;

  utc_blink_ewk_intercept_request_get_base();
  ~utc_blink_ewk_intercept_request_get_base() override;

  void LoadFinished(Evas_Object* webview) override;
  void pos_test();
  void neg_test();
  virtual void test_func(Ewk_Intercept_Request* intercept_request) = 0;

 private:
  static void callback_positive(Ewk_Context* /*ctx*/,
                                Ewk_Intercept_Request* intercept_request,
                                void* user_data);
  static void callback_negative(Ewk_Context* /*ctx*/,
                                Ewk_Intercept_Request* /*intercept_request*/,
                                void* user_data);

  std::atomic<bool> api_executed_;
};

#endif  // EWK_UNITTEST_UTC_BLINK_EWK_INTERCEPT_REQUEST_GET_BASE_H_
