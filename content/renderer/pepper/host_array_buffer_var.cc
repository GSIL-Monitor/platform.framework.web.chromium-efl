// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/pepper/host_array_buffer_var.h"

#include <stdio.h>
#include <string.h>

#include <memory>

#include "base/logging.h"
#include "base/memory/shared_memory.h"
#include "base/process/process_handle.h"
#include "content/common/pepper_file_util.h"
#include "content/renderer/pepper/host_globals.h"
#include "content/renderer/pepper/plugin_module.h"
#include "content/renderer/pepper/renderer_ppapi_host_impl.h"
#include "content/renderer/render_thread_impl.h"
#include "ppapi/c/pp_instance.h"

using ppapi::ArrayBufferVar;
using blink::WebArrayBuffer;
using blink::WebArrayBufferView;

namespace content {

namespace {

bool CopyVarToNewShmem(PP_Instance instance,
                       ppapi::ArrayBufferVar* var,
                       int* host_shm_handle_id,
                       base::SharedMemoryHandle* plugin_shm_handle) {
  std::unique_ptr<base::SharedMemory> shm(
      RenderThread::Get()
          ->HostAllocateSharedMemoryBuffer(var->ByteLength())
          .release());
  if (!shm)
    return false;

  shm->Map(var->ByteLength());
  memcpy(shm->memory(), var->Map(), var->ByteLength());
  shm->Unmap();

  // Duplicate the handle here; the SharedMemory destructor closes
  // its handle on us.
  HostGlobals* hg = HostGlobals::Get();
  PluginModule* pm = hg->GetModule(hg->GetModuleForInstance(instance));

  *plugin_shm_handle =
      pm->renderer_ppapi_host()->ShareSharedMemoryHandleWithRemote(
          shm->handle());
  *host_shm_handle_id = -1;
  return true;
}

}  // namespace

HostArrayBufferVar::HostArrayBufferVar(uint32_t size_in_bytes)
    : buffer_(WebArrayBuffer::Create(size_in_bytes, 1 /* element_size */)),
      valid_(true) {}

HostArrayBufferVar::HostArrayBufferVar(const WebArrayBuffer& buffer)
    : buffer_(buffer), valid_(true) {}

HostArrayBufferVar::HostArrayBufferVar(uint32_t size_in_bytes,
                                       base::SharedMemoryHandle handle)
    : buffer_(WebArrayBuffer::Create(size_in_bytes, 1 /* element_size */)) {
  base::SharedMemory s(handle, true);
  valid_ = s.Map(size_in_bytes);
  if (valid_) {
    memcpy(buffer_.Data(), s.memory(), size_in_bytes);
    s.Unmap();
  }
}

HostArrayBufferVar::~HostArrayBufferVar() {}

void* HostArrayBufferVar::Map() {
  if (!valid_)
    return NULL;
  return buffer_.Data();
}

void HostArrayBufferVar::Unmap() {
  // We do not used shared memory on the host side. Nothing to do.
}

uint32_t HostArrayBufferVar::ByteLength() {
  return buffer_.ByteLength();
}

bool HostArrayBufferVar::CopyToNewShmem(
    PP_Instance instance,
    int* host_shm_handle_id,
    base::SharedMemoryHandle* plugin_shm_handle) {
  return CopyVarToNewShmem(instance, this, host_shm_handle_id,
                           plugin_shm_handle);
}

HostArrayBufferViewVar::HostArrayBufferViewVar(const WebArrayBufferView& buffer)
    : buffer_(buffer), valid_(true) {}

HostArrayBufferViewVar::~HostArrayBufferViewVar() {}

void* HostArrayBufferViewVar::Map() {
  if (!valid_)
    return NULL;
  return buffer_.BaseAddress();
}

void HostArrayBufferViewVar::Unmap() {
  // We do not used shared memory on the host side. Nothing to do.
}

uint32_t HostArrayBufferViewVar::ByteLength() {
  return buffer_.ByteLength();
}

bool HostArrayBufferViewVar::CopyToNewShmem(
    PP_Instance instance,
    int* host_shm_handle_id,
    base::SharedMemoryHandle* plugin_shm_handle) {
  return CopyVarToNewShmem(instance, this, host_shm_handle_id,
                           plugin_shm_handle);
}

}  // namespace content
