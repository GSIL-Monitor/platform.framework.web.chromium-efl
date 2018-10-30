// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/render_thread_observer_efl.h"

#include "base/allocator/allocator_extension.h"
#include "base/command_line.h"
#include "common/content_switches_efl.h"
#include "common/render_messages_ewk.h"
#include "content/public/renderer/render_thread.h"
#include "third_party/WebKit/public/platform/WebCache.h"
#include "third_party/sqlite/sqlite3.h"
#include "v8/include/v8.h"
#include "renderer/content_renderer_client_efl.h"
#include "renderer/tizen_extensible.h"

#include "third_party/WebKit/public/platform/WebRuntimeFeatures.h"
// XXX:  config.h needs to be included before internal blink headers.
// XXX2: It'd be great if we did not include internal blibk headers.
#include "third_party/WebKit/Source/platform/fonts/FontCache.h"

using blink::WebCache;
using blink::WebRuntimeFeatures;
using content::RenderThread;

/* LCOV_EXCL_START */
RenderThreadObserverEfl::RenderThreadObserverEfl(
    ContentRendererClientEfl* content_client)
    : content_client_(content_client) {
  const base::CommandLine& command_line = *base::CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(switches::kEnableViewMode))
    WebRuntimeFeatures::EnableCSSViewModeMediaFeature(true);  // LCOV_EXCL_LINE
}

bool RenderThreadObserverEfl::OnControlMessageReceived(const IPC::Message& message)
{
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(RenderThreadObserverEfl, message)
    IPC_MESSAGE_HANDLER(EflViewMsg_ClearCache, OnClearCache)  // LCOV_EXCL_LINE
    IPC_MESSAGE_HANDLER(EflViewMsg_SetCache, OnSetCache)
    IPC_MESSAGE_HANDLER(EwkProcessMsg_SetExtensibleAPI,
                        OnSetExtensibleAPI)  // LCOV_EXCL_LINE
    IPC_MESSAGE_HANDLER(EwkProcessMsg_UpdateTizenExtensible,
                        OnUpdateTizenExtensible)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void RenderThreadObserverEfl::OnClearCache()
{
  WebCache::Clear();
}

void RenderThreadObserverEfl::OnSetCache(const int64_t cache_total_capacity) {
  WebCache::SetCapacity(static_cast<size_t>(cache_total_capacity));
}

void RenderThreadObserverEfl::OnSetExtensibleAPI(const std::string& api_name,
                                                 bool enable) {
  TizenExtensible::GetInstance()->SetExtensibleAPI(api_name, enable);
}

void RenderThreadObserverEfl::OnUpdateTizenExtensible(
    const std::map<std::string, bool>& params) {
  TizenExtensible::GetInstance()->UpdateTizenExtensible(params);
}
/* LCOV_EXCL_STOP */
