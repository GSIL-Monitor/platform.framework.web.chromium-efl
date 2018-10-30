// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstring>

#include "ewk_value_product.h"
#include "utc_blink_ewk_base.h"

class utc_blink_ewk_boolean_value : public utc_blink_ewk_base {};

TEST_F(utc_blink_ewk_boolean_value, POS_TEST) {
  // check type
  Ewk_Value v = ewk_value_boolean_new(EINA_TRUE);
  EXPECT_EQ(ewk_value_type_get(v), ewk_value_boolean_type_get());

  // check for true
  Eina_Bool b;
  ASSERT_EQ(ewk_value_boolean_value_get(v, &b), EINA_TRUE);
  EXPECT_EQ(b, EINA_TRUE);
  ewk_value_unref(v);

  // check for false
  v = ewk_value_boolean_new(EINA_FALSE);
  ASSERT_EQ(ewk_value_boolean_value_get(v, &b), EINA_TRUE);
  EXPECT_EQ(b, EINA_FALSE);
  ewk_value_unref(v);
}
