// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_EFL_INTEGRATION_RENDERER_KEY_SYSTEMS_TIZEN_H_
#define EWK_EFL_INTEGRATION_RENDERER_KEY_SYSTEMS_TIZEN_H_

#include <memory>
#include <vector>

#include "media/base/key_system_properties.h"

namespace efl_integration {

void TizenAddKeySystems(
    std::vector<std::unique_ptr<media::KeySystemProperties>>* key_systems_info);

}  // namespace efl_integration

#endif  // EWK_EFL_INTEGRATION_RENDERER_KEY_SYSTEMS_TIZEN_H_
