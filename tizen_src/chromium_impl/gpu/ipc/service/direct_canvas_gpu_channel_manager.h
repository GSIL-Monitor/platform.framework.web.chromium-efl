// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_IPC_SERVICE_DIRECT_CANVAS_GPU_CHANNEL_MANAGER_H_
#define GPU_IPC_SERVICE_DIRECT_CANVAS_GPU_CHANNEL_MANAGER_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <unordered_map>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/unguessable_token.h"
#include "gpu/gpu_export.h"
#include "ipc/ipc_listener.h"
#include "ipc/ipc_sender.h"

namespace gpu {

class DirectCanvasGpuChannel;
class GpuChannel;
class GpuChannelManager;

class GPU_EXPORT DirectCanvasGpuChannelManager
    : public base::SupportsWeakPtr<DirectCanvasGpuChannelManager> {
 public:
  explicit DirectCanvasGpuChannelManager(
      gpu::GpuChannelManager* channel_manager);
  ~DirectCanvasGpuChannelManager();

  void AddChannel(int32_t client_id);
  void RemoveChannel(int32_t client_id);
  void DestroyAllChannel();

  // TODO(sandersd): Should we expose the MediaGpuChannel instead?
  gpu::GpuChannel* LookupChannel(const base::UnguessableToken& channel_token);

 private:
  gpu::GpuChannelManager* const channel_manager_;
  std::unordered_map<int32_t, std::unique_ptr<DirectCanvasGpuChannel>>
      direct_canvas_gpu_channels_;
  std::map<base::UnguessableToken, int32_t> token_to_channel_;
  std::map<int32_t, base::UnguessableToken> channel_to_token_;
  DISALLOW_COPY_AND_ASSIGN(DirectCanvasGpuChannelManager);
};

}  // namespace gpu

#endif  // GPU_IPC_SERVICE_DIRECT_CANVAS_GPU_CHANNEL_MANAGER_H_
