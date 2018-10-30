// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TIZEN_SRC_EWK_EFL_INTEGRATION_COMMON_SKIA_BITMAP_UTILS_H
#define TIZEN_SRC_EWK_EFL_INTEGRATION_COMMON_SKIA_BITMAP_UTILS_H

#include "third_party/skia/include/core/SkBitmap.h"

namespace skia_bitmap_utils {

bool CopyPixels(void* dst_addr, const void* src_addr, const SkImageInfo& info);

}  // namespace skia_bitmap_utils

#endif  // TIZEN_SRC_EWK_EFL_INTEGRATION_COMMON_SKIA_BITMAP_UTILS_H
