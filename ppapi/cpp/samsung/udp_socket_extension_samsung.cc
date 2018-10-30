// Copyright (c) 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/c/pp_errors.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/module_impl.h"
#include "ppapi/cpp/samsung/udp_socket_extension_samsung.h"

namespace pp {

namespace {

template <> const char* interface_name<PPB_UDPSocketExtension_Samsung_0_1>() {
  return PPB_UDPSOCKETEXTENSION_SAMSUNG_INTERFACE_0_1;
}

}  // namespace

UDPSocketExtensionSamsung::UDPSocketExtensionSamsung()
    : UDPSocket() {
}

UDPSocketExtensionSamsung::UDPSocketExtensionSamsung(
    const InstanceHandle& instance)
    : UDPSocket(instance) {
}

int32_t UDPSocketExtensionSamsung::GetOption(
    PP_UDPSocket_Option name,
    const CompletionCallbackWithOutput<Var>& callback) {
  if (has_interface<PPB_UDPSocketExtension_Samsung_0_1>()) {
    return get_interface<PPB_UDPSocketExtension_Samsung_0_1>()->GetOption(
        pp_resource(),
        name,
        callback.output(),
        callback.pp_completion_callback());
  }
  return callback.MayForce(PP_ERROR_NOINTERFACE);
}

int32_t UDPSocketExtensionSamsung::JoinMulticast(
    const struct PP_MulticastMembership& membership,
    const CompletionCallback& callback) {
  if (has_interface<PPB_UDPSocketExtension_Samsung_0_1>()) {
    return get_interface<PPB_UDPSocketExtension_Samsung_0_1>()->JoinMulticast(
        pp_resource(), &membership, callback.pp_completion_callback());
  }
  return callback.MayForce(PP_ERROR_NOINTERFACE);
}

int32_t UDPSocketExtensionSamsung::LeaveMulticast(
    const struct PP_MulticastMembership& membership,
    const CompletionCallback& callback) {
  if (has_interface<PPB_UDPSocketExtension_Samsung_0_1>()) {
    return get_interface<PPB_UDPSocketExtension_Samsung_0_1>()->LeaveMulticast(
        pp_resource(), &membership, callback.pp_completion_callback());
  }
  return callback.MayForce(PP_ERROR_NOINTERFACE);
}

}  // namespace pp
