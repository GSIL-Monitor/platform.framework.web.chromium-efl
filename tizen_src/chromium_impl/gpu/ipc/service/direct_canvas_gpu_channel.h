// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_IPC_SERVICE_DIRECT_CANVAS_GPU_CHANNEL_H_
#define GPU_IPC_SERVICE_DIRECT_CANVAS_GPU_CHANNEL_H_

#include <memory>
#include <unordered_map>

#include "base/memory/shared_memory_handle.h"
#include "base/memory/weak_ptr.h"
#include "ipc/ipc_listener.h"
#include "ipc/ipc_sender.h"

struct GPUCreateCommandBufferConfig;

namespace base {
class SingleThreadTaskRunner;
}  // namespace base

namespace gpu {

struct Capabilities;
class DirectCanvasCommandBufferStub;
class DirectCanvasGpuChannelFilter;
class GpuChannel;

class DirectCanvasGpuChannel : public IPC::Listener, public IPC::Sender {
 public:
  DirectCanvasGpuChannel(gpu::GpuChannel* channel);
  ~DirectCanvasGpuChannel() override;

  // IPC::Sender implementation:
  bool Send(IPC::Message* msg) override;

  bool OnControlMessageReceived(const IPC::Message& msg);

  bool RouteMessage(const IPC::Message& msg);

  void SwapBuffers(int32_t route_id);

 private:
  // IPC::Listener implementation:
  bool OnMessageReceived(const IPC::Message& message) override;

  // Message handlers for control messages.
  void OnCreateCommandBuffer(const GPUCreateCommandBufferConfig& init_params,
                             int32_t route_id,
                             base::SharedMemoryHandle shared_state_handle,
                             bool* result,
                             gpu::Capabilities* capabilities);
  void OnDestroyCommandBuffer(int32_t route_id);

  bool CreateCommandBuffer(const GPUCreateCommandBufferConfig& init_params,
                           int32_t route_id,
                           base::SharedMemoryHandle shared_state_handle,
                           gpu::Capabilities* capabilities);
  void DestroyCommandBuffer(int32_t route_id);

  DirectCanvasCommandBufferStub* LookupCommandBuffer(int32_t route_id);

  gpu::GpuChannel* const channel_;
  scoped_refptr<DirectCanvasGpuChannelFilter> filter_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> gpu_task_runner_;

  std::unordered_map<int32_t, scoped_refptr<DirectCanvasCommandBufferStub>>
      stubs_;

  base::WeakPtrFactory<DirectCanvasGpuChannel> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(DirectCanvasGpuChannel);
};

}  // namespace gpu

#endif  // GPU_IPC_SERVICE_DIRECT_CANVAS_GPU_CHANNEL_H_
