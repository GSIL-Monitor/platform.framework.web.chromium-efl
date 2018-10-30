// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/var_value_converter.h"

#include <limits>
#include <random>
#include <sstream>
#include <string>

#include "base/message_loop/message_loop.h"
#include "ppapi/shared_impl/array_var.h"
#include "ppapi/shared_impl/dictionary_var.h"
#include "ppapi/shared_impl/test_globals.h"
#include "ppapi/shared_impl/var.h"
#include "testing/gtest/include/gtest/gtest.h"

template <typename T, size_t N>
size_t ArraySize(T (&)[N]) {
  return N;
}

namespace {

std::string RandomStr() {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<> len_gen(1, 129);
  static std::uniform_int_distribution<> char_gen(32, 125);

  int length = len_gen(gen);
  std::stringstream ss;
  for (int i = 0; i < length; ++i) {
    // generate random printable character
    ss << static_cast<char>(char_gen(gen));
  }
  return ss.str();
}

std::string test_keys[] = {"bool_key",   "int_key",   "double_key",
                           "string_key", "array_key", "dictionary_key"};

enum TestKeysTypes {
  BOOL_KEY,
  INT_KEY,
  DOUBLE_KEY,
  STRING_KEY,
  ARRAY_KEY,
  DICT_KEY,
  KEY_COUNT
};

enum ArrayValueTypes {
  BOOL_IDX,
  INT_IDX,
  DOUBLE_IDX,
  STRING_IDX,
  ARRAY_IDX,
  IDX_COUNT
};

// This is required to assert size of nested containers in array test.
// If adding more POD types, please add them just after the last POD type.
const size_t kArrayPODIdxCount = STRING_IDX;
// This value defines size of the tested array in ArrayVarValueConversionTest.
const size_t kArrayTestSize = IDX_COUNT;

// This is required to assert size of nested containers in dictionary test.
// If adding more POD types, please add them just after the last POD type.
const size_t kPODKeysCount = STRING_KEY;

void SetAndForget(scoped_refptr<ppapi::ArrayVar> arr, int idx, PP_Var var) {
  arr->Set(idx, var);
  ppapi::PpapiGlobals::Get()->GetVarTracker()->ReleaseVar(var);
}

void SetAndForget(scoped_refptr<ppapi::DictionaryVar> dict,
                  const ppapi::ScopedPPVar& key,
                  PP_Var var) {
  dict->Set(key.get(), var);
  ppapi::PpapiGlobals::Get()->GetVarTracker()->ReleaseVar(var);
}

}  // namespace

namespace ppapi {
namespace proxy {

class VarValueConverterTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // check if initial var count is 0
    EXPECT_EQ(PpapiGlobals::Get()->GetVarTracker()->GetLiveVars().size(), 0u);
  }
  void TearDown() override {
    // check if everything is freed properely
    EXPECT_EQ(PpapiGlobals::Get()->GetVarTracker()->GetLiveVars().size(), 0u);
  }

 private:
  base::MessageLoop message_loop_;  // required to instantiate globals
  TestGlobals globals_;             // required for var management
};

TEST_F(VarValueConverterTest, BoolVarValueConversionTest) {
  bool b;
  // to value
  ScopedPPVar var1(ScopedPPVar::PassRef(), PP_MakeBool(PP_TRUE));
  std::unique_ptr<base::Value> value1 = ValueFromVar(var1.get());
  EXPECT_EQ(value1->GetType(), base::Value::Type::BOOLEAN);
  ASSERT_TRUE(value1->GetAsBoolean(&b));
  EXPECT_EQ(b, PP_ToBool(var1.get().value.as_bool));

  ScopedPPVar var2(ScopedPPVar::PassRef(), PP_MakeBool(PP_FALSE));
  std::unique_ptr<base::Value> value2 = ValueFromVar(var2.get());
  EXPECT_EQ(value1->GetType(), base::Value::Type::BOOLEAN);
  ASSERT_TRUE(value2->GetAsBoolean(&b));
  EXPECT_EQ(b, PP_ToBool(var2.get().value.as_bool));

  // to var
  ScopedPPVar result1 = VarFromValue(value1.get());
  ASSERT_TRUE(value1->GetAsBoolean(&b));
  EXPECT_EQ(b, PP_ToBool(result1.get().value.as_bool));

  ScopedPPVar result2 = VarFromValue(value2.get());
  ASSERT_TRUE(value2->GetAsBoolean(&b));
  EXPECT_EQ(b, PP_ToBool(result2.get().value.as_bool));
}

