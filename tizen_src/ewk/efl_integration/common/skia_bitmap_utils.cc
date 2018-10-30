// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "skia_bitmap_utils.h"

namespace skia_bitmap_utils {

/* LCOV_EXCL_START */
bool CopyPixels(void* dst_addr, const void* src_addr, const SkImageInfo& info) {
  if (!dst_addr || !src_addr)
    /* LCOV_EXCL_STOP */
    return false;

  /* LCOV_EXCL_START */
  size_t row_bytes = info.bytesPerPixel() * info.width();
  for (int y = 0; y < info.height(); ++y) {
    memcpy(dst_addr, src_addr, row_bytes);
    src_addr = static_cast<const char*>(src_addr) + row_bytes;
    dst_addr = static_cast<char*>(dst_addr) + row_bytes;
    /* LCOV_EXCL_STOP */
  }

  return true;
}

}  // namespace skia_bitmap_utils
