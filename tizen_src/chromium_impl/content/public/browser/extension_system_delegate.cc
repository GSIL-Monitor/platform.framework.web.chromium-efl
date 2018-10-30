// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/extension_system_delegate.h"

#include "base/hash.h"
#include "base/memory/singleton.h"

namespace content {

ExtensionSystemDelegateManager::~ExtensionSystemDelegateManager() = default;

ExtensionSystemDelegateManager* ExtensionSystemDelegateManager::GetInstance() {
  return base::Singleton<ExtensionSystemDelegateManager>::get();
}

ExtensionSystemDelegate* ExtensionSystemDelegateManager::GetDelegateForFrame(
    const ExtensionSystemDelegateManager::RenderFrameID& id) {
  auto found_it = delegates_.find(id);
  if (found_it != delegates_.end()) {
    return found_it->second.get();
  }

  // Find delegate with lowest frame_id for given pid.
  // Such situation might happen when plugin is loaded in <iframe> element
  for (auto it = delegates_.begin(); it != delegates_.end(); ++it) {
    if (it->first.render_process_id != id.render_process_id)
      continue;

    if (found_it == delegates_.end() ||
        it->first.render_frame_id < found_it->first.render_frame_id) {
      found_it = it;
    }
  }

  if (found_it != delegates_.end())
    return found_it->second.get();

  return nullptr;
}

void ExtensionSystemDelegateManager::RegisterDelegate(
    const ExtensionSystemDelegateManager::RenderFrameID& id,
    std::unique_ptr<ExtensionSystemDelegate> delegate) {
  delegates_[id] = std::move(delegate);
}

bool ExtensionSystemDelegateManager::UnregisterDelegate(
    const ExtensionSystemDelegateManager::RenderFrameID& id) {
  return delegates_.erase(id) > 0;
}

bool ExtensionSystemDelegateManager::RenderFrameID::operator==(
    const ExtensionSystemDelegateManager::RenderFrameID& rhs) const {
  return render_process_id == rhs.render_process_id &&
         render_frame_id == rhs.render_frame_id;
}

std::size_t ExtensionSystemDelegateManager::RenderFrameIDHasher::operator()(
    const ExtensionSystemDelegateManager::RenderFrameID& k) const {
  return base::HashInts32(k.render_process_id, k.render_frame_id);
}

ExtensionSystemDelegateManager::ExtensionSystemDelegateManager() = default;

}  // namespace content
