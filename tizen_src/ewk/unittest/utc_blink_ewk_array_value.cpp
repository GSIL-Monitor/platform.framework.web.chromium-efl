// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstring>

#include "ewk_value_product.h"
#include "utc_blink_ewk_base.h"
#include "utc_blink_ewk_value_compare.h"

class utc_blink_ewk_array_value : public utc_blink_ewk_base {};

/*
Tested structure layout:
[ bool, int, double, string, array[bool, int, double] ]
*/
TEST_F(utc_blink_ewk_array_value, POS_TEST) {
  // check type
  Ewk_Value arr = ewk_value_array_new();
  EXPECT_EQ(ewk_value_type_get(arr), ewk_value_array_type_get());
  ASSERT_EQ(ewk_value_array_count(arr), 0u);
  ASSERT_EQ(ewk_value_array_is_mutable(arr), EINA_TRUE);

  // add bool
  Ewk_Value v = ewk_value_boolean_new(EINA_TRUE);
  ewk_value_array_append(arr, v);
  ewk_value_unref(v);
  ASSERT_EQ(ewk_value_array_count(arr), 1u);

  // add double
  v = ewk_value_double_new(1.23f);
  ewk_value_array_append(arr, v);
  ewk_value_unref(v);
  ASSERT_EQ(ewk_value_array_count(arr), 2u);

  // add int
  v = ewk_value_int_new(123);
  ewk_value_array_append(arr, v);
  ewk_value_unref(v);
  ASSERT_EQ(ewk_value_array_count(arr), 3u);

  // add string
  v = ewk_value_string_new("test_string");
  ewk_value_array_append(arr, v);
  ewk_value_unref(v);
  ASSERT_EQ(ewk_value_array_count(arr), 4u);

  // add another array
  Ewk_Value arr2 = ewk_value_array_new();
  // add bool
  v = ewk_value_boolean_new(EINA_TRUE);
  ewk_value_array_append(arr2, v);
  ewk_value_unref(v);
  ASSERT_EQ(ewk_value_array_count(arr2), 1u);
  // add double
  v = ewk_value_double_new(1.23f);
  ewk_value_array_append(arr2, v);
  ewk_value_unref(v);
  ASSERT_EQ(ewk_value_array_count(arr2), 2u);
  // add int
  v = ewk_value_int_new(123);
  ewk_value_array_append(arr2, v);
  ewk_value_unref(v);
  ASSERT_EQ(ewk_value_array_count(arr2), 3u);

  // append te second array to array
  ewk_value_array_append(arr, arr2);
  ewk_value_unref(arr2);
  ASSERT_EQ(ewk_value_array_count(arr), 5u);

  // check the values
  // bool
  ASSERT_EQ(ewk_value_array_get(arr, 0, &v), EINA_TRUE);
  EXPECT_EQ(ewk_value_type_get(v), ewk_value_boolean_type_get());
  Eina_Bool b;
  ASSERT_EQ(ewk_value_boolean_value_get(v, &b), EINA_TRUE);
  EXPECT_EQ(b, EINA_TRUE);
  ewk_value_unref(v);

  // double
  ASSERT_EQ(ewk_value_array_get(arr, 1, &v), EINA_TRUE);
  EXPECT_EQ(ewk_value_type_get(v), ewk_value_double_type_get());
  double d;
  ASSERT_EQ(ewk_value_double_value_get(v, &d), EINA_TRUE);
  EXPECT_EQ(d, 1.23f);
  ewk_value_unref(v);

  // int
  ASSERT_EQ(ewk_value_array_get(arr, 2, &v), EINA_TRUE);
  EXPECT_EQ(ewk_value_type_get(v), ewk_value_int_type_get());
  int i;
  ASSERT_EQ(ewk_value_int_value_get(v, &i), EINA_TRUE);
  EXPECT_EQ(i, 123);
  ewk_value_unref(v);

  // string
  ASSERT_EQ(ewk_value_array_get(arr, 3, &v), EINA_TRUE);
  EXPECT_EQ(ewk_value_type_get(v), ewk_value_string_type_get());
  Eina_Stringshare* str = ewk_value_string_value_get(v);
  ASSERT_TRUE(str != NULL);
  EXPECT_STREQ((const char*)str, "test_string");
  eina_stringshare_del(str);
  ewk_value_unref(v);

  // array
  ASSERT_EQ(ewk_value_array_get(arr, 4, &arr2), EINA_TRUE);
  EXPECT_EQ(ewk_value_type_get(arr2), ewk_value_array_type_get());
  // check values inside the array
  // bool
  ASSERT_EQ(ewk_value_array_get(arr2, 0, &v), EINA_TRUE);
  EXPECT_EQ(ewk_value_type_get(v), ewk_value_boolean_type_get());
  ASSERT_EQ(ewk_value_boolean_value_get(v, &b), EINA_TRUE);
  EXPECT_EQ(b, EINA_TRUE);
  ewk_value_unref(v);
  // double
  ASSERT_EQ(ewk_value_array_get(arr2, 1, &v), EINA_TRUE);
  EXPECT_EQ(ewk_value_type_get(v), ewk_value_double_type_get());
  ASSERT_EQ(ewk_value_double_value_get(v, &d), EINA_TRUE);
  EXPECT_EQ(d, 1.23f);
  ewk_value_unref(v);
  // int
  ASSERT_EQ(ewk_value_array_get(arr2, 2, &v), EINA_TRUE);
  EXPECT_EQ(ewk_value_type_get(v), ewk_value_int_type_get());
  ASSERT_EQ(ewk_value_int_value_get(v, &i), EINA_TRUE);
  EXPECT_EQ(i, 123);
  ewk_value_unref(v);
  ewk_value_unref(arr2);

  // remove some values
  // try to remove out of scope
  EXPECT_EQ(ewk_value_array_remove(arr, 10), EINA_FALSE);
  ASSERT_EQ(ewk_value_array_count(arr), 5u);

  // remove bool
  EXPECT_EQ(ewk_value_array_remove(arr, 0), EINA_TRUE);
  ASSERT_EQ(ewk_value_array_count(arr), 4u);

  // remove int
  EXPECT_EQ(ewk_value_array_remove(arr, 1), EINA_TRUE);
  ASSERT_EQ(ewk_value_array_count(arr), 3u);

  // check values that are left
  // double
  ASSERT_EQ(ewk_value_array_get(arr, 0, &v), EINA_TRUE);
  EXPECT_EQ(ewk_value_type_get(v), ewk_value_double_type_get());
  ASSERT_EQ(ewk_value_double_value_get(v, &d), EINA_TRUE);
  EXPECT_EQ(d, 1.23f);
  ewk_value_unref(v);

  // string
  ASSERT_EQ(ewk_value_array_get(arr, 1, &v), EINA_TRUE);
  EXPECT_EQ(ewk_value_type_get(v), ewk_value_string_type_get());
  str = ewk_value_string_value_get(v);
  ASSERT_TRUE(str != NULL);
  EXPECT_STREQ((const char*)str, "test_string");
  eina_stringshare_del(str);
  ewk_value_unref(v);

  // try to get value out of scope
  EXPECT_EQ(ewk_value_array_get(arr, 3, &v), EINA_FALSE);

  ewk_value_unref(arr);
}
