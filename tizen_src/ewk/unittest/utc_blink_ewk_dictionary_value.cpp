// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstring>

#include "ewk_value_product.h"
#include "utc_blink_ewk_base.h"
#include "utc_blink_ewk_value_compare.h"

class utc_blink_ewk_dictionary_value : public utc_blink_ewk_base {};

/*
Tested structure layout:
- bool_key -> bool
- int_key -> int
- double_key -> double
- string key -> string
- array_key -> array [bool, int, double]
- dict_key -> dictionary
    - bool_key -> bool
    - int_key -> int
    - double_key -> double
*/
TEST_F(utc_blink_ewk_dictionary_value, POS_TEST) {
  Ewk_Value dict = ewk_value_dictionary_new();
  EXPECT_EQ(ewk_value_type_get(dict), ewk_value_dictionary_type_get());
  ASSERT_EQ(ewk_value_dictionary_is_mutable(dict), EINA_TRUE);

  // prepare test keys
  Ewk_Value test_values[ArraySize(test_keys)];
  test_values[BOOL_KEY] = ewk_value_boolean_new(EINA_TRUE);
  test_values[DOUBLE_KEY] = ewk_value_double_new(1.23f);
  test_values[INT_KEY] = ewk_value_int_new(123);
  test_values[STRING_KEY] = ewk_value_string_new("test_string");
  test_values[ARRAY_KEY] = ewk_value_array_new();
  ewk_value_array_append(test_values[ARRAY_KEY], test_values[BOOL_KEY]);
  ewk_value_array_append(test_values[ARRAY_KEY], test_values[DOUBLE_KEY]);
  ewk_value_array_append(test_values[ARRAY_KEY], test_values[INT_KEY]);
  test_values[DICT_KEY] = ewk_value_dictionary_new();

  Eina_Bool b;

  // set values in the nested dictionary
  for (size_t i = BOOL_KEY; i < STRING_KEY; ++i) {
    Ewk_Value k = ewk_value_string_new(test_keys[i]);
    ASSERT_EQ(
        ewk_value_dictionary_add(test_values[DICT_KEY], k, test_values[i], &b),
        EINA_TRUE);
    EXPECT_EQ(b, EINA_TRUE);
    ewk_value_unref(k);
  }

  // set all dictionary values
  for (size_t i = BOOL_KEY; i < KEY_COUNT; ++i) {
    Ewk_Value k = ewk_value_string_new(test_keys[i]);
    ASSERT_EQ(ewk_value_dictionary_add(dict, k, test_values[i], &b), EINA_TRUE);
    EXPECT_EQ(b, EINA_TRUE);
    ewk_value_unref(k);
  }

  // get keys array
  Ewk_Value keys;
  ASSERT_EQ(ewk_value_dictionary_keys(dict, &keys), EINA_TRUE);
  ASSERT_EQ(ewk_value_type_get(keys), ewk_value_array_type_get());
  ASSERT_EQ(ewk_value_array_is_mutable(keys), EINA_TRUE);
  ASSERT_EQ(ewk_value_array_count(keys), ArraySize(test_keys));

  for (size_t i = BOOL_KEY; i < KEY_COUNT; ++i) {
    Ewk_Value key;
    ASSERT_EQ(ewk_value_array_get(keys, i, &key), EINA_TRUE);
    Eina_Stringshare* key_str = ewk_value_string_value_get(key);

    // get index of the key in test data
    size_t idx;
    for (idx = BOOL_KEY; idx < KEY_COUNT; ++idx) {
      if (strcmp(static_cast<const char*>(key_str), test_keys[idx]) == 0)
        break;
    }

    Ewk_Value v;
    ASSERT_EQ(ewk_value_dictionary_get(dict, key, &v), EINA_TRUE);
    EXPECT_TRUE(Compare(v, test_values[idx]));

    ewk_value_unref(key);
    ewk_value_unref(v);
    eina_stringshare_del(key_str);
  }

  // try some invalid operations
  Ewk_Value v;
  Ewk_Value k = ewk_value_string_new("non-existent_key");
  EXPECT_EQ(ewk_value_dictionary_get(dict, k, &v), EINA_FALSE);
  ewk_value_unref(k);

  k = ewk_value_string_new(test_keys[INT_KEY]);

  EXPECT_EQ(ewk_value_dictionary_add(dict, k, test_values[INT_KEY], &b),
            EINA_FALSE);
  EXPECT_EQ(b, EINA_FALSE);

  EXPECT_EQ(ewk_value_dictionary_set(dict, k, test_values[INT_KEY], &b),
            EINA_TRUE);
  EXPECT_EQ(b, EINA_FALSE);
  ewk_value_unref(k);
}
