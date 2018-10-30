// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstring>

#include "ewk_value_product.h"
#include "utc_blink_ewk_base.h"
#include "utc_blink_ewk_value_compare.h"

class utc_blink_ewk_string_value : public utc_blink_ewk_base {};

TEST_F(utc_blink_ewk_string_value, POS_TEST) {
  // check type
  Ewk_Value v = ewk_value_string_new("");
  EXPECT_EQ(ewk_value_type_get(v), ewk_value_string_type_get());
  ewk_value_unref(v);

  // check value range
  for (size_t i = 0; i < ArraySize(test_strings); ++i) {
    v = ewk_value_string_new(test_strings[i]);
    Eina_Stringshare* str = ewk_value_string_value_get(v);
    ASSERT_TRUE(str != NULL);
    EXPECT_STREQ((const char*)str, test_strings[i]);
    eina_stringshare_del(str);
    ewk_value_unref(v);
  }
}
