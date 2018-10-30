// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_IPC_DIRECT_CANVAS_IN_PROCESS_THREAD_SERVICE_H_
#define GPU_IPC_DIRECT_CANVAS_IN_PROCESS_THREAD_SERVICE_H_

#include "base/compiler_specific.h"
#include "base/single_thread_task_runner.h"
#include "gpu/command_buffer/service/mailbox_manager.h"
#include "gpu/gpu_export.h"
#include "gpu/ipc/in_process_command_buffer.h"
#include "ui/gl/gl_share_group.h"

namespace gpu {

class GPU_EXPORT DirectCanvasInProcessThreadService
    : public gpu::InProcessCommandBuffer::Service,
      public base::RefCountedThreadSafe<DirectCanvasInProcessThreadService> {
 public:
  DirectCanvasInProcessThreadService(
      gpu::SyncPointManager* sync_point_manager,
      gpu::gles2::MailboxManager* mailbox_manager,
      scoped_refptr<gl::GLShareGroup> share_group,
      const GpuFeatureInfo& gpu_feature_info);

  // gpu::InProcessCommandBuffer::Service implementation.
  void ScheduleTask(const base::Closure& task) override;
  void ScheduleDelayedWork(const base::Closure& task) override;
  bool UseVirtualizedGLContexts() override;
  gpu::SyncPointManager* sync_point_manager() override;
  void AddRef() const override;
  void Release() const override;
  bool BlockThreadOnWaitSyncToken() const override;

 private:
  friend class base::RefCountedThreadSafe<DirectCanvasInProcessThreadService>;

  ~DirectCanvasInProcessThreadService() override;

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  gpu::SyncPointManager* sync_point_manager_;  // Non-owning.

  DISALLOW_COPY_AND_ASSIGN(DirectCanvasInProcessThreadService);
};

}  // namespace gpu

#endif  // GPU_IPC_DIRECT_CANVAS_IN_PROCESS_THREAD_SERVICE_H_
