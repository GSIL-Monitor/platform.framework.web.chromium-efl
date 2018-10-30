// Copyright (c) 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/cpp/samsung/teec_session_samsung.h"

#include "ppapi/c/pp_errors.h"
#include "ppapi/cpp/module_impl.h"

namespace pp {

namespace {

template <>
const char* interface_name<PPB_TEECSession_Samsung_1_0>() {
  return PPB_TEECSESSION_SAMSUNG_INTERFACE_1_0;
}

}  // namespace

TEECSession_Samsung::TEECSession_Samsung() {}

TEECSession_Samsung::TEECSession_Samsung(PP_Resource resource)
    : Resource(resource) {}

TEECSession_Samsung::TEECSession_Samsung(PassRef, PP_Resource resource)
    : Resource(PASS_REF, resource) {}

TEECSession_Samsung::TEECSession_Samsung(const TEECContext_Samsung& context) {
  if (has_interface<PPB_TEECSession_Samsung_1_0>()) {
    PassRefFromConstructor(get_interface<PPB_TEECSession_Samsung_1_0>()->Create(
        context.pp_resource()));
  }
}

TEECSession_Samsung::TEECSession_Samsung(const TEECSession_Samsung& other)
    : Resource(other) {}

TEECSession_Samsung& TEECSession_Samsung::operator=(
    const TEECSession_Samsung& other) {
  Resource::operator=(other);
  return *this;
}

TEECSession_Samsung::~TEECSession_Samsung() {}

int32_t TEECSession_Samsung::Open(
    PP_TEEC_UUID* destination,
    uint32_t connection_method,
    uint32_t connection_data_size,
    const void* connection_data,
    PP_TEEC_Operation* operation,
    const CompletionCallbackWithOutput<PP_TEEC_Result>& callback) {
  if (has_interface<PPB_TEECSession_Samsung_1_0>()) {
    return get_interface<PPB_TEECSession_Samsung_1_0>()->Open(
        pp_resource(), destination, connection_method, connection_data_size,
        connection_data, operation, callback.output(),
        callback.pp_completion_callback());
  }

  return callback.MayForce(PP_ERROR_NOINTERFACE);
}

int32_t TEECSession_Samsung::InvokeCommand(
    uint32_t command_id,
    PP_TEEC_Operation* operation,
    const CompletionCallbackWithOutput<PP_TEEC_Result>& callback) {
  if (has_interface<PPB_TEECSession_Samsung_1_0>()) {
    return get_interface<PPB_TEECSession_Samsung_1_0>()->InvokeCommand(
        pp_resource(), command_id, operation, callback.output(),
        callback.pp_completion_callback());
  }

  return callback.MayForce(PP_ERROR_NOINTERFACE);
}

int32_t TEECSession_Samsung::RequestCancellation(
    const PP_TEEC_Operation* operation,
    const CompletionCallback& callback) {
  if (has_interface<PPB_TEECSession_Samsung_1_0>()) {
    return get_interface<PPB_TEECSession_Samsung_1_0>()->RequestCancellation(
        pp_resource(), operation, callback.pp_completion_callback());
  }

  return callback.MayForce(PP_ERROR_NOINTERFACE);
}

}  // namespace pp
