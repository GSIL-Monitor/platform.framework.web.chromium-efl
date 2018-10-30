// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/service/direct_canvas_command_buffer_stub.h"

#include "base/memory/shared_memory.h"
#include "gpu/command_buffer/common/command_buffer_shared.h"
#include "gpu/command_buffer/service/command_buffer_service.h"
#include "gpu/command_buffer/service/gles2_cmd_decoder.h"
#include "gpu/ipc/common/gpu_messages.h"
#include "gpu/ipc/in_process_command_buffer.h"
#include "gpu/ipc/service/direct_canvas_gpu_channel.h"

namespace gpu {

DirectCanvasCommandBufferStub::DirectCanvasCommandBufferStub(
    DirectCanvasGpuChannel* channel,
    int32_t route_id)
    : InProcessCommandBuffer(nullptr), channel_(channel), route_id_(route_id) {}

DirectCanvasCommandBufferStub::~DirectCanvasCommandBufferStub() {}

bool DirectCanvasCommandBufferStub::Send(IPC::Message* msg) {
  return channel_->Send(msg);
}

void DirectCanvasCommandBufferStub::SetSharedStateBuffer(
    std::unique_ptr<base::SharedMemory> shared_state_shm,
    gpu::Capabilities* capabilities) {
  const size_t kSharedStateSize = sizeof(CommandBufferSharedState);
  if (!shared_state_shm->Map(kSharedStateSize)) {
    DLOG(ERROR) << "Failed to map shared state buffer.";
  }
  command_buffer()->SetSharedStateBuffer(MakeBackingFromSharedMemory(
      std::move(shared_state_shm), kSharedStateSize));
  *capabilities = decoder()->GetCapabilities();
}

void DirectCanvasCommandBufferStub::HandleMessage(const IPC::Message& message) {
  IPC_BEGIN_MESSAGE_MAP(DirectCanvasCommandBufferStub, message)
    IPC_MESSAGE_HANDLER(GpuCommandBufferMsg_SetGetBuffer, OnSetGetBuffer);
    IPC_MESSAGE_HANDLER(GpuCommandBufferMsg_TakeFrontBuffer, OnTakeFrontBuffer);
    IPC_MESSAGE_HANDLER(GpuCommandBufferMsg_ReturnFrontBuffer,
                        OnReturnFrontBuffer);
    IPC_MESSAGE_HANDLER_DELAY_REPLY(GpuCommandBufferMsg_WaitForTokenInRange,
                                    OnWaitForTokenInRange);
    IPC_MESSAGE_HANDLER_DELAY_REPLY(GpuCommandBufferMsg_WaitForGetOffsetInRange,
                                    OnWaitForGetOffsetInRange);
    IPC_MESSAGE_HANDLER(GpuCommandBufferMsg_AsyncFlush, OnAsyncFlush);
    IPC_MESSAGE_HANDLER(GpuCommandBufferMsg_RegisterTransferBuffer,
                        OnRegisterTransferBuffer);
    IPC_MESSAGE_HANDLER(GpuCommandBufferMsg_DestroyTransferBuffer,
                        OnDestroyTransferBuffer);
    IPC_MESSAGE_HANDLER(GpuCommandBufferMsg_WaitSyncToken, OnWaitSyncToken)
    IPC_MESSAGE_HANDLER(GpuCommandBufferMsg_SignalSyncToken, OnSignalSyncToken)
    IPC_MESSAGE_HANDLER(GpuCommandBufferMsg_SignalQuery, OnSignalQuery)
    IPC_MESSAGE_HANDLER(GpuCommandBufferMsg_CreateImage, OnCreateImage);
    IPC_MESSAGE_HANDLER(GpuCommandBufferMsg_DestroyImage, OnDestroyImage);
    IPC_MESSAGE_HANDLER(GpuCommandBufferMsg_CreateStreamTexture,
                        OnCreateStreamTexture)
    IPC_MESSAGE_UNHANDLED()
  IPC_END_MESSAGE_MAP()
}

void DirectCanvasCommandBufferStub::OnFenceSyncRelease(uint64_t release) {
  InProcessCommandBuffer::OnFenceSyncRelease(release);
  channel_->SwapBuffers(route_id_);
}

bool DirectCanvasCommandBufferStub::SwapBuffers() {
  return (surface()->SwapBuffers() == gfx::SwapResult::SWAP_ACK);
}

void DirectCanvasCommandBufferStub::OnSetGetBuffer(int32_t shm_id) {
  SetGetBuffer(shm_id);
}

void DirectCanvasCommandBufferStub::OnTakeFrontBuffer(const Mailbox& mailbox) {
  NOTIMPLEMENTED();
}

void DirectCanvasCommandBufferStub::OnReturnFrontBuffer(const Mailbox& mailbox,
                                                        bool is_lost) {
  NOTIMPLEMENTED();
}

void DirectCanvasCommandBufferStub::OnWaitForTokenInRange(
    int32_t start,
    int32_t end,
    IPC::Message* reply_message) {
  WaitForTokenInRange(start, end);
  GpuCommandBufferMsg_WaitForTokenInRange::WriteReplyParams(reply_message,
                                                            GetLastState());
  Send(reply_message);
}

void DirectCanvasCommandBufferStub::OnWaitForGetOffsetInRange(
    uint32_t set_get_buffer_count,
    int32_t start,
    int32_t end,
    IPC::Message* reply_message) {
  WaitForGetOffsetInRange(set_get_buffer_count, start, end);
  GpuCommandBufferMsg_WaitForTokenInRange::WriteReplyParams(reply_message,
                                                            GetLastState());
  Send(reply_message);
}
void DirectCanvasCommandBufferStub::OnAsyncFlush(
    int32_t put_offset,
    uint32_t flush_count,
    const std::vector<ui::LatencyInfo>& latency_info) {
  Flush(put_offset);
}
void DirectCanvasCommandBufferStub::OnRegisterTransferBuffer(
    int32_t id,
    base::SharedMemoryHandle transfer_buffer,
    uint32_t size) {
  std::unique_ptr<base::SharedMemory> shared_memory(
      new base::SharedMemory(transfer_buffer, false));
  if (!shared_memory->Map(size)) {
    DVLOG(0) << "Failed to map shared memory.";
    return;
  }

  command_buffer()->RegisterTransferBuffer(
      id, MakeBackingFromSharedMemory(std::move(shared_memory), size));
}

void DirectCanvasCommandBufferStub::OnDestroyTransferBuffer(int32_t id) {
  DestroyTransferBuffer(id);
}

bool DirectCanvasCommandBufferStub::OnWaitSyncToken(
    const SyncToken& sync_token) {
  NOTIMPLEMENTED();
  return false;
}

void DirectCanvasCommandBufferStub::OnSignalSyncToken(
    const SyncToken& sync_token,
    uint32_t id) {
  NOTIMPLEMENTED();
}

void DirectCanvasCommandBufferStub::OnSignalQuery(uint32_t query, uint32_t id) {
  NOTIMPLEMENTED();
}

void DirectCanvasCommandBufferStub::OnCreateImage(
    const GpuCommandBufferMsg_CreateImage_Params& params) {
  NOTIMPLEMENTED();
}
void DirectCanvasCommandBufferStub::OnDestroyImage(int32_t id) {
  NOTIMPLEMENTED();
}

void DirectCanvasCommandBufferStub::OnCreateStreamTexture(uint32_t texture_id,
                                                          int32_t stream_id,
                                                          bool* succeeded) {
  NOTIMPLEMENTED();
}

}  // namespace gpu
