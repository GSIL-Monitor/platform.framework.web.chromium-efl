// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstring>

#include "ewk_value_product.h"
#include "utc_blink_ewk_base.h"

template <typename T, size_t N>
size_t ArraySize(T (&)[N]) {
  return N;
}

namespace {

const char* test_strings[] = {
    "",                                   // empty
    "a",                                  // one char
    "    \n\n",                           // white space string
    "testtesttest",                       // continous text string
    "1 2 3 4 5 6 7 8 9 0",                // digits
    "test test test 1 2 3 4 5 6 7 8 9 0"  // mixed
};

const char* test_keys[] = {"bool_key",   "double_key", "int_key",
                           "string_key", "array_key",  "dictionary_key"};

enum TestKeysTypes {
  BOOL_KEY,
  DOUBLE_KEY,
  INT_KEY,
  STRING_KEY,
  ARRAY_KEY,
  DICT_KEY,
  KEY_COUNT
};

bool CompareString(Ewk_Value v1, Ewk_Value v2);
bool CompareArray(Ewk_Value v1, Ewk_Value v2);
bool CompareDictionary(Ewk_Value v1, Ewk_Value v2);

bool Compare(Ewk_Value v1, Ewk_Value v2) {
  if (ewk_value_type_get(v1) != ewk_value_type_get(v2))
    return false;

  if (ewk_value_type_get(v1) == ewk_value_boolean_type_get()) {
    Eina_Bool b1, b2;
    ewk_value_boolean_value_get(v1, &b1);
    ewk_value_boolean_value_get(v2, &b2);
    return b1 == b2;
  } else if (ewk_value_type_get(v1) == ewk_value_double_type_get()) {
    double d1, d2;
    ewk_value_double_value_get(v1, &d1);
    ewk_value_double_value_get(v2, &d2);
    return d1 == d2;
  } else if (ewk_value_type_get(v1) == ewk_value_int_type_get()) {
    int i1, i2;
    ewk_value_int_value_get(v1, &i1);
    ewk_value_int_value_get(v2, &i2);
    return i1 == i2;
  } else if (ewk_value_type_get(v1) == ewk_value_string_type_get()) {
    return CompareString(v1, v2);
  } else if (ewk_value_type_get(v1) == ewk_value_array_type_get()) {
    return CompareArray(v1, v2);
  } else if (ewk_value_type_get(v1) == ewk_value_dictionary_type_get()) {
    return CompareDictionary(v1, v2);
  } else {
    return false;
  }
}

bool CompareString(Ewk_Value v1, Ewk_Value v2) {
  if (ewk_value_type_get(v1) != ewk_value_string_type_get() ||
      ewk_value_type_get(v2) != ewk_value_string_type_get())
    return false;

  Eina_Stringshare* s1 = ewk_value_string_value_get(v1);
  Eina_Stringshare* s2 = ewk_value_string_value_get(v2);
  bool cmp =
      strcmp(static_cast<const char*>(s1), static_cast<const char*>(s2)) == 0;
  eina_stringshare_del(s1);
  eina_stringshare_del(s2);
  return cmp;
}

bool CompareArray(Ewk_Value v1, Ewk_Value v2) {
  if (ewk_value_type_get(v1) != ewk_value_array_type_get() ||
      ewk_value_type_get(v2) != ewk_value_array_type_get() ||
      ewk_value_array_is_mutable(v1) == EINA_FALSE ||
      ewk_value_array_is_mutable(v2) == EINA_FALSE ||
      ewk_value_array_count(v1) != ewk_value_array_count(v2))
    return false;

  size_t count = ewk_value_array_count(v1);
  for (size_t i = 0; i < count; ++i) {
    Ewk_Value a1, a2;
    if (ewk_value_array_get(v1, i, &a1) == EINA_FALSE)
      return false;
    if (ewk_value_array_get(v2, i, &a2) == EINA_FALSE) {
      ewk_value_unref(a1);
      return false;
    }
    bool cmp = Compare(a1, a2);
    ewk_value_unref(a1);
    ewk_value_unref(a2);
    if (!cmp)
      return false;
  }
  return true;
}

bool CompareDictionary(Ewk_Value v1, Ewk_Value v2) {
  if (ewk_value_type_get(v1) != ewk_value_dictionary_type_get() ||
      ewk_value_type_get(v2) != ewk_value_dictionary_type_get() ||
      ewk_value_dictionary_is_mutable(v1) == EINA_FALSE ||
      ewk_value_dictionary_is_mutable(v2) == EINA_FALSE)
    return false;

  Ewk_Value k;
  if (ewk_value_dictionary_keys(v1, &k) == EINA_FALSE)
    return false;

  size_t count = ewk_value_array_count(k);
  for (size_t i = 0; i < count; ++i) {
    Ewk_Value key;
    if (ewk_value_array_get(k, i, &key) == EINA_FALSE)
      return false;

    Ewk_Value a1, a2;
    if (ewk_value_dictionary_get(v1, key, &a1) == EINA_FALSE) {
      ewk_value_unref(k);
      return false;
    }
    if (ewk_value_dictionary_get(v2, key, &a2) == EINA_FALSE) {
      ewk_value_unref(k);
      ewk_value_unref(a1);
      return false;
    }

    bool cmp = Compare(a1, a2);
    ewk_value_unref(a1);
    ewk_value_unref(a2);
    if (!cmp) {
      ewk_value_unref(k);
      return false;
    }
  }
  ewk_value_unref(k);
  return true;
}

}  // namespace
