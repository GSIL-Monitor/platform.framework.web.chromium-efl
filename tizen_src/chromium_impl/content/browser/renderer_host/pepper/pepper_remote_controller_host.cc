// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/pepper_remote_controller_host.h"

#include "base/bind.h"
#include "base/logging.h"
#include "content/public/browser/browser_ppapi_host.h"
#include "content/public/browser/browser_thread.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/host/host_message_context.h"
#include "ppapi/host/ppapi_host.h"
#include "ppapi/proxy/dispatch_reply_message.h"
#include "ppapi/proxy/ppapi_messages.h"

#if defined(OS_TIZEN_TV_PRODUCT)
#include "ewk/efl_integration/common/application_type.h"
#include "content/browser/renderer_host/pepper/remote_controller_wrt.h"
#endif

namespace content {

PepperRemoteControllerHost::PepperRemoteControllerHost(BrowserPpapiHost* host,
                                                       PP_Instance instance,
                                                       PP_Resource resource)
    : ResourceHost(host->GetPpapiHost(), instance, resource), host_(host) {}

PepperRemoteControllerHost::~PepperRemoteControllerHost() = default;

int32_t PepperRemoteControllerHost::OnResourceMessageReceived(
    const IPC::Message& msg,
    ppapi::host::HostMessageContext* context) {
  PPAPI_BEGIN_MESSAGE_MAP(PepperRemoteControllerHost, msg)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_RemoteController_RegisterKeys,
                                    OnHostMsgRegisterKeys)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL(
      PpapiHostMsg_RemoteController_UnRegisterKeys, OnHostMsgUnRegisterKeys)
  PPAPI_END_MESSAGE_MAP()
  LOG(ERROR) << "Resource message unresolved";
  return PP_ERROR_FAILED;
}

int32_t PepperRemoteControllerHost::OnHostMsgRegisterKeys(
    ppapi::host::HostMessageContext* context,
    const std::vector<std::string>& keys) {
  auto delegate = GetPlatformDelegate();
  if (!delegate) {
    LOG(ERROR) << "Invalid delegate";
    return PP_ERROR_NOTSUPPORTED;
  }

  auto cb = base::Bind(&PepperRemoteControllerHost::DidRegisterKeys,
                       AsWeakPtr(), context->MakeReplyMessageContext());
  delegate->RegisterKeys(keys, cb);
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperRemoteControllerHost::OnHostMsgUnRegisterKeys(
    ppapi::host::HostMessageContext* context,
    const std::vector<std::string>& keys) {
  auto delegate = GetPlatformDelegate();
  if (!delegate) {
    LOG(ERROR) << "Invalid delegate";
    return PP_ERROR_NOTSUPPORTED;
  }

  auto cb = base::Bind(&PepperRemoteControllerHost::DidUnRegisterKeys,
                       AsWeakPtr(), context->MakeReplyMessageContext());
  delegate->UnRegisterKeys(keys, cb);
  return PP_OK_COMPLETIONPENDING;
}

void PepperRemoteControllerHost::DidRegisterKeys(
    ppapi::host::ReplyMessageContext reply_context,
    int32_t result) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  reply_context.params.set_result(result);
  host()->SendReply(reply_context,
                    PpapiHostMsg_RemoteController_RegisterKeysReply());
}

void PepperRemoteControllerHost::DidUnRegisterKeys(
    ppapi::host::ReplyMessageContext reply_context,
    int32_t result) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  reply_context.params.set_result(result);
  host()->SendReply(reply_context,
                    PpapiHostMsg_RemoteController_UnRegisterKeysReply());
}

PepperRemoteControllerHost::PlatformDelegate*
PepperRemoteControllerHost::GetPlatformDelegate() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (delegate_)
    return delegate_.get();
#if defined(OS_TIZEN_TV_PRODUCT)
  if (IsTIZENWRT())
    delegate_ = CreateRemoteControllerWRT(pp_instance(), host_);
#endif
  return delegate_.get();
}

}  // namespace content
