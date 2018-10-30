// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/filters/source_buffer_platform.h"

namespace media {

// 12MB: approximately 5 minutes of 320Kbps content.
// 80MB: approximately 2m 40s of 4Mbps content.
#if defined(OS_TIZEN_TV_PRODUCT)
const size_t kSourceBufferAudioMemoryLimit = 8 * 1024 * 1024;
const size_t kSourceBufferVideoMemoryLimitForFHD = 40 * 1024 * 1024;
const size_t kSourceBufferVideoMemoryLimitForUHD = 80 * 1024 * 1024;
#else
const size_t kSourceBufferAudioMemoryLimit = 12 * 1024 * 1024;
const size_t kSourceBufferVideoMemoryLimit = 80 * 1024 * 1024;
#endif
}  // namespace media
