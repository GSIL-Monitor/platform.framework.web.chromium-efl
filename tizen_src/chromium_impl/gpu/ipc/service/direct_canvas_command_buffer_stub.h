// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_IPC_SERVICE_DIRECT_CANVAS_COMMAND_BUFFER_STUB_H_
#define GPU_IPC_SERVICE_DIRECT_CANVAS_COMMAND_BUFFER_STUB_H_
#include <memory>

#include "base/memory/shared_memory_handle.h"
#include "base/memory/weak_ptr.h"
#include "gpu/ipc/in_process_command_buffer.h"
#include "ipc/ipc_sender.h"
#include "ui/latency/latency_info.h"

struct GpuCommandBufferMsg_CreateImage_Params;

namespace base {
class SharedMemory;
}

namespace gpu {

struct Capabilities;
class DirectCanvasGpuChannel;
class GpuChannel;
class InProcessCommandBuffer;
struct Mailbox;
struct SyncToken;

class DirectCanvasCommandBufferStub
    : public InProcessCommandBuffer,
      public IPC::Sender,
      public base::SupportsWeakPtr<DirectCanvasCommandBufferStub>,
      public base::RefCountedThreadSafe<DirectCanvasCommandBufferStub> {
 public:
  DirectCanvasCommandBufferStub(DirectCanvasGpuChannel* channel,
                                int32_t route_id);

  // ipc::Sender implementation:
  bool Send(IPC::Message* msg) override;

  void HandleMessage(const IPC::Message& message);

  void OnFenceSyncRelease(uint64_t release) override;

  bool SwapBuffers();

  int32_t route_id() const { return route_id_; }

  void SetSharedStateBuffer(
      std::unique_ptr<base::SharedMemory> shared_memory_shm,
      gpu::Capabilities* capabilities);

 private:
  friend class base::RefCountedThreadSafe<DirectCanvasCommandBufferStub>;
  ~DirectCanvasCommandBufferStub() override;

  // Message handlers:
  void OnSetGetBuffer(int32_t shm_id);
  void OnTakeFrontBuffer(const Mailbox& mailbox);
  void OnReturnFrontBuffer(const Mailbox& mailbox, bool is_lost);
  void OnWaitForTokenInRange(int32_t start,
                             int32_t end,
                             IPC::Message* reply_message);
  void OnWaitForGetOffsetInRange(uint32_t set_get_buffer_count,
                                 int32_t start,
                                 int32_t end,
                                 IPC::Message* reply_message);
  void OnAsyncFlush(int32_t put_offset,
                    uint32_t flush_count,
                    const std::vector<ui::LatencyInfo>& latency_info);
  void OnRegisterTransferBuffer(int32_t id,
                                base::SharedMemoryHandle transfer_buffer,
                                uint32_t size);
  void OnDestroyTransferBuffer(int32_t id);
  bool OnWaitSyncToken(const SyncToken& sync_token);
  void OnSignalSyncToken(const SyncToken& sync_token, uint32_t id);
  void OnSignalQuery(uint32_t query, uint32_t id);
  void OnCreateImage(const GpuCommandBufferMsg_CreateImage_Params& params);
  void OnDestroyImage(int32_t id);
  void OnCreateStreamTexture(uint32_t texture_id,
                             int32_t stream_id,
                             bool* succeeded);

  // The lifetime of objects of this class is managed by a
  // DirectCanvasGpuChannel. The GpuChannels destroy all the
  // DirectCanvasCommandBufferStub that they own when they are destroyed. So a
  // raw pointer is safe.
  DirectCanvasGpuChannel* const channel_;
  int32_t route_id_;
};

}  // namespace gpu

#endif  // GPU_IPC_SERVICE_DIRECT_CANVAS_COMMAND_BUFFER_STUB_H_
