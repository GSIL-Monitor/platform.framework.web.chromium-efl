// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_EFL_INTEGRATION_RENDERER_GIN_NATIVE_FUNCTION_INVOCATION_HELPER_H_
#define EWK_EFL_INTEGRATION_RENDERER_GIN_NATIVE_FUNCTION_INVOCATION_HELPER_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "gin/arguments.h"
#include "gin/handle.h"
#include "renderer/gin_native_bridge_dispatcher.h"

namespace content {

class GinNativeBridgeValueConverter;

class GinNativeFunctionInvocationHelper {
 public:
  GinNativeFunctionInvocationHelper(
      const std::string& method_name,
      const base::WeakPtr<GinNativeBridgeDispatcher>& dispatcher);
  ~GinNativeFunctionInvocationHelper();

  v8::Local<v8::Value> Invoke(gin::Arguments* args);

 private:
  std::string method_name_;
  base::WeakPtr<GinNativeBridgeDispatcher> dispatcher_;
  std::unique_ptr<GinNativeBridgeValueConverter> converter_;

  DISALLOW_COPY_AND_ASSIGN(GinNativeFunctionInvocationHelper);
};

}  // namespace content

#endif  // EWK_EFL_INTEGRATION_RENDERER_GIN_NATIVE_FUNCTION_INVOCATION_HELPER_H_
