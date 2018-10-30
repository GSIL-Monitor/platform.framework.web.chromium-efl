// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "cc/base/switches.h"
#include "content/gpu/gpu_process.h"
#include "content/public/browser/browser_main_runner.h"
#include "content/public/common/content_client.h"
#include "content/public/gpu/content_gpu_client.h"
#include "gpu/command_buffer/client/shared_mailbox_manager.h"
#include "gpu/command_buffer/service/mailbox_manager.h"
#include "gpu/ipc/in_process_command_buffer.h"
#include "gpu/ipc/service/direct_canvas_gpu_channel_manager.h"
#include "gpu/ipc/service/gpu_init.h"
#include "ui/gl/gl_shared_context_efl.h"

#define private public
#include "gpu/ipc/service/gpu_channel_manager.h"
#include "content/gpu/in_process_gpu_thread.h"
#include "content/gpu/gpu_child_thread.h"

// Implementation of InProcessGpuThread and GpuChildThread overrides
// in order to provide on startup shared context and mailbox manager
// created from Efl shared evas gl context.

namespace content {

struct GpuChildThreadEfl : public content::GpuChildThread {
  explicit GpuChildThreadEfl(
      const InProcessChildThreadParams& params,
      std::unique_ptr<gpu::GpuInit> gpu_init)
      : GpuChildThread(params, std::move(gpu_init)) {}

  ~GpuChildThreadEfl() {
#if defined(DIRECT_CANVAS)
    set_direct_canvas_gpu_channel_manager(nullptr);
#endif
  }

  void CreateGpuService(
      viz::mojom::GpuServiceRequest request,
      ui::mojom::GpuHostPtr gpu_host,
      const gpu::GpuPreferences& preferences,
      mojo::ScopedSharedBufferHandle activity_flags) override {
#if defined(TIZEN_VD_DISABLE_GPU)
    return;
#endif
    GpuChildThread::CreateGpuService(std::move(request), std::move(gpu_host),
                                     preferences, std::move(activity_flags));
    gpu_channel_manager()->set_share_group(GLSharedContextEfl::GetShareGroup());
    gpu_channel_manager()->set_mailbox_manager(
        SharedMailboxManager::GetMailboxManager());
#if defined(DIRECT_CANVAS)
    gpu::InProcessGpuService::SetGpuService(
        gpu_channel_manager()->mailbox_manager(),
        gpu_channel_manager()->share_group());
    set_direct_canvas_gpu_channel_manager(
        new gpu::DirectCanvasGpuChannelManager(gpu_channel_manager()));
#endif
  }
};

struct InProcessGpuThreadEfl : public content::InProcessGpuThread {
  explicit InProcessGpuThreadEfl(
      const content::InProcessChildThreadParams& params)
      : InProcessGpuThread(params) {}

  void Init() override {
    gpu_process_ = new content::GpuProcess(base::ThreadPriority::NORMAL);

    auto gpu_init = std::make_unique<gpu::GpuInit>();
    auto* client = GetContentClient()->gpu();
    gpu_init->InitializeInProcess(
        base::CommandLine::ForCurrentProcess(),
        client ? client->GetGPUInfo() : nullptr,
        client ? client->GetGpuFeatureInfo() : nullptr);

    GetContentClient()->SetGpuInfo(gpu_init->gpu_info());

    // Do not create new thread if browser is shutting down.
    if (BrowserMainRunner::ExitedMainMessageLoop()) {
      return;
    }

    // The process object takes ownership of the thread object, so do not
    // save and delete the pointer.
    GpuChildThread* child_thread =
        new GpuChildThreadEfl(params_, std::move(gpu_init));

    // Since we are in the browser process, use the thread start time as the
    // process start time.
    child_thread->Init(base::Time::Now());

    gpu_process_->set_main_thread(child_thread);
  }
};

base::Thread* CreateInProcessGpuThread(
    const content::InProcessChildThreadParams& params) {
  return new InProcessGpuThreadEfl(params);
}

} // namespace content
