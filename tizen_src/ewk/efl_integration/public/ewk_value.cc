// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/ewk_value_product.h"
#include "private/ewk_value_private.h"

#include <memory>
#include <sstream>
#include <string>

namespace {

inline Eina_Bool Eina_FromBool(bool v) {
  return v ? EINA_TRUE : EINA_FALSE;
}

inline bool Eina_ToBool(Eina_Bool v) {
  return (v == EINA_FALSE) ? false : true;
}

inline const EwkValuePrivate* EwkValueCast(Ewk_Value value) {
  return static_cast<const EwkValuePrivate*>(value);
}

}  // namespace

#define EWK_VALUE_TYPE_CHECK_RETURN_VAL(value, type, retval) \
    EINA_SAFETY_ON_FALSE_RETURN_VAL((type) == ewk_value_type_get(value), \
                                    (retval))

Ewk_Value ewk_value_ref(Ewk_Value value) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(value, NULL);
  EwkValueCast(value)->AddRef();
  return value;
}

void ewk_value_unref(Ewk_Value value) {
  EINA_SAFETY_ON_NULL_RETURN(value);
  EwkValueCast(value)->Release();
}

Ewk_Value_Type ewk_value_null_type_get() {
  return static_cast<Ewk_Value_Type>(base::Value::Type::NONE);
}

Ewk_Value_Type ewk_value_type_get(Ewk_Value value) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(
      value, static_cast<Ewk_Value_Type>(base::Value::Type::NONE));
  return static_cast<Ewk_Value_Type>(EwkValueCast(value)->GetType());
}

Ewk_Value ewk_value_boolean_new(Eina_Bool initial_value) {
  Ewk_Value val =
      static_cast<Ewk_Value>(new EwkValuePrivate(Eina_ToBool(initial_value)));
  return ewk_value_ref(val);
}

Ewk_Value_Type ewk_value_boolean_type_get() {
  return static_cast<Ewk_Value_Type>(base::Value::Type::BOOLEAN);
}