TEST_F(VarValueConverterTest, IntVarValueConversionTest) {
  for (int k = -102; k < 100; ++k) {
    int i;
    ScopedPPVar var;
    switch (k) {
      case -102:
        var = ScopedPPVar(ScopedPPVar::PassRef(),
                          PP_MakeInt32(std::numeric_limits<int>::min()));
        break;
      case -101:
        var = ScopedPPVar(ScopedPPVar::PassRef(),
                          PP_MakeInt32(std::numeric_limits<int>::max()));
        break;
      default:
        var = ScopedPPVar(ScopedPPVar::PassRef(), PP_MakeInt32(k));
        break;
    }

    // to value
    std::unique_ptr<base::Value> value = ValueFromVar(var.get());
    EXPECT_EQ(value->GetType(), base::Value::Type::INTEGER);
    ASSERT_TRUE(value->GetAsInteger(&i));
    EXPECT_EQ(i, var.get().value.as_int);

    // to var
    ScopedPPVar result = VarFromValue(value.get());
    EXPECT_EQ(i, result.get().value.as_int);
  }
}

TEST_F(VarValueConverterTest, DoubleVarValueConversionTest) {
  for (int k = -104; k < 100; ++k) {
    // prepare test data
    double d;
    ScopedPPVar var;
    // Warning!! NaN, Inf and -Inf cannot be tested as base::Value doesn't
    // support them, they are represented as 0 upon conversion to base::Value.
    // This is due to JSON serialization requirements.
    switch (k) {
      case -103:
        var = ScopedPPVar(ScopedPPVar::PassRef(),
                          PP_MakeDouble(std::numeric_limits<double>::min()));
        break;
      case -102:
        var = ScopedPPVar(ScopedPPVar::PassRef(),
                          PP_MakeDouble(-std::numeric_limits<double>::max()));
        break;
      case -101:
        var = ScopedPPVar(ScopedPPVar::PassRef(),
                          PP_MakeDouble(std::numeric_limits<double>::max()));
        break;
      default:
        var = ScopedPPVar(ScopedPPVar::PassRef(),
                          PP_MakeDouble(static_cast<double>(k) * M_PI));
        break;
    }

    // to value
    std::unique_ptr<base::Value> value = ValueFromVar(var.get());
    EXPECT_EQ(value->GetType(), base::Value::Type::DOUBLE);
    ASSERT_TRUE(value->GetAsDouble(&d));
    EXPECT_EQ(d, var.get().value.as_double);

    // to var
    ScopedPPVar result = VarFromValue(value.get());
    EXPECT_EQ(d, result.get().value.as_double);
  }
}

TEST_F(VarValueConverterTest, StringVarValueConversionTest) {
  for (int k = 0; k < 100; ++k) {
    // prepare test data
    std::string s;
    std::string str = RandomStr();
    ScopedPPVar var(ScopedPPVar::PassRef(), StringVar::StringToPPVar(str));

    // to value
    std::unique_ptr<base::Value> value = ValueFromVar(var.get());
    ASSERT_TRUE(value != NULL);
    EXPECT_EQ(value->GetType(), base::Value::Type::STRING);
    ASSERT_TRUE(value->GetAsString(&s));
    EXPECT_STREQ(str.c_str(), s.c_str());

    // to var
    ScopedPPVar result = VarFromValue(value.get());
    scoped_refptr<StringVar> str_var =
        scoped_refptr<StringVar>(StringVar::FromPPVar(result.get()));
    ASSERT_TRUE(str_var != NULL);
    EXPECT_STREQ(str_var->value().c_str(), s.c_str());
  }
}

