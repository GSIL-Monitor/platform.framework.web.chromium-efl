// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/json/JSONParser.h"

#include "platform/json/JSONValues.h"
#include "platform/wtf/text/StringBuilder.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

TEST(JSONParserTest, Reading) {
  JSONValue* tmp_value;
  std::unique_ptr<JSONValue> root;
  std::unique_ptr<JSONValue> root2;
  String str_val;
  int int_val = 0;

  // some whitespace checking
  root = ParseJSON("    null    ");
  ASSERT_TRUE(root.get());
  EXPECT_EQ(JSONValue::kTypeNull, root->GetType());

  // Invalid JSON string
  root = ParseJSON("nu");
  EXPECT_FALSE(root.get());

  // Simple bool
  root = ParseJSON("true  ");
  ASSERT_TRUE(root.get());
  EXPECT_EQ(JSONValue::kTypeBoolean, root->GetType());

  // Embedded comment
  root = ParseJSON("40 /*/");
  EXPECT_FALSE(root.get());
  root = ParseJSON("/* comment */null");
  ASSERT_TRUE(root.get());
  EXPECT_EQ(JSONValue::kTypeNull, root->GetType());
  root = ParseJSON("40 /* comment */");
  ASSERT_TRUE(root.get());
  EXPECT_EQ(JSONValue::kTypeInteger, root->GetType());
  EXPECT_TRUE(root->AsInteger(&int_val));
  EXPECT_EQ(40, int_val);
  root = ParseJSON("/**/ 40 /* multi-line\n comment */ // more comment");
  ASSERT_TRUE(root.get());
  EXPECT_EQ(JSONValue::kTypeInteger, root->GetType());
  EXPECT_TRUE(root->AsInteger(&int_val));
  EXPECT_EQ(40, int_val);
  root = ParseJSON("true // comment");
  ASSERT_TRUE(root.get());
  EXPECT_EQ(JSONValue::kTypeBoolean, root->GetType());
  root = ParseJSON("/* comment */\"sample string\"");
  ASSERT_TRUE(root.get());
  EXPECT_TRUE(root->AsString(&str_val));
  EXPECT_EQ("sample string", str_val);
  root = ParseJSON("[1, /* comment, 2 ] */ \n 3]");
  ASSERT_TRUE(root.get());
  JSONArray* list = JSONArray::Cast(root.get());
  ASSERT_TRUE(list);
  EXPECT_EQ(2u, list->size());
  tmp_value = list->at(0);
  ASSERT_TRUE(tmp_value);
  EXPECT_TRUE(tmp_value->AsInteger(&int_val));
  EXPECT_EQ(1, int_val);
  tmp_value = list->at(1);
  ASSERT_TRUE(tmp_value);
  EXPECT_TRUE(tmp_value->AsInteger(&int_val));
  EXPECT_EQ(3, int_val);
  root = ParseJSON("[1, /*a*/2, 3]");
  ASSERT_TRUE(root.get());
  list = JSONArray::Cast(root.get());
  ASSERT_TRUE(list);
  EXPECT_EQ(3u, list->size());
  root = ParseJSON("/* comment **/42");
  ASSERT_TRUE(root.get());
  EXPECT_EQ(JSONValue::kTypeInteger, root->GetType());
  EXPECT_TRUE(root->AsInteger(&int_val));
  EXPECT_EQ(42, int_val);
  root = ParseJSON(
      "/* comment **/\n"
      "// */ 43\n"
      "44");
  ASSERT_TRUE(root.get());
  EXPECT_EQ(JSONValue::kTypeInteger, root->GetType());
  EXPECT_TRUE(root->AsInteger(&int_val));
  EXPECT_EQ(44, int_val);

  // Test number formats
  root = ParseJSON("43");
  ASSERT_TRUE(root.get());
  EXPECT_EQ(JSONValue::kTypeInteger, root->GetType());
  EXPECT_TRUE(root->AsInteger(&int_val));
  EXPECT_EQ(43, int_val);

  // According to RFC4627, oct, hex, and leading zeros are invalid JSON.
  root = ParseJSON("043");
  EXPECT_FALSE(root.get());
  root = ParseJSON("0x43");
  EXPECT_FALSE(root.get());
  root = ParseJSON("00");
  EXPECT_FALSE(root.get());

  // Test 0 (which needs to be special cased because of the leading zero
  // clause).
  root = ParseJSON("0");
  ASSERT_TRUE(root.get());
  EXPECT_EQ(JSONValue::kTypeInteger, root->GetType());
  int_val = 1;
  EXPECT_TRUE(root->AsInteger(&int_val));
  EXPECT_EQ(0, int_val);

  // Numbers that overflow ints should succeed, being internally promoted to
  // storage as doubles
  root = ParseJSON("2147483648");
  ASSERT_TRUE(root.get());
  double double_val;
  EXPECT_EQ(JSONValue::kTypeDouble, root->GetType());
  double_val = 0.0;
  EXPECT_TRUE(root->AsDouble(&double_val));
  EXPECT_DOUBLE_EQ(2147483648.0, double_val);
  root = ParseJSON("-2147483649");
  ASSERT_TRUE(root.get());
  EXPECT_EQ(JSONValue::kTypeDouble, root->GetType());
  double_val = 0.0;
  EXPECT_TRUE(root->AsDouble(&double_val));
  EXPECT_DOUBLE_EQ(-2147483649.0, double_val);

  // Parse a double
  root = ParseJSON("43.1");
  ASSERT_TRUE(root.get());
  EXPECT_EQ(JSONValue::kTypeDouble, root->GetType());
  double_val = 0.0;
  EXPECT_TRUE(root->AsDouble(&double_val));
  EXPECT_DOUBLE_EQ(43.1, double_val);

  root = ParseJSON("4.3e-1");
  ASSERT_TRUE(root.get());
  EXPECT_EQ(JSONValue::kTypeDouble, root->GetType());
  double_val = 0.0;
  EXPECT_TRUE(root->AsDouble(&double_val));
  EXPECT_DOUBLE_EQ(.43, double_val);

  root = ParseJSON("2.1e0");
  ASSERT_TRUE(root.get());
  EXPECT_EQ(JSONValue::kTypeDouble, root->GetType());
  double_val = 0.0;
  EXPECT_TRUE(root->AsDouble(&double_val));
  EXPECT_DOUBLE_EQ(2.1, double_val);

  root = ParseJSON("2.1e+0001");
  ASSERT_TRUE(root.get());
  EXPECT_EQ(JSONValue::kTypeInteger, root->GetType());
  double_val = 0.0;
  EXPECT_TRUE(root->AsDouble(&double_val));
  EXPECT_DOUBLE_EQ(21.0, double_val);

  root = ParseJSON("0.01");
  ASSERT_TRUE(root.get());
  EXPECT_EQ(JSONValue::kTypeDouble, root->GetType());
  double_val = 0.0;
  EXPECT_TRUE(root->AsDouble(&double_val));
  EXPECT_DOUBLE_EQ(0.01, double_val);

  root = ParseJSON("1.00");
  ASSERT_TRUE(root.get());
  EXPECT_EQ(JSONValue::kTypeInteger, root->GetType());
  double_val = 0.0;
  EXPECT_TRUE(root->AsDouble(&double_val));
  EXPECT_DOUBLE_EQ(1.0, double_val);

  // Fractional parts must have a digit before and after the decimal point.
  root = ParseJSON("1.");
  EXPECT_FALSE(root.get());
  root = ParseJSON(".1");
  EXPECT_FALSE(root.get());
  root = ParseJSON("1.e10");
  EXPECT_FALSE(root.get());

  // Exponent must have a digit following the 'e'.
  root = ParseJSON("1e");
  EXPECT_FALSE(root.get());
  root = ParseJSON("1E");
  EXPECT_FALSE(root.get());
  root = ParseJSON("1e1.");
  EXPECT_FALSE(root.get());
  root = ParseJSON("1e1.0");
  EXPECT_FALSE(root.get());

  // INF/-INF/NaN are not valid
  root = ParseJSON("NaN");
  EXPECT_FALSE(root.get());
  root = ParseJSON("nan");
  EXPECT_FALSE(root.get());
  root = ParseJSON("inf");
  EXPECT_FALSE(root.get());

  // Invalid number formats
  root = ParseJSON("4.3.1");
  EXPECT_FALSE(root.get());
  root = ParseJSON("4e3.1");
  EXPECT_FALSE(root.get());

  // Test string parser
  root = ParseJSON("\"hello world\"");
  ASSERT_TRUE(root.get());
  EXPECT_EQ(JSONValue::kTypeString, root->GetType());
  EXPECT_TRUE(root->AsString(&str_val));
  EXPECT_EQ("hello world", str_val);

  // Empty string
  root = ParseJSON("\"\"");
  ASSERT_TRUE(root.get());
  EXPECT_EQ(JSONValue::kTypeString, root->GetType());
  EXPECT_TRUE(root->AsString(&str_val));
  EXPECT_EQ("", str_val);

  // Test basic string escapes
  root = ParseJSON("\" \\\"\\\\\\/\\b\\f\\n\\r\\t\\v\"");
  ASSERT_TRUE(root.get());
  EXPECT_EQ(JSONValue::kTypeString, root->GetType());
  EXPECT_TRUE(root->AsString(&str_val));
  EXPECT_EQ(" \"\\/\b\f\n\r\t\v", str_val);

  // Test hex and unicode escapes including the null character.
  root = ParseJSON("\"\\x41\\x00\\u1234\"");
  EXPECT_FALSE(root.get());

  // Test invalid strings
  root = ParseJSON("\"no closing quote");
  EXPECT_FALSE(root.get());
  root = ParseJSON("\"\\z invalid escape char\"");
  EXPECT_FALSE(root.get());
  root = ParseJSON("\"not enough escape chars\\u123\"");
  EXPECT_FALSE(root.get());
  root = ParseJSON("\"extra backslash at end of input\\\"");
  EXPECT_FALSE(root.get());

  // Basic array
  root = ParseJSON("[true, false, null]");
  ASSERT_TRUE(root.get());
  EXPECT_EQ(JSONValue::kTypeArray, root->GetType());
  list = JSONArray::Cast(root.get());
  ASSERT_TRUE(list);
  EXPECT_EQ(3U, list->size());

  // Empty array
  root = ParseJSON("[]");
  ASSERT_TRUE(root.get());
  EXPECT_EQ(JSONValue::kTypeArray, root->GetType());
  list = JSONArray::Cast(root.get());
  ASSERT_TRUE(list);
  EXPECT_EQ(0U, list->size());

  // Nested arrays
  root = ParseJSON("[[true], [], [false, [], [null]], null]");
  ASSERT_TRUE(root.get());
  EXPECT_EQ(JSONValue::kTypeArray, root->GetType());
  list = JSONArray::Cast(root.get());
  ASSERT_TRUE(list);
  EXPECT_EQ(4U, list->size());

  // Invalid, missing close brace.
  root = ParseJSON("[[true], [], [false, [], [null]], null");
  EXPECT_FALSE(root.get());

  // Invalid, too many commas
  root = ParseJSON("[true,, null]");
  EXPECT_FALSE(root.get());

  // Invalid, no commas
  root = ParseJSON("[true null]");
  EXPECT_FALSE(root.get());

  // Invalid, trailing comma
  root = ParseJSON("[true,]");
  EXPECT_FALSE(root.get());

  root = ParseJSON("[true]");
  ASSERT_TRUE(root.get());
  EXPECT_EQ(JSONValue::kTypeArray, root->GetType());
  list = JSONArray::Cast(root.get());
  ASSERT_TRUE(list);
  EXPECT_EQ(1U, list->size());
  tmp_value = list->at(0);
  ASSERT_TRUE(tmp_value);
  EXPECT_EQ(JSONValue::kTypeBoolean, tmp_value->GetType());
  bool bool_value = false;
  EXPECT_TRUE(tmp_value->AsBoolean(&bool_value));
  EXPECT_TRUE(bool_value);

  // Don't allow empty elements.
  root = ParseJSON("[,]");
  EXPECT_FALSE(root.get());
  root = ParseJSON("[true,,]");
  EXPECT_FALSE(root.get());
  root = ParseJSON("[,true,]");
  EXPECT_FALSE(root.get());
  root = ParseJSON("[true,,false]");
  EXPECT_FALSE(root.get());

  // Test objects
  root = ParseJSON("{}");
  ASSERT_TRUE(root.get());
  EXPECT_EQ(JSONValue::kTypeObject, root->GetType());

  root = ParseJSON("{\"number\":9.87654321, \"null\":null , \"S\" : \"str\" }");
  ASSERT_TRUE(root.get());
  EXPECT_EQ(JSONValue::kTypeObject, root->GetType());
  JSONObject* object_val = JSONObject::Cast(root.get());
  ASSERT_TRUE(object_val);
  double_val = 0.0;
  EXPECT_TRUE(object_val->GetDouble("number", &double_val));
  EXPECT_DOUBLE_EQ(9.87654321, double_val);
  JSONValue* null_val = object_val->Get("null");
  ASSERT_TRUE(null_val);
  EXPECT_EQ(JSONValue::kTypeNull, null_val->GetType());
  EXPECT_TRUE(object_val->GetString("S", &str_val));
  EXPECT_EQ("str", str_val);

  // Test newline equivalence.
  root2 = ParseJSON(
      "{\n"
      "  \"number\":9.87654321,\n"
      "  \"null\":null,\n"
      "  \"S\":\"str\"\n"
      "}\n");
  ASSERT_TRUE(root2.get());
  EXPECT_EQ(root->ToJSONString(), root2->ToJSONString());

  root2 = ParseJSON(
      "{\r\n"
      "  \"number\":9.87654321,\r\n"
      "  \"null\":null,\r\n"
      "  \"S\":\"str\"\r\n"
      "}\r\n");
  ASSERT_TRUE(root2.get());
  EXPECT_EQ(root->ToJSONString(), root2->ToJSONString());

  // Test nesting
  root = ParseJSON("{\"inner\":{\"array\":[true]},\"false\":false,\"d\":{}}");
  ASSERT_TRUE(root.get());
  EXPECT_EQ(JSONValue::kTypeObject, root->GetType());
  object_val = JSONObject::Cast(root.get());
  ASSERT_TRUE(object_val);
  JSONObject* inner_object = object_val->GetJSONObject("inner");
  ASSERT_TRUE(inner_object);
  JSONArray* inner_array = inner_object->GetArray("array");
  ASSERT_TRUE(inner_array);
  EXPECT_EQ(1U, inner_array->size());
  bool_value = true;
  EXPECT_TRUE(object_val->GetBoolean("false", &bool_value));
  EXPECT_FALSE(bool_value);
  inner_object = object_val->GetJSONObject("d");
  EXPECT_TRUE(inner_object);

  // Test keys with periods
  root = ParseJSON("{\"a.b\":3,\"c\":2,\"d.e.f\":{\"g.h.i.j\":1}}");
  ASSERT_TRUE(root.get());
  EXPECT_EQ(JSONValue::kTypeObject, root->GetType());
  object_val = JSONObject::Cast(root.get());
  ASSERT_TRUE(object_val);
  int integer_value = 0;
  EXPECT_TRUE(object_val->GetInteger("a.b", &integer_value));
  EXPECT_EQ(3, integer_value);
  EXPECT_TRUE(object_val->GetInteger("c", &integer_value));
  EXPECT_EQ(2, integer_value);
  inner_object = object_val->GetJSONObject("d.e.f");
  ASSERT_TRUE(inner_object);
  EXPECT_EQ(1U, inner_object->size());
  EXPECT_TRUE(inner_object->GetInteger("g.h.i.j", &integer_value));
  EXPECT_EQ(1, integer_value);

  root = ParseJSON("{\"a\":{\"b\":2},\"a.b\":1}");
  ASSERT_TRUE(root.get());
  EXPECT_EQ(JSONValue::kTypeObject, root->GetType());
  object_val = JSONObject::Cast(root.get());
  ASSERT_TRUE(object_val);
  inner_object = object_val->GetJSONObject("a");
  ASSERT_TRUE(inner_object);
  EXPECT_TRUE(inner_object->GetInteger("b", &integer_value));
  EXPECT_EQ(2, integer_value);
  EXPECT_TRUE(object_val->GetInteger("a.b", &integer_value));
  EXPECT_EQ(1, integer_value);

  // Invalid, no closing brace
  root = ParseJSON("{\"a\": true");
  EXPECT_FALSE(root.get());

  // Invalid, keys must be quoted
  root = ParseJSON("{foo:true}");
  EXPECT_FALSE(root.get());

  // Invalid, trailing comma
  root = ParseJSON("{\"a\":true,}");
  EXPECT_FALSE(root.get());

  // Invalid, too many commas
  root = ParseJSON("{\"a\":true,,\"b\":false}");
  EXPECT_FALSE(root.get());

  // Invalid, no separator
  root = ParseJSON("{\"a\" \"b\"}");
  EXPECT_FALSE(root.get());

  // Invalid, lone comma.
  root = ParseJSON("{,}");
  EXPECT_FALSE(root.get());
  root = ParseJSON("{\"a\":true,,}");
  EXPECT_FALSE(root.get());
  root = ParseJSON("{,\"a\":true}");
  EXPECT_FALSE(root.get());
  root = ParseJSON("{\"a\":true,,\"b\":false}");
  EXPECT_FALSE(root.get());

  // Test stack overflow
  StringBuilder evil;
  evil.ReserveCapacity(2000000);
  for (int i = 0; i < 1000000; ++i)
    evil.Append('[');
  for (int i = 0; i < 1000000; ++i)
    evil.Append(']');
  root = ParseJSON(evil.ToString());
  EXPECT_FALSE(root.get());

  // A few thousand adjacent lists is fine.
  StringBuilder not_evil;
  not_evil.ReserveCapacity(15010);
  not_evil.Append('[');
  for (int i = 0; i < 5000; ++i)
    not_evil.Append("[],");
  not_evil.Append("[]]");
  root = ParseJSON(not_evil.ToString());
  ASSERT_TRUE(root.get());
  EXPECT_EQ(JSONValue::kTypeArray, root->GetType());
  list = JSONArray::Cast(root.get());
  ASSERT_TRUE(list);
  EXPECT_EQ(5001U, list->size());

  // Test utf8 encoded input
  root = ParseJSON("\"\\xe7\\xbd\\x91\\xe9\\xa1\\xb5\"");
  ASSERT_FALSE(root.get());

  // Test utf16 encoded strings.
  root = ParseJSON("\"\\u20ac3,14\"");
  ASSERT_TRUE(root.get());
  EXPECT_EQ(JSONValue::kTypeString, root->GetType());
  EXPECT_TRUE(root->AsString(&str_val));
  UChar tmp2[] = {0x20ac, 0x33, 0x2c, 0x31, 0x34};
  EXPECT_EQ(String(tmp2, WTF_ARRAY_LENGTH(tmp2)), str_val);

  root = ParseJSON("\"\\ud83d\\udca9\\ud83d\\udc6c\"");
  ASSERT_TRUE(root.get());
  EXPECT_EQ(JSONValue::kTypeString, root->GetType());
  EXPECT_TRUE(root->AsString(&str_val));
  UChar tmp3[] = {0xd83d, 0xdca9, 0xd83d, 0xdc6c};
  EXPECT_EQ(String(tmp3, WTF_ARRAY_LENGTH(tmp3)), str_val);

  // Test literal root objects.
  root = ParseJSON("null");
  EXPECT_EQ(JSONValue::kTypeNull, root->GetType());

  root = ParseJSON("true");
  ASSERT_TRUE(root.get());
  EXPECT_TRUE(root->AsBoolean(&bool_value));
  EXPECT_TRUE(bool_value);

  root = ParseJSON("10");
  ASSERT_TRUE(root.get());
  EXPECT_TRUE(root->AsInteger(&integer_value));
  EXPECT_EQ(10, integer_value);

  root = ParseJSON("\"root\"");
  ASSERT_TRUE(root.get());
  EXPECT_TRUE(root->AsString(&str_val));
  EXPECT_EQ("root", str_val);
}

