// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/key_systems_tizen.h"

#include <emeCDM/IEME.h>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "renderer/tizen_key_system_properties.h"

namespace efl_integration {

void TizenAddKeySystems(
    std::vector<std::unique_ptr<media::KeySystemProperties>>*
        key_systems_info) {
  eme::supportedCDMList supportedKeySystems;
  eme::IEME::enumerateMediaKeySystems(&supportedKeySystems);
  for (const auto& keySystem : supportedKeySystems) {
    LOG(INFO) << "Adding key system " << keySystem;
    key_systems_info->push_back(
        base::MakeUnique<media::TizenPlatformKeySystemProperties>(keySystem));
  }
}

}  // namespace efl_integration