TEST_F(VarValueConverterTest, ArrayVarValueConversionTest) {
  for (int k = 0; k < 100; ++k) {
    // prepare test data
    bool b = ((k % 2) == 0);
    int i = k;
    double d = k;
    std::string str = RandomStr();
    bool b2;
    int i2;
    double d2;
    std::string str2;

    scoped_refptr<ArrayVar> nested_array =
        scoped_refptr<ArrayVar>(new ArrayVar());
    nested_array->SetLength(kArrayPODIdxCount);
    size_t current_idx = BOOL_IDX;
    SetAndForget(nested_array, current_idx++, PP_MakeBool(PP_FromBool(b)));
    SetAndForget(nested_array, current_idx++, PP_MakeInt32(i));
    SetAndForget(nested_array, current_idx++, PP_MakeDouble(d));
    ASSERT_EQ(current_idx, kArrayPODIdxCount);

    scoped_refptr<ArrayVar> array = scoped_refptr<ArrayVar>(new ArrayVar());
    array->SetLength(kArrayTestSize);
    current_idx = BOOL_IDX;
    SetAndForget(array, current_idx++, PP_MakeBool(PP_FromBool(b)));
    SetAndForget(array, current_idx++, PP_MakeInt32(i));
    SetAndForget(array, current_idx++, PP_MakeDouble(d));
    SetAndForget(array, current_idx++, StringVar::StringToPPVar(str));
    SetAndForget(array, current_idx++, nested_array->GetPPVar());
    ASSERT_EQ(current_idx, kArrayTestSize);

    // convert to value
    ScopedPPVar var(ScopedPPVar::PassRef(), array->GetPPVar());
    std::unique_ptr<base::Value> value = ValueFromVar(var.get());

    ASSERT_TRUE(value != NULL);
    EXPECT_EQ(value->GetType(), base::Value::Type::LIST);
    base::ListValue* list = nullptr;
    ASSERT_TRUE(value->GetAsList(&list));
    ASSERT_TRUE(list != NULL);
    EXPECT_EQ(list->GetSize(), kArrayTestSize);

    ASSERT_TRUE(list->GetBoolean(BOOL_IDX, &b2));
    EXPECT_EQ(b, b2);
    ASSERT_TRUE(list->GetInteger(INT_IDX, &i2));
    EXPECT_EQ(i, i2);
    ASSERT_TRUE(list->GetDouble(DOUBLE_IDX, &d2));
    EXPECT_EQ(d, d2);
    ASSERT_TRUE(list->GetString(STRING_IDX, &str2));
    EXPECT_STREQ(str.c_str(), str2.c_str());

    base::ListValue* nested_list = nullptr;
    ASSERT_TRUE(list->GetList(ARRAY_IDX, &nested_list));
    ASSERT_TRUE(nested_list != NULL);
    EXPECT_EQ(nested_list->GetSize(), kArrayPODIdxCount);

    ASSERT_TRUE(nested_list->GetBoolean(BOOL_IDX, &b2));
    EXPECT_EQ(b, b2);
    ASSERT_TRUE(nested_list->GetInteger(INT_IDX, &i2));
    EXPECT_EQ(i, i2);
    ASSERT_TRUE(nested_list->GetDouble(DOUBLE_IDX, &d2));
    EXPECT_EQ(d, d2);

    // convert back to var
    var = VarFromValue(value.get());
    ASSERT_EQ(var.get().type, PP_VARTYPE_ARRAY);
    scoped_refptr<ArrayVar> result =
        scoped_refptr<ArrayVar>(ArrayVar::FromPPVar(var.get()));

    ASSERT_TRUE(result != NULL);
    EXPECT_EQ(result->GetLength(), kArrayTestSize);

    var = ScopedPPVar(ScopedPPVar::PassRef(), result->Get(BOOL_IDX));
    EXPECT_EQ(var.get().type, PP_VARTYPE_BOOL);
    EXPECT_EQ(var.get().value.as_bool, PP_FromBool(b));
    var = ScopedPPVar(ScopedPPVar::PassRef(), result->Get(INT_IDX));
    EXPECT_EQ(var.get().type, PP_VARTYPE_INT32);
    EXPECT_EQ(var.get().value.as_int, i);
    var = ScopedPPVar(ScopedPPVar::PassRef(), result->Get(DOUBLE_IDX));
    EXPECT_EQ(var.get().type, PP_VARTYPE_DOUBLE);
    EXPECT_EQ(var.get().value.as_double, d);
    var = ScopedPPVar(ScopedPPVar::PassRef(), result->Get(STRING_IDX));
    EXPECT_EQ(var.get().type, PP_VARTYPE_STRING);
    scoped_refptr<StringVar> str_var =
        scoped_refptr<StringVar>(StringVar::FromPPVar(var.get()));
    ASSERT_TRUE(str_var != NULL);
    EXPECT_STREQ(str.c_str(), str_var->value().c_str());

    var = ScopedPPVar(ScopedPPVar::PassRef(), result->Get(ARRAY_IDX));
    ASSERT_EQ(var.get().type, PP_VARTYPE_ARRAY);
    scoped_refptr<ArrayVar> nested_var =
        scoped_refptr<ArrayVar>(ArrayVar::FromPPVar(var.get()));
    ASSERT_TRUE(nested_var != NULL);
    EXPECT_EQ(nested_var->GetLength(), kArrayPODIdxCount);
    var = ScopedPPVar(ScopedPPVar::PassRef(), nested_var->Get(BOOL_IDX));
    EXPECT_EQ(var.get().type, PP_VARTYPE_BOOL);
    EXPECT_EQ(var.get().value.as_bool, PP_FromBool(b));
    var = ScopedPPVar(ScopedPPVar::PassRef(), nested_var->Get(INT_IDX));
    EXPECT_EQ(var.get().type, PP_VARTYPE_INT32);
    EXPECT_EQ(var.get().value.as_int, i);
    var = ScopedPPVar(ScopedPPVar::PassRef(), nested_var->Get(DOUBLE_IDX));
    EXPECT_EQ(var.get().type, PP_VARTYPE_DOUBLE);
    EXPECT_EQ(var.get().value.as_double, d);
  }
}

