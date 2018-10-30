// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_TEEC_SHARED_MEMORY_HOST_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_TEEC_SHARED_MEMORY_HOST_H_

#include <tee_client_api.h>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/renderer_host/pepper/pepper_teec_shared_memory_impl.h"
#include "ppapi/c/samsung/pp_teec_samsung.h"
#include "ppapi/host/resource_host.h"

namespace base {
class SharedMemory;
}

namespace ppapi {
namespace host {
struct ReplyMessageContext;
}  // namespace host

class HostResource;
}  // namespace ppapi

namespace content {
class BrowserPpapiHost;

class PepperTEECSharedMemoryHost : public ppapi::host::ResourceHost {
 public:
  PepperTEECSharedMemoryHost(BrowserPpapiHost* host,
                             PP_Instance instance,
                             PP_Resource resource,
                             PP_Resource context,
                             uint32_t flags);
  ~PepperTEECSharedMemoryHost();

  bool IsTEECSharedMemoryHost() override;
  scoped_refptr<PepperTEECSharedMemoryImpl> GetImpl() const { return impl_; }

 protected:
  // ppapi::host::ResourceHost override.
  int32_t OnResourceMessageReceived(
      const IPC::Message& msg,
      ppapi::host::HostMessageContext* context) override;

 private:
  int32_t OnRegister(ppapi::host::HostMessageContext* context, uint32_t size);
  int32_t OnAllocate(ppapi::host::HostMessageContext* context, uint32_t size);

  scoped_refptr<PepperTEECSharedMemoryImpl> impl_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_TEEC_SHARED_MEMORY_HOST_H_
