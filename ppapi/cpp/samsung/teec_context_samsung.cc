// Copyright (c) 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/cpp/samsung/teec_context_samsung.h"

#include "ppapi/c/pp_errors.h"
#include "ppapi/cpp/module_impl.h"

namespace pp {

namespace {

template <>
const char* interface_name<PPB_TEECContext_Samsung_1_0>() {
  return PPB_TEECCONTEXT_SAMSUNG_INTERFACE_1_0;
}

}  // namespace

TEECContext_Samsung::TEECContext_Samsung() {}

TEECContext_Samsung::TEECContext_Samsung(const InstanceHandle& instance) {
  if (has_interface<PPB_TEECContext_Samsung_1_0>()) {
    PassRefFromConstructor(get_interface<PPB_TEECContext_Samsung_1_0>()->Create(
        instance.pp_instance()));
  }
}

TEECContext_Samsung::TEECContext_Samsung(PP_Resource resource)
    : Resource(resource) {}

TEECContext_Samsung::TEECContext_Samsung(PassRef, PP_Resource resource)
    : Resource(PASS_REF, resource) {}

TEECContext_Samsung::TEECContext_Samsung(const TEECContext_Samsung& other)
    : Resource(other) {}

TEECContext_Samsung& TEECContext_Samsung::operator=(
    const TEECContext_Samsung& other) {
  Resource::operator=(other);
  return *this;
}

TEECContext_Samsung::~TEECContext_Samsung() {}

int32_t TEECContext_Samsung::Open(
    const std::string& context_name,
    const CompletionCallbackWithOutput<PP_TEEC_Result>& callback) {
  if (has_interface<PPB_TEECContext_Samsung_1_0>()) {
    return get_interface<PPB_TEECContext_Samsung_1_0>()->Open(
        pp_resource(), context_name.c_str(), callback.output(),
        callback.pp_completion_callback());
  }

  return callback.MayForce(PP_ERROR_NOINTERFACE);
}

}  // namespace pp
