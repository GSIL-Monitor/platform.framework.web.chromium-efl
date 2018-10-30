// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RENDER_THREAD_OBSERVER_EFL_H
#define RENDER_THREAD_OBSERVER_EFL_H

#include <map>
#include <string>

#include "base/compiler_specific.h"
#include "content/public/renderer/render_thread_observer.h"

namespace IPC {
class Message;
}

class ContentRendererClientEfl;

/* LCOV_EXCL_START */
class RenderThreadObserverEfl : public content::RenderThreadObserver {
  /* LCOV_EXCL_STOP */
 public:
  explicit RenderThreadObserverEfl(ContentRendererClientEfl* content_client);
  bool OnControlMessageReceived(const IPC::Message& message) override;
  void OnClearCache();

private:
 void OnSetCache(const int64_t cache_total_capacity);
 void OnSetExtensibleAPI(const std::string& api_name, bool enable);
 void OnUpdateTizenExtensible(const std::map<std::string, bool>& params);
 ContentRendererClientEfl* content_client_;
};

#endif
