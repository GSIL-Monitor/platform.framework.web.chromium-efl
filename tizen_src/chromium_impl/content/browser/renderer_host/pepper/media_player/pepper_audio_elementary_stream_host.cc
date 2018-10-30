// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/media_player/pepper_audio_elementary_stream_host.h"

#include <utility>

#include "content/browser/renderer_host/pepper/media_player/pepper_audio_elementary_stream_private.h"
#include "content/public/browser/browser_ppapi_host.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/host/host_message_context.h"
#include "ppapi/host/ppapi_host.h"
#include "ppapi/proxy/dispatch_reply_message.h"
#include "ppapi/proxy/ppapi_messages.h"

namespace content {

std::unique_ptr<PepperAudioElementaryStreamHost>
PepperAudioElementaryStreamHost::Create(
    BrowserPpapiHost* host,
    PP_Instance instance,
    PP_Resource resource,
    scoped_refptr<PepperAudioElementaryStreamPrivate> audio_private) {
  return std::make_unique<PepperAudioElementaryStreamHost>(
      host, instance, resource, audio_private);
}

PepperAudioElementaryStreamHost::~PepperAudioElementaryStreamHost() {
  private_->RemoveListener(this);
}

int32_t PepperAudioElementaryStreamHost::OnResourceMessageReceived(
    const IPC::Message& msg,
    ppapi::host::HostMessageContext* context) {
  PPAPI_BEGIN_MESSAGE_MAP(PepperAudioElementaryStreamHost, msg)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL(
      PpapiHostMsg_AudioElementaryStream_InitializeDone, OnInitializeDone)
  PPAPI_END_MESSAGE_MAP()
  return PepperElementaryStreamHost::OnResourceMessageReceived(msg, context);
}

PepperAudioElementaryStreamPrivate*
PepperAudioElementaryStreamHost::ElementaryStreamPrivate() {
  return private_.get();
}

PepperAudioElementaryStreamHost::PepperAudioElementaryStreamHost(
    BrowserPpapiHost* host,
    PP_Instance instance,
    PP_Resource resource,
    scoped_refptr<PepperAudioElementaryStreamPrivate> prp_private)
    : PepperElementaryStreamHost(host, instance, resource),
      private_(prp_private) {
  private_->SetListener(ListenerAsWeakPtr());
}

int32_t PepperAudioElementaryStreamHost::OnInitializeDone(
    ppapi::host::HostMessageContext* context,
    PP_StreamInitializationMode mode,
    const ppapi::AudioCodecConfig& config) {
  private_->InitializeDone(
      mode, config,
      base::Bind(&ReplyCallback<
                     PpapiPluginMsg_AudioElementaryStream_InitializeDoneReply>,
                 pp_instance(), context->MakeReplyMessageContext()));
  return PP_OK_COMPLETIONPENDING;
}

}  // namespace content
