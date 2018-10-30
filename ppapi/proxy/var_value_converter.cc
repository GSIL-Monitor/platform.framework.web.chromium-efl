// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/var_value_converter.h"

#include <string>

#include "base/memory/ptr_util.h"
#include "ppapi/shared_impl/array_var.h"
#include "ppapi/shared_impl/dictionary_var.h"

namespace ppapi {
namespace proxy {

namespace {

std::unique_ptr<base::Value> ValueFromVarArray(const PP_Var& var) {
  if (var.type == PP_VARTYPE_ARRAY) {
    scoped_refptr<ArrayVar> array =
        scoped_refptr<ArrayVar>(ArrayVar::FromPPVar(var));
    if (!array) {
      return base::MakeUnique<base::Value>();
    }
    auto ret = base::MakeUnique<base::ListValue>();
    for (size_t i = 0; i < array->GetLength(); ++i) {
      ScopedPPVar var(ScopedPPVar::PassRef(), array->Get(i));
      ret->Append(ValueFromVar(var.get()));
    }
    return std::move(ret);
  }

  return base::MakeUnique<base::Value>();
}

std::unique_ptr<base::Value> ValueFromVarDictionary(const PP_Var& var) {
  if (var.type == PP_VARTYPE_DICTIONARY) {
    scoped_refptr<DictionaryVar> dict =
        scoped_refptr<DictionaryVar>(DictionaryVar::FromPPVar(var));
    if (!dict) {
      return base::MakeUnique<base::Value>();
    }
    ScopedPPVar keys_var(ScopedPPVar::PassRef(), dict->GetKeys());
    scoped_refptr<ArrayVar> keys =
        scoped_refptr<ArrayVar>(ArrayVar::FromPPVar(keys_var.get()));
    if (!keys) {
      return base::MakeUnique<base::Value>();
    }

    auto ret = base::MakeUnique<base::DictionaryValue>();
    for (size_t i = 0; i < keys->GetLength(); ++i) {
      ScopedPPVar var_k(ScopedPPVar::PassRef(), keys->Get(i));
      scoped_refptr<StringVar> key =
          scoped_refptr<StringVar>(StringVar::FromPPVar(var_k.get()));
      if (!key) {
        continue;
      }
      std::string key_string = key->value();

      ScopedPPVar var_v(ScopedPPVar::PassRef(), dict->Get(var_k.get()));
      // SetWithoutPathExpansion is used instead of Set here to allow
      // e.g. URLs to be used as keys. Set method treats '.' as keys separator.
      ret->SetWithoutPathExpansion(key_string, ValueFromVar(var_v.get()));
    }
    return std::move(ret);
  }

  return base::MakeUnique<base::Value>();
}

ScopedPPVar VarFromValueArray(const base::Value* value) {
  if (!value)
    return ScopedPPVar();

  if (value->GetType() == base::Value::Type::LIST) {
    scoped_refptr<ArrayVar> ret(new ArrayVar);
    const base::ListValue* list;
    value->GetAsList(&list);

    ret->SetLength(list->GetSize());
    for (size_t i = 0; i < list->GetSize(); ++i) {
      const base::Value* val;
      list->Get(i, &val);
      ScopedPPVar var = VarFromValue(val);
      ret->Set(i, var.get());
    }

    return ScopedPPVar(ScopedPPVar::PassRef(), ret->GetPPVar());
  }

  return ScopedPPVar();
}

ScopedPPVar VarFromValueDictionary(const base::Value* value) {
  if (!value)
    return ScopedPPVar();

  if (value->GetType() == base::Value::Type::DICTIONARY) {
    scoped_refptr<DictionaryVar> ret(new DictionaryVar);
    const base::DictionaryValue* dict;
    value->GetAsDictionary(&dict);
    base::DictionaryValue::Iterator it(*dict);
    while (!it.IsAtEnd()) {
      ScopedPPVar var_k(ScopedPPVar::PassRef(),
                        StringVar::StringToPPVar(it.key()));
      ScopedPPVar var_v = VarFromValue(&it.value());
      ret->Set(var_k.get(), var_v.get());
      it.Advance();
    }
    return ScopedPPVar(ScopedPPVar::PassRef(), ret->GetPPVar());
  }

  return ScopedPPVar();
}

}  // namespace

std::unique_ptr<base::Value> ValueFromVar(const PP_Var& var) {
  switch (var.type) {
    case PP_VARTYPE_BOOL:
      return base::MakeUnique<base::Value>(PP_ToBool(var.value.as_bool));
    case PP_VARTYPE_INT32:
      return base::MakeUnique<base::Value>(var.value.as_int);
    case PP_VARTYPE_DOUBLE:
      return base::MakeUnique<base::Value>(var.value.as_double);
    case PP_VARTYPE_STRING: {
      StringVar* str = StringVar::FromPPVar(var);
      if (!str)
        return base::MakeUnique<base::Value>();
      return base::MakeUnique<base::Value>(str->value());
    }
    case PP_VARTYPE_ARRAY:
      return ValueFromVarArray(var);
    case PP_VARTYPE_DICTIONARY:
      return ValueFromVarDictionary(var);
    default:
      return base::MakeUnique<base::Value>();
  }
}

ScopedPPVar VarFromValue(const base::Value* value) {
  if (!value)
    return ScopedPPVar();

  switch (value->GetType()) {
    case base::Value::Type::BOOLEAN: {
      bool val;
      value->GetAsBoolean(&val);
      return ScopedPPVar(ScopedPPVar::PassRef(), PP_MakeBool(PP_FromBool(val)));
    }
    case base::Value::Type::INTEGER: {
      int val;
      value->GetAsInteger(&val);
      return ScopedPPVar(ScopedPPVar::PassRef(), PP_MakeInt32(val));
    }
    case base::Value::Type::DOUBLE: {
      double val;
      value->GetAsDouble(&val);
      return ScopedPPVar(ScopedPPVar::PassRef(), PP_MakeDouble(val));
    }
    case base::Value::Type::STRING: {
      std::string val;
      value->GetAsString(&val);
      return ScopedPPVar(ScopedPPVar::PassRef(), StringVar::StringToPPVar(val));
    }
    case base::Value::Type::LIST:
      return VarFromValueArray(value);
    case base::Value::Type::DICTIONARY:
      return VarFromValueDictionary(value);
    default:
      return ScopedPPVar();
  }
}

}  // namespace proxy
}  // namespace ppapi
