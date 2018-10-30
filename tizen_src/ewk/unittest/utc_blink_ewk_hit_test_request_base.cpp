// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ewk/unittest/utc_blink_ewk_hit_test_request_base.h"

void utc_blink_ewk_hit_test_request_base::LoadFinished(Evas_Object* webview) {
  EventLoopStop(utc_blink_ewk_base::Success);
}
