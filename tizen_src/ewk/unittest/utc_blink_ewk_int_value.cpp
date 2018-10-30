// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstring>

#include "ewk_value_product.h"
#include "utc_blink_ewk_base.h"

class utc_blink_ewk_int_value : public utc_blink_ewk_base {};

TEST_F(utc_blink_ewk_int_value, POS_TEST) {
  // check type
  Ewk_Value v = ewk_value_int_new(0);
  EXPECT_EQ(ewk_value_type_get(v), ewk_value_int_type_get());
  ewk_value_unref(v);

  // check value range
  int n;
  for (int i = -100; i < 100; ++i) {
    v = ewk_value_int_new(i);
    ASSERT_EQ(ewk_value_int_value_get(v, &n), EINA_TRUE);
    EXPECT_EQ(n, i);
    ewk_value_unref(v);
  }
}
