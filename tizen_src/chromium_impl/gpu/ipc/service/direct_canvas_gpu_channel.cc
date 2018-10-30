// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/service/direct_canvas_gpu_channel.h"

#include "base/memory/shared_memory.h"
#include "base/single_thread_task_runner.h"
#include "gpu/ipc/common/gpu_messages.h"
#include "gpu/ipc/service/direct_canvas_command_buffer_stub.h"
#include "gpu/ipc/service/gpu_channel.h"
#include "ipc/message_filter.h"

namespace {
template <typename T>
static void RunTaskWithResult(base::Callback<T(void)> task,
                              T* result,
                              base::WaitableEvent* completion) {
  *result = task.Run();
  completion->Signal();
}
}  // namespace

namespace gpu {

// The filter to respond to GetChannelToken on the IO thread.
class DirectCanvasGpuChannelFilter : public IPC::MessageFilter {
 public:
  explicit DirectCanvasGpuChannelFilter(
      DirectCanvasGpuChannel* direct_canvas_channel)
      : channel_(nullptr), direct_canvas_channel_(direct_canvas_channel) {}

  void OnFilterAdded(IPC::Channel* channel) override { channel_ = channel; }
  bool Send(IPC::Message* msg) { return channel_->Send(msg); }

  bool OnMessageReceived(const IPC::Message& msg) override {
    bool handled = false;
    if (IsDirectCanvasChannelValid()) {
      if (msg.routing_id() == MSG_ROUTING_CONTROL)
        handled = direct_canvas_channel_->OnControlMessageReceived(msg);
      else
        handled = direct_canvas_channel_->RouteMessage(msg);
    }
    return handled;
  }

  void InvalidateDirectCanvasChannel() { direct_canvas_channel_ = nullptr; }

  bool IsDirectCanvasChannelValid() {
    if (direct_canvas_channel_)
      return true;

    return false;
  }

 private:
  ~DirectCanvasGpuChannelFilter() override {}

  IPC::Channel* channel_;
  DirectCanvasGpuChannel* direct_canvas_channel_;
};

void DirectCanvasGpuChannel::SwapBuffers(int32_t route_id) {
  DirectCanvasCommandBufferStub* stub = LookupCommandBuffer(route_id);
  if (!stub) {
    LOG(ERROR) << "Can't find a stub";
    return;
  }
  stub->SwapBuffers();
}

static void ReleaseCommandBufferStub(
    scoped_refptr<DirectCanvasCommandBufferStub> stub) {}

DirectCanvasGpuChannel::DirectCanvasGpuChannel(gpu::GpuChannel* channel)
    : channel_(channel),
      task_runner_(gpu::InProcessGpuService::GetMainThread()),
      gpu_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      weak_factory_(this) {
  filter_ = new DirectCanvasGpuChannelFilter(this);
  channel_->AddFilter(filter_.get());
}

DirectCanvasGpuChannel::~DirectCanvasGpuChannel() {
  for (auto& stub : stubs_) {
    task_runner_->PostTask(FROM_HERE,
                           base::Bind(&ReleaseCommandBufferStub, stub.second));
  }
  filter_->InvalidateDirectCanvasChannel();
}

bool DirectCanvasGpuChannel::Send(IPC::Message* msg) {
  return channel_->Send(msg);
}

bool DirectCanvasGpuChannel::OnMessageReceived(const IPC::Message& message) {
  return false;
}

bool DirectCanvasGpuChannel::OnControlMessageReceived(const IPC::Message& msg) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(DirectCanvasGpuChannel, msg)
    IPC_MESSAGE_HANDLER(GpuChannelMsg_CreateDirectCanvasCommandBuffer,
                        OnCreateCommandBuffer)
    IPC_MESSAGE_HANDLER(GpuChannelMsg_DestroyDirectCanvasCommandBuffer,
                        OnDestroyCommandBuffer)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

bool DirectCanvasGpuChannel::RouteMessage(const IPC::Message& msg) {
  int32_t routing_id = msg.routing_id();
  DirectCanvasCommandBufferStub* stub = LookupCommandBuffer(routing_id);

  if (stub) {
    task_runner_->PostTask(
        FROM_HERE, base::Bind(&DirectCanvasCommandBufferStub::HandleMessage,
                              stub->AsWeakPtr(), msg));
    return true;
  }
  return false;
}

DirectCanvasCommandBufferStub* DirectCanvasGpuChannel::LookupCommandBuffer(
    int32_t route_id) {
  auto it = stubs_.find(route_id);
  if (it == stubs_.end())
    return nullptr;

  return it->second.get();
}

void DirectCanvasGpuChannel::OnCreateCommandBuffer(
    const GPUCreateCommandBufferConfig& init_params,
    int32_t route_id,
    base::SharedMemoryHandle shared_state_handle,
    bool* result,
    gpu::Capabilities* capabilities) {
  base::Callback<bool(void)> create_task = base::Bind(
      &DirectCanvasGpuChannel::CreateCommandBuffer, base::Unretained(this),
      init_params, route_id, shared_state_handle, capabilities);
  base::WaitableEvent completion(
      base::WaitableEvent::ResetPolicy::MANUAL,
      base::WaitableEvent::InitialState::NOT_SIGNALED);
  bool create_result = false;
  task_runner_->PostTask(FROM_HERE,
                         base::Bind(&RunTaskWithResult<bool>, create_task,
                                    &create_result, &completion));
  completion.Wait();
  *result = create_result;
}

void DirectCanvasGpuChannel::OnDestroyCommandBuffer(int32_t route_id) {
  task_runner_->PostTask(
      FROM_HERE, base::Bind(&DirectCanvasGpuChannel::DestroyCommandBuffer,
                            weak_factory_.GetWeakPtr(), route_id));
}

bool DirectCanvasGpuChannel::CreateCommandBuffer(
    const GPUCreateCommandBufferConfig& init_params,
    int32_t route_id,
    base::SharedMemoryHandle shared_state_handle,
    gpu::Capabilities* capabilities) {
  std::unique_ptr<base::SharedMemory> shared_state_shm(
      new base::SharedMemory(shared_state_handle, false));

  DirectCanvasCommandBufferStub* stub =
      new DirectCanvasCommandBufferStub(this, route_id);
  if (stub->Initialize(nullptr, false, init_params.surface_handle,
                       init_params.attribs, nullptr, nullptr, nullptr,
                       gpu::InProcessGpuService::GetMainThread())) {
    stub->SetSharedStateBuffer(std::move(shared_state_shm), capabilities);
    stubs_[route_id] = stub;
    gpu::InProcessGpuService::SetDirectCanvasEnabled();
    return true;
  }
  *capabilities = gpu::Capabilities();
  return false;
}

void DirectCanvasGpuChannel::DestroyCommandBuffer(int32_t route_id) {
  auto it = stubs_.find(route_id);
  if (it != stubs_.end()) {
    it->second = nullptr;
    stubs_.erase(it);
  }
}

}  // namespace gpu
