// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/pepper_teec_session_host.h"

#include "content/public/browser/browser_ppapi_host.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/host/ppapi_host.h"
#include "ppapi/proxy/ppapi_messages.h"

namespace content {

PepperTEECSessionHost::PepperTEECSessionHost(BrowserPpapiHost* host,
                                             PP_Instance instance,
                                             PP_Resource resource,
                                             PP_Resource context)
    : ResourceHost(host->GetPpapiHost(), instance, resource),
      impl_(new PepperTEECSessionImpl(host, instance, context)) {}

PepperTEECSessionHost::~PepperTEECSessionHost() {}

bool PepperTEECSessionHost::IsTEECSessionHost() {
  return true;
}

int32_t PepperTEECSessionHost::OnResourceMessageReceived(
    const IPC::Message& msg,
    ppapi::host::HostMessageContext* context) {
  PPAPI_BEGIN_MESSAGE_MAP(PepperTEECSessionHost, msg)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_TEECSession_Open, OnOpen)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_TEECSession_InvokeCommand,
                                    OnInvokeCommand)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL(
      PpapiHostMsg_TEECSession_RequestCancellation, OnRequestCancellation)
  PPAPI_END_MESSAGE_MAP()
  LOG(ERROR) << "Resource message unresolved";
  return PP_ERROR_FAILED;
}

int32_t PepperTEECSessionHost::OnOpen(ppapi::host::HostMessageContext* context,
                                      const PP_TEEC_UUID& uuid,
                                      uint32_t connection_method,
                                      const std::string& connection_data,
                                      const PP_TEEC_Operation& operation) {
  return impl_->OnOpen(context, uuid, connection_method, connection_data,
                       operation);
}

int32_t PepperTEECSessionHost::OnInvokeCommand(
    ppapi::host::HostMessageContext* context,
    uint32_t command_id,
    const PP_TEEC_Operation& operation) {
  return impl_->OnInvokeCommand(context, command_id, operation);
}

int32_t PepperTEECSessionHost::OnRequestCancellation(
    ppapi::host::HostMessageContext* context,
    const PP_TEEC_Operation& operation) {
  return impl_->OnRequestCancellation(context, operation);
}

}  // namespace content
