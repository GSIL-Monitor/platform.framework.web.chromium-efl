// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_TEEC_SHARED_MEMORY_IMPL_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_TEEC_SHARED_MEMORY_IMPL_H_

#include <tee_client_api.h>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/samsung/pp_teec_samsung.h"
#include "ppapi/host/resource_host.h"

namespace base {
class SharedMemory;
}

namespace ppapi {
namespace host {
struct HostMessageContext;
struct ReplyMessageContext;
}  // namespace host

class HostResource;
}  // namespace ppapi

namespace content {

class BrowserPpapiHost;
class PepperTEECContextHost;
class PepperTEECContextImpl;
class PepperTEECSharedMemoryHost;

class PepperTEECSharedMemoryImpl
    : public base::RefCountedThreadSafe<PepperTEECSharedMemoryImpl> {
 public:
  PepperTEECSharedMemoryImpl(BrowserPpapiHost* host,
                             PP_Instance instance,
                             PP_Resource context,
                             uint32_t flags);

  TEEC_SharedMemory* GetSharedMemory();
  bool PropagateSharedMemory(PP_TEEC_MemoryType direction,
                             uint32_t offset,
                             uint32_t size);

 private:
  friend class PepperTEECSharedMemoryHost;
  friend class base::RefCountedThreadSafe<PepperTEECSharedMemoryImpl>;

  ~PepperTEECSharedMemoryImpl();

  int32_t OnRegister(ppapi::host::HostMessageContext* context, uint32_t size);
  int32_t OnAllocate(ppapi::host::HostMessageContext* context, uint32_t size);

  PP_TEEC_Result Allocate(TEEC_Context* teec_context);
  bool PrepareBuffer(ppapi::host::ReplyMessageContext* reply_context);

  PP_Instance pp_instance_;
  BrowserPpapiHost* host_;
  uint32_t flags_;
  uint32_t size_;
  base::WeakPtr<PepperTEECContextHost> teec_context_host_;
  scoped_refptr<PepperTEECContextImpl> teec_context_impl_;

  TEEC_SharedMemory teec_shared_memory_;
  std::atomic<bool> teec_shared_memory_initialized_;
  std::unique_ptr<base::SharedMemory> shared_memory_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_TEEC_SHARED_MEMORY_IMPL_H_