TEST(JSONParserTest, InvalidSanity) {
  const char* const kInvalidJson[] = {
      "/* test *", "{\"foo\"", "{\"foo\":", "  [", "\"\\u123g\"", "{\n\"eh:\n}",
      "////",      "*/**/",    "/**/",      "/*/", "//**/",       "\"\\"};

  for (size_t i = 0; i < WTF_ARRAY_LENGTH(kInvalidJson); ++i) {
    std::unique_ptr<JSONValue> result = ParseJSON(kInvalidJson[i]);
    EXPECT_FALSE(result.get());
  }
}

// Test that the nesting depth can be limited to values less than 1000, but
// cannot be extended past that maximum.
TEST(JSONParserTest, LimitedDepth) {
  std::unique_ptr<JSONValue> root;

  // Test cases. Each pair is a JSON string, and the minimum depth required
  // to successfully parse that string.
  std::vector<std::pair<const char*, int>> test_cases = {
      {"[[[[[]]]]]", 5},
      {"[[[[[\"a\"]]]]]", 6},
      {"[[],[],[],[],[]]", 2},
      {"{\"a\":{\"a\":{\"a\":{\"a\":{\"a\": \"a\"}}}}}", 6},
      {"\"root\"", 1}};

  for (const auto& test_case : test_cases) {
    // Each test case should parse successfully at the default depth
    root = ParseJSON(test_case.first);
    EXPECT_TRUE(root.get());

    // ... and should parse successfully at the minimum depth
    root = ParseJSON(test_case.first, test_case.second);
    EXPECT_TRUE(root.get());

    // ... but should fail to parse at a shallower depth.
    root = ParseJSON(test_case.first, test_case.second - 1);
    EXPECT_FALSE(root.get());
  }

  // Test that everything fails to parse with depth 0
  root = ParseJSON("", 0);
  EXPECT_FALSE(root.get());
  root = ParseJSON("", -1);
  EXPECT_FALSE(root.get());
  root = ParseJSON("true", 0);
  EXPECT_FALSE(root.get());

  // Test that the limit can be set to the constant maximum.
  StringBuilder evil;
  evil.ReserveCapacity(2002);
  for (int i = 0; i < 1000; ++i)
    evil.Append('[');
  for (int i = 0; i < 1000; ++i)
    evil.Append(']');
  root = ParseJSON(evil.ToString());
  EXPECT_TRUE(root.get());
  root = ParseJSON(evil.ToString(), 1000);
  EXPECT_TRUE(root.get());

  // Test that the limit cannot be set higher than the constant maximum.
  evil.Clear();
  for (int i = 0; i < 1001; ++i)
    evil.Append('[');
  for (int i = 0; i < 1001; ++i)
    evil.Append(']');
  root = ParseJSON(evil.ToString());
  EXPECT_FALSE(root.get());
  root = ParseJSON(evil.ToString(), 1001);
  EXPECT_FALSE(root.get());
}

}  // namespace blink
