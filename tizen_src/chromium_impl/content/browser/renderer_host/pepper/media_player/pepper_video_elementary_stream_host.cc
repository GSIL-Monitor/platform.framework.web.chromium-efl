// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/media_player/pepper_video_elementary_stream_host.h"

#include "content/browser/renderer_host/pepper/browser_host_callback_helpers.h"
#include "content/browser/renderer_host/pepper/media_player/pepper_video_elementary_stream_private.h"
#include "content/public/browser/browser_ppapi_host.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/host/host_message_context.h"
#include "ppapi/host/ppapi_host.h"
#include "ppapi/proxy/dispatch_reply_message.h"
#include "ppapi/proxy/ppapi_messages.h"

namespace content {

std::unique_ptr<PepperVideoElementaryStreamHost>
PepperVideoElementaryStreamHost::Create(
    BrowserPpapiHost* host,
    PP_Instance instance,
    PP_Resource resource,
    scoped_refptr<PepperVideoElementaryStreamPrivate> video_private) {
  return std::make_unique<PepperVideoElementaryStreamHost>(
      host, instance, resource, video_private);
}

PepperVideoElementaryStreamHost::~PepperVideoElementaryStreamHost() {
  private_->RemoveListener(this);
}

int32_t PepperVideoElementaryStreamHost::OnResourceMessageReceived(
    const IPC::Message& msg,
    ppapi::host::HostMessageContext* context) {
  PPAPI_BEGIN_MESSAGE_MAP(PepperVideoElementaryStreamHost, msg)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL(
      PpapiHostMsg_VideoElementaryStream_InitializeDone, OnInitializeDone)
  PPAPI_END_MESSAGE_MAP()
  return PepperElementaryStreamHost::OnResourceMessageReceived(msg, context);
}

PepperVideoElementaryStreamPrivate*
PepperVideoElementaryStreamHost::ElementaryStreamPrivate() {
  return private_.get();
}

PepperVideoElementaryStreamHost::PepperVideoElementaryStreamHost(
    BrowserPpapiHost* host,
    PP_Instance instance,
    PP_Resource resource,
    scoped_refptr<PepperVideoElementaryStreamPrivate> prp_private)
    : PepperElementaryStreamHost(host, instance, resource),
      private_(prp_private) {
  private_->SetListener(ListenerAsWeakPtr());
}

int32_t PepperVideoElementaryStreamHost::OnInitializeDone(
    ppapi::host::HostMessageContext* context,
    PP_StreamInitializationMode mode,
    const ppapi::VideoCodecConfig& config) {
  private_->InitializeDone(
      mode, config,
      base::Bind(&ReplyCallback<
                     PpapiPluginMsg_VideoElementaryStream_InitializeDoneReply>,
                 pp_instance(), context->MakeReplyMessageContext()));
  return PP_OK_COMPLETIONPENDING;
}

}  // namespace content
