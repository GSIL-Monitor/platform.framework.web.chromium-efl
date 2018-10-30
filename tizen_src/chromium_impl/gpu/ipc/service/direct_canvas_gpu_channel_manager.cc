// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/service/direct_canvas_gpu_channel_manager.h"

#include <memory>
#include <utility>

#include "direct_canvas_gpu_channel.h"
#include "gpu/ipc/in_process_command_buffer.h"
#include "gpu/ipc/service/gpu_channel.h"
#include "gpu/ipc/service/gpu_channel_manager.h"
#include "ipc/ipc_message_macros.h"
#include "ipc/param_traits_macros.h"

namespace gpu {

DirectCanvasGpuChannelManager::DirectCanvasGpuChannelManager(
    gpu::GpuChannelManager* channel_manager)
    : channel_manager_(channel_manager) {}

DirectCanvasGpuChannelManager::~DirectCanvasGpuChannelManager() {
  gpu::InProcessGpuService::ReleaseGpuService();
}

void DirectCanvasGpuChannelManager::AddChannel(int32_t client_id) {
  gpu::GpuChannel* gpu_channel = channel_manager_->LookupChannel(client_id);
  DCHECK(gpu_channel);
  base::UnguessableToken channel_token = base::UnguessableToken::Create();
  std::unique_ptr<DirectCanvasGpuChannel> direct_canvas_gpu_channel(
      new DirectCanvasGpuChannel(gpu_channel));
  direct_canvas_gpu_channels_[client_id] = std::move(direct_canvas_gpu_channel);
  channel_to_token_[client_id] = channel_token;
  token_to_channel_[channel_token] = client_id;
}

void DirectCanvasGpuChannelManager::RemoveChannel(int32_t client_id) {
  direct_canvas_gpu_channels_.erase(client_id);
  const auto it = channel_to_token_.find(client_id);
  if (it != channel_to_token_.end()) {
    token_to_channel_.erase(it->second);
    channel_to_token_.erase(it);
  }
}

void DirectCanvasGpuChannelManager::DestroyAllChannel() {
  direct_canvas_gpu_channels_.clear();
  token_to_channel_.clear();
  channel_to_token_.clear();
}

gpu::GpuChannel* DirectCanvasGpuChannelManager::LookupChannel(
    const base::UnguessableToken& channel_token) {
  const auto it = token_to_channel_.find(channel_token);
  if (it == token_to_channel_.end())
    return nullptr;
  return channel_manager_->LookupChannel(it->second);
}

}  // namespace gpu
