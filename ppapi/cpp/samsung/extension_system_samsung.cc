// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/cpp/samsung/extension_system_samsung.h"

#include "ppapi/c/pp_errors.h"
#include "ppapi/c/samsung/ppb_extension_system_samsung.h"
#include "ppapi/cpp/module_impl.h"

namespace pp {

namespace {

template <>
const char* interface_name<PPB_ExtensionSystem_Samsung_0_1>() {
  return PPB_EXTENSIONSYSTEM_SAMSUNG_INTERFACE_0_1;
}

}  // namespace

ExtensionSystemSamsung::ExtensionSystemSamsung(const InstanceHandle& instance)
    : instance_(instance) {}

ExtensionSystemSamsung::~ExtensionSystemSamsung() {}

std::string ExtensionSystemSamsung::GetEmbedderName() const {
  if (!has_interface<PPB_ExtensionSystem_Samsung_0_1>())
    return std::string();
  Var result(PASS_REF,
             get_interface<PPB_ExtensionSystem_Samsung_0_1>()->GetEmbedderName(
                 instance_.pp_instance()));
  return result.is_string() ? result.AsString() : std::string();
}

Var ExtensionSystemSamsung::GetCurrentExtensionInfo() {
  if (!has_interface<PPB_ExtensionSystem_Samsung_0_1>())
    return Var();
  PP_Var rv =
      get_interface<PPB_ExtensionSystem_Samsung_0_1>()->GetCurrentExtensionInfo(
          instance_.pp_instance());
  return Var(PASS_REF, rv);
}

Var ExtensionSystemSamsung::GenericSyncCall(const Var& operation_name,
                                            const Var& operation_data) {
  if (!has_interface<PPB_ExtensionSystem_Samsung_0_1>())
    return Var();
  PP_Var operation_result;
  int32_t error =
      get_interface<PPB_ExtensionSystem_Samsung_0_1>()->GenericSyncCall(
          instance_.pp_instance(), operation_name.pp_var(),
          operation_data.pp_var(), &operation_result);
  if (error != PP_OK)
    return Var();
  return Var(PASS_REF, operation_result);
}

}  // namespace pp
