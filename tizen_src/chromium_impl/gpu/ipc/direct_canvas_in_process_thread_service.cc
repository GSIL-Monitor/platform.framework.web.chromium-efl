// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/direct_canvas_in_process_thread_service.h"

#include "base/lazy_instance.h"
#include "base/threading/thread_task_runner_handle.h"

namespace gpu {

DirectCanvasInProcessThreadService::DirectCanvasInProcessThreadService(
    gpu::SyncPointManager* sync_point_manager,
    gpu::gles2::MailboxManager* mailbox_manager,
    scoped_refptr<gl::GLShareGroup> share_group,
    const GpuFeatureInfo& gpu_feature_info)
    : gpu::InProcessCommandBuffer::Service(GpuPreferences(),
                                           mailbox_manager,
                                           share_group,
                                           gpu_feature_info),
      sync_point_manager_(sync_point_manager) {}

void DirectCanvasInProcessThreadService::ScheduleTask(
    const base::Closure& task) {
  task.Run();
}

void DirectCanvasInProcessThreadService::ScheduleDelayedWork(
    const base::Closure& task) {
  NOTREACHED();
}
bool DirectCanvasInProcessThreadService::UseVirtualizedGLContexts() {
  return false;
}

gpu::SyncPointManager*
DirectCanvasInProcessThreadService::sync_point_manager() {
  return sync_point_manager_;
}

void DirectCanvasInProcessThreadService::AddRef() const {
  base::RefCountedThreadSafe<DirectCanvasInProcessThreadService>::AddRef();
}

void DirectCanvasInProcessThreadService::Release() const {
  base::RefCountedThreadSafe<DirectCanvasInProcessThreadService>::Release();
}

bool DirectCanvasInProcessThreadService::BlockThreadOnWaitSyncToken() const {
  return false;
}

DirectCanvasInProcessThreadService::~DirectCanvasInProcessThreadService() {}

}  // namespace gpu
