// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/gin_native_bridge_value_converter.h"

#include <cmath>

#include "base/values.h"
#include "gin/array_buffer.h"
#include "renderer/gin_native_bridge_object.h"

namespace content {

/* LCOV_EXCL_START */
GinNativeBridgeValueConverter::GinNativeBridgeValueConverter()
    : converter_(V8ValueConverter::Create()) {
  converter_->SetDateAllowed(false);
  converter_->SetRegExpAllowed(false);
  converter_->SetFunctionAllowed(true);
  converter_->SetStrategy(this);
}

GinNativeBridgeValueConverter::~GinNativeBridgeValueConverter() {}

v8::Local<v8::Value> GinNativeBridgeValueConverter::ToV8Value(
    const base::Value* value,
    v8::Local<v8::Context> context) const {
  return converter_->ToV8Value(value, context);
}
std::unique_ptr<base::Value> GinNativeBridgeValueConverter::FromV8Value(
    v8::Local<v8::Value> value,
    v8::Local<v8::Context> context) const {
  return converter_->FromV8Value(value, context);
}
/* LCOV_EXCL_STOP */

}  // namespace content