Eina_Bool ewk_value_boolean_value_get(Ewk_Value value, Eina_Bool* dst) {
  EWK_VALUE_TYPE_CHECK_RETURN_VAL(value, ewk_value_boolean_type_get(),
                                  EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(dst, EINA_FALSE);
  bool val;
  bool result = EwkValueCast(value)->GetValue()->GetAsBoolean(&val);
  if (result) {
    *dst = Eina_FromBool(val);
    return EINA_TRUE;
  } else {
    return EINA_FALSE;
  }
}

Ewk_Value ewk_value_double_new(double initial_value) {
  Ewk_Value val = static_cast<Ewk_Value>(new EwkValuePrivate(initial_value));
  return ewk_value_ref(val);
}

Ewk_Value_Type ewk_value_double_type_get() {
  return static_cast<Ewk_Value_Type>(base::Value::Type::DOUBLE);
}

Eina_Bool ewk_value_double_value_get(Ewk_Value value, double* dst) {
  EWK_VALUE_TYPE_CHECK_RETURN_VAL(value, ewk_value_double_type_get(),
                                  EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(dst, EINA_FALSE);
  bool result = EwkValueCast(value)->GetValue()->GetAsDouble(dst);
  return Eina_FromBool(result);
}

Ewk_Value ewk_value_int_new(int initial_value) {
  Ewk_Value val = static_cast<Ewk_Value>(new EwkValuePrivate(initial_value));
  return ewk_value_ref(val);
}

Ewk_Value_Type ewk_value_int_type_get() {
  return static_cast<Ewk_Value_Type>(base::Value::Type::INTEGER);
}

Eina_Bool ewk_value_int_value_get(Ewk_Value value, int* dst) {
  EWK_VALUE_TYPE_CHECK_RETURN_VAL(value, ewk_value_int_type_get(), EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(dst, EINA_FALSE);
  bool result = EwkValueCast(value)->GetValue()->GetAsInteger(dst);
  return Eina_FromBool(result);
}

Ewk_Value ewk_value_string_new(const char* initial_value) {
  Ewk_Value val =
      static_cast<Ewk_Value>(new EwkValuePrivate(std::string(initial_value)));
  return ewk_value_ref(val);
}

Ewk_Value_Type ewk_value_string_type_get() {
  return static_cast<Ewk_Value_Type>(base::Value::Type::STRING);
}

Eina_Stringshare* ewk_value_string_value_get(Ewk_Value value) {
  EWK_VALUE_TYPE_CHECK_RETURN_VAL(value, ewk_value_string_type_get(), NULL);
  std::string str;
  bool result = EwkValueCast(value)->GetValue()->GetAsString(&str);
  if (result) {
    return eina_stringshare_add(str.c_str());
  } else {
    return NULL;
  }
}

Ewk_Value ewk_value_array_new() {
  Ewk_Value val = static_cast<Ewk_Value>(
      new EwkValuePrivate(std::unique_ptr<base::Value>(new base::ListValue())));
  return ewk_value_ref(val);
}

Ewk_Value_Type ewk_value_array_type_get() {
  return static_cast<Ewk_Value_Type>(base::Value::Type::LIST);
}

Eina_Bool ewk_value_array_is_mutable(Ewk_Value array) {
  EWK_VALUE_TYPE_CHECK_RETURN_VAL(array, ewk_value_array_type_get(),
                                  EINA_FALSE);
  return EINA_TRUE;
}

Eina_Bool ewk_value_array_append(Ewk_Value array, Ewk_Value value) {
  EINA_SAFETY_ON_FALSE_RETURN_VAL(ewk_value_array_is_mutable(array),
                                  EINA_FALSE);
  base::ListValue* list;
  bool result = EwkValueCast(array)->GetValue()->GetAsList(&list);
  if (result) {
    // TODO(m.majczak) consider a workaround for the deep copy
    list->Append(std::unique_ptr<base::Value>(EwkValueCast(value)->GetValue()->DeepCopy()));
    return EINA_TRUE;
  } else {
    return EINA_FALSE;
  }
}

size_t ewk_value_array_count(Ewk_Value array) {
  EWK_VALUE_TYPE_CHECK_RETURN_VAL(array, ewk_value_array_type_get(), 0);
  base::ListValue* list;
  bool result = EwkValueCast(array)->GetValue()->GetAsList(&list);
  if (result) {
    return list->GetSize();
  } else {
    return 0;
  }
}

Eina_Bool ewk_value_array_get(Ewk_Value array,
                              size_t position,
                              Ewk_Value* dst) {
  EINA_SAFETY_ON_FALSE_RETURN_VAL(position < ewk_value_array_count(array),
                                  EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(dst, EINA_FALSE);
  base::ListValue* list;
  bool result = EwkValueCast(array)->GetValue()->GetAsList(&list);
  if (result) {
    // TODO(m.majczak) consider a workaround for the deep copy
    base::Value* val = nullptr;
    list->Get(position, &val);
    if (!val)
      return EINA_FALSE;
    Ewk_Value ret = static_cast<Ewk_Value>(
        new EwkValuePrivate(std::unique_ptr<base::Value>(val->DeepCopy())));
    // warning!!! the ownership is passed to the caller here
    ewk_value_ref(ret);
    *dst = ret;
    return EINA_TRUE;
  } else {
    return EINA_FALSE;
  }
}

Eina_Bool ewk_value_array_remove(Ewk_Value array, size_t position) {
  EINA_SAFETY_ON_FALSE_RETURN_VAL(ewk_value_array_is_mutable(array),
                                  EINA_FALSE);
  EINA_SAFETY_ON_FALSE_RETURN_VAL(position < ewk_value_array_count(array),
                                  EINA_FALSE);
  base::ListValue* list;
  bool result = EwkValueCast(array)->GetValue()->GetAsList(&list);
  if (result) {
    // warning!!! value is completely deleted here
    return Eina_FromBool(list->Remove(position, NULL));
  } else {
    return EINA_FALSE;
  }
}

Ewk_Value ewk_value_dictionary_new() {
  Ewk_Value val = static_cast<Ewk_Value>(new EwkValuePrivate(
      std::unique_ptr<base::Value>(new base::DictionaryValue())));
  return ewk_value_ref(val);
}

Ewk_Value_Type ewk_value_dictionary_type_get() {
  return static_cast<Ewk_Value_Type>(base::Value::Type::DICTIONARY);
}

Eina_Bool ewk_value_dictionary_is_mutable(Ewk_Value dictionary) {
  EWK_VALUE_TYPE_CHECK_RETURN_VAL(dictionary, ewk_value_dictionary_type_get(),
                                  EINA_FALSE);
  return EINA_TRUE;
}

Eina_Bool ewk_value_dictionary_keys(Ewk_Value dictionary, Ewk_Value* keys) {
  EWK_VALUE_TYPE_CHECK_RETURN_VAL(dictionary, ewk_value_dictionary_type_get(),
                                  EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(keys, EINA_FALSE);
  // warning!!! This is very expensive operation.
  base::DictionaryValue* dict;
  bool result = EwkValueCast(dictionary)->GetValue()->GetAsDictionary(&dict);
  if (result) {
    *keys = ewk_value_array_new();
    base::DictionaryValue::Iterator it(*dict);
    while (!it.IsAtEnd()) {
      Ewk_Value str = ewk_value_string_new(it.key().c_str());
      ewk_value_array_append(*keys, str);
      ewk_value_unref(str);
      it.Advance();
    }
    return EINA_TRUE;
  } else {
    return EINA_FALSE;
  }
}

Eina_Bool ewk_value_dictionary_set(Ewk_Value dictionary,
                                   Ewk_Value key,
                                   Ewk_Value value,
                                   Eina_Bool* new_entry) {
  EINA_SAFETY_ON_FALSE_RETURN_VAL(ewk_value_dictionary_is_mutable(dictionary),
                                  EINA_FALSE);
  EWK_VALUE_TYPE_CHECK_RETURN_VAL(key, ewk_value_string_type_get(), EINA_FALSE);
  base::DictionaryValue* dict;
  std::string k;
  bool result1 = EwkValueCast(dictionary)->GetValue()->GetAsDictionary(&dict);
  bool result2 = EwkValueCast(key)->GetValue()->GetAsString(&k);
  if (result1 && result2) {
    // TODO(m.majczak) consider a workaround for the deep copy
    *new_entry = Eina_FromBool(!dict->HasKey(k));
    dict->Set(k, EwkValueCast(value)->GetValue()->CreateDeepCopy());
    return EINA_TRUE;
  } else {
    return EINA_FALSE;
  }
}

Eina_Bool ewk_value_dictionary_add(Ewk_Value dictionary,
                                   Ewk_Value key,
                                   Ewk_Value value,
                                   Eina_Bool* new_entry) {
  EINA_SAFETY_ON_FALSE_RETURN_VAL(ewk_value_dictionary_is_mutable(dictionary),
                                  EINA_FALSE);
  EWK_VALUE_TYPE_CHECK_RETURN_VAL(key, ewk_value_string_type_get(), EINA_FALSE);
  base::DictionaryValue* dict;
  std::string k;
  bool result1 = EwkValueCast(dictionary)->GetValue()->GetAsDictionary(&dict);
  bool result2 = EwkValueCast(key)->GetValue()->GetAsString(&k);
  if (result1 && result2) {
    if (!dict->HasKey(k)) {
      *new_entry = EINA_TRUE;
      // TODO(m.majczak) consider a workaround for the deep copy
      dict->Set(k, EwkValueCast(value)->GetValue()->CreateDeepCopy());
      return EINA_TRUE;
    }
    *new_entry = EINA_FALSE;
    return EINA_FALSE;
  } else {
    return EINA_FALSE;
  }
}

Eina_Bool ewk_value_dictionary_get(Ewk_Value dictionary,
                                   Ewk_Value key,
                                   Ewk_Value* dst) {
  EWK_VALUE_TYPE_CHECK_RETURN_VAL(dictionary, ewk_value_dictionary_type_get(),
                                  EINA_FALSE);
  EWK_VALUE_TYPE_CHECK_RETURN_VAL(key, ewk_value_string_type_get(), EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(dst, EINA_FALSE);
  base::DictionaryValue* dict;
  std::string k;
  bool result1 = EwkValueCast(dictionary)->GetValue()->GetAsDictionary(&dict);
  bool result2 = EwkValueCast(key)->GetValue()->GetAsString(&k);
  if (result1 && result2) {
    base::Value* val = nullptr;
    dict->Get(k, &val);
    if (!val)
      return EINA_FALSE;
    // TODO(m.majczak) consider a workaround for the deep copy
    Ewk_Value ret = static_cast<Ewk_Value>(
        new EwkValuePrivate(std::unique_ptr<base::Value>(val->DeepCopy())));
    // warning!!! the ownership is passed to the caller here
    ewk_value_ref(ret);
    *dst = ret;
    return EINA_TRUE;
  } else {
    return EINA_FALSE;
  }
}

Eina_Bool ewk_value_dictionary_remove(Ewk_Value dictionary, Ewk_Value key) {
  EINA_SAFETY_ON_FALSE_RETURN_VAL(ewk_value_dictionary_is_mutable(dictionary),
                                  EINA_FALSE);
  EWK_VALUE_TYPE_CHECK_RETURN_VAL(key, ewk_value_string_type_get(), EINA_FALSE);
  base::DictionaryValue* dict;
  std::string k;
  bool result1 = EwkValueCast(dictionary)->GetValue()->GetAsDictionary(&dict);
  bool result2 = EwkValueCast(key)->GetValue()->GetAsString(&k);
  if (result1 && result2) {
    // warning!!! value is completely deleted here
    return Eina_FromBool(dict->Remove(k, NULL));
  } else {
    return EINA_FALSE;
  }
}
