// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_EFL_INTEGRATION_RENDERER_GIN_NATIVE_BRIDGE_VALUE_CONVERTER_H_
#define EWK_EFL_INTEGRATION_RENDERER_GIN_NATIVE_BRIDGE_VALUE_CONVERTER_H_

#include "content/common/content_export.h"
#include "content/public/child/v8_value_converter.h"

namespace content {

class GinNativeBridgeValueConverter
    : public content::V8ValueConverter::Strategy {
 public:
  CONTENT_EXPORT GinNativeBridgeValueConverter();
  CONTENT_EXPORT ~GinNativeBridgeValueConverter() override;

  CONTENT_EXPORT v8::Local<v8::Value> ToV8Value(
      const base::Value* value,
      v8::Local<v8::Context> context) const;
  CONTENT_EXPORT std::unique_ptr<base::Value> FromV8Value(
      v8::Local<v8::Value> value,
      v8::Local<v8::Context> context) const;

 private:
  std::unique_ptr<V8ValueConverter> converter_;
};

}  // namespace content

#endif  // EWK_EFL_INTEGRATION_RENDERER_GIN_NATIVE_BRIDGE_VALUE_CONVERTER_H_
