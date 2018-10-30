// Copyright (c) 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/cpp/samsung/teec_shared_memory_samsung.h"

#include "ppapi/c/pp_errors.h"
#include "ppapi/cpp/module_impl.h"

namespace pp {

namespace {

template <>
const char* interface_name<PPB_TEECSharedMemory_Samsung_1_0>() {
  return PPB_TEECSHAREDMEMORY_SAMSUNG_INTERFACE_1_0;
}

}  // namespace

TEECSharedMemory_Samsung::TEECSharedMemory_Samsung() {}

TEECSharedMemory_Samsung::TEECSharedMemory_Samsung(PP_Resource resource)
    : Resource(resource) {}

TEECSharedMemory_Samsung::TEECSharedMemory_Samsung(PassRef,
                                                   PP_Resource resource)
    : Resource(PASS_REF, resource) {}

TEECSharedMemory_Samsung::TEECSharedMemory_Samsung(
    const TEECContext_Samsung& context,
    uint32_t flags) {
  if (has_interface<PPB_TEECSharedMemory_Samsung_1_0>()) {
    PassRefFromConstructor(
        get_interface<PPB_TEECSharedMemory_Samsung_1_0>()->Create(
            context.pp_resource(), flags));
  }
}

TEECSharedMemory_Samsung::TEECSharedMemory_Samsung(
    const TEECSharedMemory_Samsung& other)
    : Resource(other) {}

TEECSharedMemory_Samsung& TEECSharedMemory_Samsung::operator=(
    const TEECSharedMemory_Samsung& other) {
  Resource::operator=(other);
  return *this;
}

TEECSharedMemory_Samsung::~TEECSharedMemory_Samsung() {}

int32_t TEECSharedMemory_Samsung::Register(
    const void* buffer,
    uint32_t size,
    const CompletionCallbackWithOutput<PP_TEEC_Result>& callback) {
  if (has_interface<PPB_TEECSharedMemory_Samsung_1_0>()) {
    return get_interface<PPB_TEECSharedMemory_Samsung_1_0>()->Register(
        pp_resource(), buffer, size, callback.output(),
        callback.pp_completion_callback());
  }

  return callback.MayForce(PP_ERROR_NOINTERFACE);
}

int32_t TEECSharedMemory_Samsung::Allocate(
    uint32_t size,
    const CompletionCallbackWithOutput<PP_TEEC_Result>& callback) {
  if (has_interface<PPB_TEECSharedMemory_Samsung_1_0>()) {
    return get_interface<PPB_TEECSharedMemory_Samsung_1_0>()->Allocate(
        pp_resource(), size, callback.output(),
        callback.pp_completion_callback());
  }

  return callback.MayForce(PP_ERROR_NOINTERFACE);
}

void* TEECSharedMemory_Samsung::Map() {
  if (has_interface<PPB_TEECSharedMemory_Samsung_1_0>()) {
    return get_interface<PPB_TEECSharedMemory_Samsung_1_0>()->Map(
        pp_resource());
  }

  return 0;
}

}  // namespace pp
