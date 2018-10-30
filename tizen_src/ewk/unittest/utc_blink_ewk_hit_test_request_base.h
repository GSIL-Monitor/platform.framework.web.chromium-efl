// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UTC_BLINK_EWK_HIT_TEST_REQUEST_BASE_H
#define UTC_BLINK_EWK_HIT_TEST_REQUEST_BASE_H

#include "utc_blink_ewk_base.h"

class utc_blink_ewk_hit_test_request_base : public utc_blink_ewk_base {
protected:
 void LoadFinished(Evas_Object* webview) override

     virtual void CheckHitTest(Ewk_Hit_Test* hit_test) {}

 static void hit_test_result(Evas_Object* o,
                             int x,
                             int y,
                             int mode,
                             Ewk_Hit_Test* hit_test,
                             void* user_data) {
   utc_blink_ewk_hit_test_request_base* owner =
       static_cast<utc_blink_ewk_hit_test_request_base*>(user_data);
   ASSERT_TRUE(owner);

   owner->EventLoopStop(Success);
   owner->CheckHitTest(hit_test);
  }

};

#endif // UTC_BLINK_EWK_HIT_TEST_REQUEST_BASE_H
