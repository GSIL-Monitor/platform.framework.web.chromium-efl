// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if defined(S_TERRACE_SUPPORT)

#include "content/public/browser/terrace_content_bridge.h"

namespace content {
namespace {
TerraceContentBridge* g_instance = nullptr;
}

// static
TerraceContentBridge& TerraceContentBridge::Get() {
  CHECK(g_instance);
  return *g_instance;
}

// static
void TerraceContentBridge::SetInstance(TerraceContentBridge* instance) {
  CHECK(!g_instance);
  g_instance = instance;
}

} // namespace content

#endif // S_TERRACE_SUPPORT