TEST_F(VarValueConverterTest, DictionaryVarValueConversionTest) {
  ScopedPPVar key_vars[] = {
      ScopedPPVar(ScopedPPVar::PassRef(),
                  StringVar::StringToPPVar(test_keys[BOOL_KEY])),
      ScopedPPVar(ScopedPPVar::PassRef(),
                  StringVar::StringToPPVar(test_keys[INT_KEY])),
      ScopedPPVar(ScopedPPVar::PassRef(),
                  StringVar::StringToPPVar(test_keys[DOUBLE_KEY])),
      ScopedPPVar(ScopedPPVar::PassRef(),
                  StringVar::StringToPPVar(test_keys[STRING_KEY])),
      ScopedPPVar(ScopedPPVar::PassRef(),
                  StringVar::StringToPPVar(test_keys[ARRAY_KEY])),
      ScopedPPVar(ScopedPPVar::PassRef(),
                  StringVar::StringToPPVar(test_keys[DICT_KEY]))};

  for (int k = 0; k < 100; ++k) {
    // prepare test data

    bool b = ((k % 2) == 0);
    int i = k;
    double d = k;
    std::string str = RandomStr();
    bool b2;
    int i2;
    double d2;
    std::string str2;

    scoped_refptr<ArrayVar> nested_array =
        scoped_refptr<ArrayVar>(new ArrayVar());
    nested_array->SetLength(kArrayPODIdxCount);
    size_t current_idx = BOOL_IDX;
    SetAndForget(nested_array, current_idx++, PP_MakeBool(PP_FromBool(b)));
    SetAndForget(nested_array, current_idx++, PP_MakeInt32(i));
    SetAndForget(nested_array, current_idx++, PP_MakeDouble(d));
    ASSERT_EQ(current_idx, kArrayPODIdxCount);

    scoped_refptr<DictionaryVar> nested_dict =
        scoped_refptr<DictionaryVar>(new DictionaryVar());
    SetAndForget(nested_dict, key_vars[BOOL_KEY], PP_MakeBool(PP_FromBool(b)));
    SetAndForget(nested_dict, key_vars[INT_KEY], PP_MakeInt32(i));
    SetAndForget(nested_dict, key_vars[DOUBLE_KEY], PP_MakeDouble(d));

    scoped_refptr<DictionaryVar> dict =
        scoped_refptr<DictionaryVar>(new DictionaryVar());
    SetAndForget(dict, key_vars[BOOL_KEY], PP_MakeBool(PP_FromBool(b)));
    SetAndForget(dict, key_vars[INT_KEY], PP_MakeInt32(i));
    SetAndForget(dict, key_vars[DOUBLE_KEY], PP_MakeDouble(d));
    SetAndForget(dict, key_vars[STRING_KEY], StringVar::StringToPPVar(str));
    SetAndForget(dict, key_vars[ARRAY_KEY], nested_array->GetPPVar());
    SetAndForget(dict, key_vars[DICT_KEY], nested_dict->GetPPVar());

    // convert to value
    ScopedPPVar var = ScopedPPVar(ScopedPPVar::PassRef(), dict->GetPPVar());
    std::unique_ptr<base::Value> value = ValueFromVar(var.get());
    ASSERT_TRUE(value != NULL);
    EXPECT_EQ(value->GetType(), base::Value::Type::DICTIONARY);
    base::DictionaryValue* dict_val = nullptr;
    ASSERT_TRUE(value->GetAsDictionary(&dict_val));
    ASSERT_TRUE(dict_val != NULL);
    EXPECT_EQ(dict_val->size(), ArraySize(key_vars));

    ASSERT_TRUE(dict_val->GetBoolean(test_keys[BOOL_KEY], &b2));
    EXPECT_EQ(b, b2);
    ASSERT_TRUE(dict_val->GetInteger(test_keys[INT_KEY], &i2));
    EXPECT_EQ(i, i2);
    ASSERT_TRUE(dict_val->GetDouble(test_keys[DOUBLE_KEY], &d2));
    EXPECT_EQ(d, d2);
    ASSERT_TRUE(dict_val->GetString(test_keys[STRING_KEY], &str2));
    EXPECT_STREQ(str.c_str(), str2.c_str());

    base::ListValue* nested_list = nullptr;
    ASSERT_TRUE(dict_val->GetList(test_keys[ARRAY_KEY], &nested_list));
    ASSERT_TRUE(nested_list != NULL);
    EXPECT_EQ(nested_list->GetSize(), kPODKeysCount);

    ASSERT_TRUE(nested_list->GetBoolean(BOOL_IDX, &b2));
    EXPECT_EQ(b, b2);
    ASSERT_TRUE(nested_list->GetInteger(INT_IDX, &i2));
    EXPECT_EQ(i, i2);
    ASSERT_TRUE(nested_list->GetDouble(DOUBLE_IDX, &d2));
    EXPECT_EQ(d, d2);

    base::DictionaryValue* nested_dict_val = nullptr;
    ASSERT_TRUE(dict_val->GetDictionary(test_keys[DICT_KEY], &nested_dict_val));
    ASSERT_TRUE(nested_dict_val != NULL);
    EXPECT_EQ(nested_dict_val->size(), kPODKeysCount);

    ASSERT_TRUE(nested_dict_val->GetBoolean(test_keys[BOOL_KEY], &b2));
    EXPECT_EQ(b, b2);
    ASSERT_TRUE(nested_dict_val->GetInteger(test_keys[INT_KEY], &i2));
    EXPECT_EQ(i, i2);
    ASSERT_TRUE(nested_dict_val->GetDouble(test_keys[DOUBLE_KEY], &d2));
    EXPECT_EQ(d, d2);

    // convert back to var
    var = VarFromValue(value.get());
    ASSERT_EQ(var.get().type, PP_VARTYPE_DICTIONARY);
    scoped_refptr<DictionaryVar> result =
        scoped_refptr<DictionaryVar>(DictionaryVar::FromPPVar(var.get()));
    ASSERT_TRUE(result != NULL);

    var = ScopedPPVar(ScopedPPVar::PassRef(),
                      result->Get(key_vars[BOOL_KEY].get()));
    EXPECT_EQ(var.get().type, PP_VARTYPE_BOOL);
    EXPECT_EQ(var.get().value.as_bool, PP_FromBool(b));
    var = ScopedPPVar(ScopedPPVar::PassRef(),
                      result->Get(key_vars[INT_KEY].get()));
    EXPECT_EQ(var.get().type, PP_VARTYPE_INT32);
    EXPECT_EQ(var.get().value.as_int, i);
    var = ScopedPPVar(ScopedPPVar::PassRef(),
                      result->Get(key_vars[DOUBLE_KEY].get()));
    EXPECT_EQ(var.get().type, PP_VARTYPE_DOUBLE);
    EXPECT_EQ(var.get().value.as_double, d);
    var = ScopedPPVar(ScopedPPVar::PassRef(),
                      result->Get(key_vars[STRING_KEY].get()));
    EXPECT_EQ(var.get().type, PP_VARTYPE_STRING);
    scoped_refptr<StringVar> str_var =
        scoped_refptr<StringVar>(StringVar::FromPPVar(var.get()));
    ASSERT_TRUE(str_var != NULL);
    EXPECT_STREQ(str.c_str(), str_var->value().c_str());

    var = ScopedPPVar(ScopedPPVar::PassRef(),
                      result->Get(key_vars[ARRAY_KEY].get()));
    EXPECT_EQ(var.get().type, PP_VARTYPE_ARRAY);
    scoped_refptr<ArrayVar> nested_var =
        scoped_refptr<ArrayVar>(ArrayVar::FromPPVar(var.get()));
    ASSERT_TRUE(nested_var != NULL);
    EXPECT_EQ(nested_var->GetLength(), kPODKeysCount);
    var = ScopedPPVar(ScopedPPVar::PassRef(), nested_var->Get(BOOL_IDX));
    EXPECT_EQ(var.get().type, PP_VARTYPE_BOOL);
    EXPECT_EQ(var.get().value.as_bool, PP_FromBool(b));
    var = ScopedPPVar(ScopedPPVar::PassRef(), nested_var->Get(INT_IDX));
    EXPECT_EQ(var.get().type, PP_VARTYPE_INT32);
    EXPECT_EQ(var.get().value.as_int, i);
    var = ScopedPPVar(ScopedPPVar::PassRef(), nested_var->Get(DOUBLE_IDX));
    EXPECT_EQ(var.get().type, PP_VARTYPE_DOUBLE);
    EXPECT_EQ(var.get().value.as_double, d);

    var = ScopedPPVar(ScopedPPVar::PassRef(),
                      result->Get(key_vars[DICT_KEY].get()));
    EXPECT_EQ(var.get().type, PP_VARTYPE_DICTIONARY);
    scoped_refptr<DictionaryVar> nested_var2 =
        scoped_refptr<DictionaryVar>(DictionaryVar::FromPPVar(var.get()));
    ASSERT_TRUE(nested_var2 != NULL);
    var = ScopedPPVar(ScopedPPVar::PassRef(),
                      nested_var2->Get(key_vars[BOOL_KEY].get()));
    EXPECT_EQ(var.get().type, PP_VARTYPE_BOOL);
    EXPECT_EQ(var.get().value.as_bool, PP_FromBool(b));
    var = ScopedPPVar(ScopedPPVar::PassRef(),
                      nested_var2->Get(key_vars[INT_KEY].get()));
    EXPECT_EQ(var.get().type, PP_VARTYPE_INT32);
    EXPECT_EQ(var.get().value.as_int, i);
    var = ScopedPPVar(ScopedPPVar::PassRef(),
                      nested_var2->Get(key_vars[DOUBLE_KEY].get()));
    EXPECT_EQ(var.get().type, PP_VARTYPE_DOUBLE);
    EXPECT_EQ(var.get().value.as_double, d);
  }
}

}  // namespace proxy
}  // namespace ppapi
