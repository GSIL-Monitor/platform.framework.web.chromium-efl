// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstring>

#include "tizen_src/ewk/efl_integration/public/ewk_value_product.h"
#include "utc_blink_ewk_base.h"

class utc_blink_ewk_double_value : public utc_blink_ewk_base {};

TEST_F(utc_blink_ewk_double_value, POS_TEST) {
  // check type
  Ewk_Value v = ewk_value_double_new(0.0);
  EXPECT_EQ(ewk_value_type_get(v), ewk_value_double_type_get());
  ewk_value_unref(v);

  // check value range
  double d;
  for (int i = -100; i < 100; ++i) {
    double d2 = static_cast<double>(i) * M_PI;
    v = ewk_value_double_new(d2);
    ASSERT_EQ(ewk_value_double_value_get(v, &d), EINA_TRUE);
    EXPECT_EQ(d, d2);
    ewk_value_unref(v);
  }
}
