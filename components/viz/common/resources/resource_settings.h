// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_RESOURCES_RESOURCE_SETTINGS_H_
#define COMPONENTS_VIZ_COMMON_RESOURCES_RESOURCE_SETTINGS_H_

#include "components/viz/common/resources/buffer_to_texture_target_map.h"
#include "components/viz/common/viz_common_export.h"

namespace viz {

// ResourceSettings contains all the settings that are needed to create a
// ResourceProvider.
class VIZ_COMMON_EXPORT ResourceSettings {
 public:
  ResourceSettings();
  ResourceSettings(const ResourceSettings& other);
  ResourceSettings& operator=(const ResourceSettings& other);
  ~ResourceSettings();

  size_t texture_id_allocation_chunk_size = 64;
  bool use_gpu_memory_buffer_resources = false;
  bool high_bit_for_testing = false;
  // TODO(riju): Remove after r16 is used without the flag. crbug.com/759456
  bool use_r16_texture = false;
  BufferToTextureTargetMap buffer_to_texture_target_map;
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_RESOURCES_RESOURCE_SETTINGS_H_
