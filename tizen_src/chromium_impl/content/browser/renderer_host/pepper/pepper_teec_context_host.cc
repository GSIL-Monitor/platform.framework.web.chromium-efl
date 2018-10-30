// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/pepper_teec_context_host.h"

#include "content/public/browser/browser_ppapi_host.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/samsung/pp_teec_samsung.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/host/resource_host.h"
#include "ppapi/proxy/dispatch_reply_message.h"
#include "ppapi/proxy/ppapi_messages.h"

namespace content {

PepperTEECContextHost::PepperTEECContextHost(BrowserPpapiHost* host,
                                             PP_Instance instance,
                                             PP_Resource resource)
    : ResourceHost(host->GetPpapiHost(), instance, resource),
      impl_(new PepperTEECContextImpl(instance)) {}

PepperTEECContextHost::~PepperTEECContextHost() {}

bool PepperTEECContextHost::IsTEECContextHost() {
  return true;
}

int32_t PepperTEECContextHost::OnResourceMessageReceived(
    const IPC::Message& msg,
    ppapi::host::HostMessageContext* context) {
  PPAPI_BEGIN_MESSAGE_MAP(PepperTEECContextHost, msg)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_TEECContext_Open, OnOpen)
  PPAPI_END_MESSAGE_MAP()
  LOG(ERROR) << "Resource message unresolved";
  return PP_ERROR_FAILED;
}

int32_t PepperTEECContextHost::OnOpen(ppapi::host::HostMessageContext* context,
                                      const std::string& name) {
  return impl_->OnOpen(context, name);
}

}  // namespace content
