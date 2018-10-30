// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/media_player/tizen/platform_context_tizen.h"

namespace content {

PepperMediaDataSourcePrivate::PlatformContext::PlatformContext()
    : dispatcher_(nullptr), drm_manager_(nullptr) {}

PepperMediaDataSourcePrivate::PlatformContext::~PlatformContext() = default;

}  // namespace content
