// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/gin_native_function_invocation_helper.h"

#include "common/gin_native_bridge_value.h"
#include "content/public/child/v8_value_converter.h"
#include "renderer/gin_native_bridge_object.h"
#include "renderer/gin_native_bridge_value_converter.h"

namespace content {

namespace {

const char kMethodInvocationAsConstructorDisallowed[] =
    "Native bridge method can't be invoked as a constructor";
const char kMethodInvocationOnNonInjectedObjectDisallowed[] =
    "Native bridge method can't be invoked on a non-injected object";
const char kMethodInvocationErrorMessage[] =
    "Native bridge method invocation error";

}  // namespace

/* LCOV_EXCL_START */
GinNativeFunctionInvocationHelper::GinNativeFunctionInvocationHelper(
    const std::string& method_name,
    const base::WeakPtr<GinNativeBridgeDispatcher>& dispatcher)
    : method_name_(method_name),
      dispatcher_(dispatcher),
      converter_(new GinNativeBridgeValueConverter()) {}

GinNativeFunctionInvocationHelper::~GinNativeFunctionInvocationHelper() {}

v8::Local<v8::Value> GinNativeFunctionInvocationHelper::Invoke(
    gin::Arguments* args) {
  if (!dispatcher_) {
    args->isolate()->ThrowException(v8::Exception::Error(
        gin::StringToV8(args->isolate(), kMethodInvocationErrorMessage)));
    return v8::Undefined(args->isolate());
  }

  if (args->IsConstructCall()) {
    args->isolate()->ThrowException(v8::Exception::Error(gin::StringToV8(
        args->isolate(), kMethodInvocationAsConstructorDisallowed)));
    return v8::Undefined(args->isolate());
  }

  content::GinNativeBridgeObject* object = NULL;
  if (!args->GetHolder(&object) || !object) {
    args->isolate()->ThrowException(v8::Exception::Error(gin::StringToV8(
        args->isolate(), kMethodInvocationOnNonInjectedObjectDisallowed)));
    return v8::Undefined(args->isolate());
  }

  base::ListValue arguments;
  {
    v8::HandleScope handle_scope(args->isolate());
    v8::Local<v8::Context> context = args->isolate()->GetCurrentContext();
    v8::Local<v8::Value> val;
    while (args->GetNext(&val)) {
      std::unique_ptr<base::Value> arg(converter_->FromV8Value(val, context));
      if (arg.get()) {
        arguments.Append(std::move(arg));
      } else {
        arguments.Append(base::MakeUnique<base::Value>());
      }
    }
  }

  GinNativeBridgeError error;
  std::unique_ptr<base::Value> result = dispatcher_->InvokeNativeMethod(
      object->object_id(), method_name_, arguments, &error);
  if (!result.get()) {
    args->isolate()->ThrowException(v8::Exception::Error(
        gin::StringToV8(args->isolate(), GinNativeBridgeErrorToString(error))));
    return v8::Undefined(args->isolate());
  }
  if (!result->IsType(base::Value::Type::BINARY)) {
    return converter_->ToV8Value(result.get(),
                                 args->isolate()->GetCurrentContext());
  }

  std::unique_ptr<const GinNativeBridgeValue> gin_value =
      GinNativeBridgeValue::FromValue(result.get());

  if (gin_value->IsType(GinNativeBridgeValue::TYPE_OBJECT_ID)) {
    GinNativeBridgeObject* result = NULL;
    GinNativeBridgeDispatcher::ObjectID object_id;
    if (gin_value->GetAsObjectID(&object_id)) {
      result = dispatcher_->GetObject(object_id);
    }
    if (result) {
      gin::Handle<GinNativeBridgeObject> controller =
          gin::CreateHandle(args->isolate(), result);
      if (controller.IsEmpty())
        return v8::Undefined(args->isolate());
      return controller.ToV8();
    }
  } else if (gin_value->IsType(GinNativeBridgeValue::TYPE_NONFINITE)) {
    float float_value;
    gin_value->GetAsNonFinite(&float_value);
    return v8::Number::New(args->isolate(), float_value);
  }
  return v8::Undefined(args->isolate());
}
/* LCOV_EXCL_STOP */

}  // namespace content
