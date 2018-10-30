// Copyright 2014-2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

class utc_blink_ewk_error_code_get : public utc_blink_ewk_base {
protected:
  /* Callback for load error */
 bool LoadError(Evas_Object* webview, Ewk_Error* error) override {
   utc_message("LoadError :: error: %p", error);
   if (EWK_ERROR_CODE_CANT_LOOKUP_HOST == ewk_error_code_get(error)) {
     EventLoopStop(Success);
     return true;
   }

   // returning false will cause default behaviour - exiting main loop with
   // Failure
   return false;
  }

protected:
  static const char* const test_url;
};

const char* const utc_blink_ewk_error_code_get::test_url =
    "http://page_that_does_not_exist";

/**
 * @brief Positive test case of ewk_error_code_get(). Page is loaded and stopped
 * in between to generate loadError
 */
TEST_F(utc_blink_ewk_error_code_get, POS) {
  ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(), test_url));
  ASSERT_EQ(Success, EventLoopStart());
}

/**
* @brief Checking whether function works properly in case of NULL argument.
*/
TEST_F(utc_blink_ewk_error_code_get, NEG) {
  ASSERT_EQ(0, ewk_error_code_get(NULL));
}
