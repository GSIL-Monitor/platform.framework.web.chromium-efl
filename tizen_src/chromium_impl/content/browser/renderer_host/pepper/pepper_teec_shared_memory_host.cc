// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/pepper_teec_shared_memory_host.h"

#include "content/public/browser/browser_ppapi_host.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/host/host_message_context.h"
#include "ppapi/host/ppapi_host.h"
#include "ppapi/proxy/host_dispatcher.h"
#include "ppapi/proxy/ppapi_messages.h"

namespace content {

PepperTEECSharedMemoryHost::PepperTEECSharedMemoryHost(BrowserPpapiHost* host,
                                                       PP_Instance instance,
                                                       PP_Resource resource,
                                                       PP_Resource context,
                                                       uint32_t flags)
    : ResourceHost(host->GetPpapiHost(), instance, resource),
      impl_(new PepperTEECSharedMemoryImpl(host, instance, context, flags)) {}

PepperTEECSharedMemoryHost::~PepperTEECSharedMemoryHost() {}

bool PepperTEECSharedMemoryHost::IsTEECSharedMemoryHost() {
  return true;
}

int32_t PepperTEECSharedMemoryHost::OnResourceMessageReceived(
    const IPC::Message& msg,
    ppapi::host::HostMessageContext* context) {
  PPAPI_BEGIN_MESSAGE_MAP(PepperTEECSharedMemoryHost, msg)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_TEECSharedMemory_Register,
                                    OnRegister)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_TEECSharedMemory_Allocate,
                                    OnAllocate)
  PPAPI_END_MESSAGE_MAP()
  LOG(ERROR) << "Resource message unresolved";
  return PP_ERROR_FAILED;
}

int32_t PepperTEECSharedMemoryHost::OnRegister(
    ppapi::host::HostMessageContext* context,
    uint32_t size) {
  return impl_->OnRegister(context, size);
}

int32_t PepperTEECSharedMemoryHost::OnAllocate(
    ppapi::host::HostMessageContext* context,
    uint32_t size) {
  return impl_->OnAllocate(context, size);
}

}  // namespace content
