// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/cpp/samsung/remote_controller_samsung.h"

#include "ppapi/c/pp_errors.h"
#include "ppapi/cpp/module_impl.h"

namespace pp {

namespace {

template <>
const char* interface_name<PPB_RemoteController_Samsung_0_1>() {
  return PPB_REMOTECONTROLLER_SAMSUNG_INTERFACE_0_1;
}

}  // namespace

RemoteControllerSamsung::RemoteControllerSamsung(const InstanceHandle& instance)
    : instance_(instance) {}

int32_t RemoteControllerSamsung::RegisterKeys(uint32_t key_count,
                                              const char* keys[]) {
  if (!has_interface<PPB_RemoteController_Samsung_0_1>())
    return PP_ERROR_NOINTERFACE;

  return get_interface<PPB_RemoteController_Samsung_0_1>()->RegisterKeys(
      instance_.pp_instance(), key_count, keys);
}

int32_t RemoteControllerSamsung::RegisterKeys(
    const std::vector<std::string>& keys) {
  std::vector<const char*> v;
  v.reserve(keys.size());
  for (uint32_t i = 0; i < keys.size(); ++i)
    v.push_back(keys[i].c_str());
  return RegisterKeys(static_cast<uint32_t>(v.size()), &v[0]);
}

int32_t RemoteControllerSamsung::UnRegisterKeys(uint32_t key_count,
                                                const char* keys[]) {
  if (!has_interface<PPB_RemoteController_Samsung_0_1>())
    return PP_ERROR_NOINTERFACE;

  return get_interface<PPB_RemoteController_Samsung_0_1>()->UnRegisterKeys(
      instance_.pp_instance(), key_count, keys);
}

int32_t RemoteControllerSamsung::UnRegisterKeys(
    const std::vector<std::string>& keys) {
  std::vector<const char*> v;
  v.reserve(keys.size());
  for (uint32_t i = 0; i < keys.size(); ++i)
    v.push_back(keys[i].c_str());
  return UnRegisterKeys(static_cast<uint32_t>(v.size()), &v[0]);
}

}  // namespace pp
