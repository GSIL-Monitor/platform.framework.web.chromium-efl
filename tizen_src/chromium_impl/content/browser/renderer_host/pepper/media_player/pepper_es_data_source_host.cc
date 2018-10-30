// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/media_player/pepper_es_data_source_host.h"

#include "base/logging.h"
#include "content/browser/renderer_host/pepper/browser_host_callback_helpers.h"
#include "content/browser/renderer_host/pepper/media_player/pepper_audio_elementary_stream_host.h"
#include "content/browser/renderer_host/pepper/media_player/pepper_es_data_source_private.h"
#include "content/browser/renderer_host/pepper/media_player/pepper_media_player_private_factory.h"
#include "content/browser/renderer_host/pepper/media_player/pepper_video_elementary_stream_host.h"
#include "content/public/browser/browser_ppapi_host.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/host/host_message_context.h"
#include "ppapi/host/ppapi_host.h"
#include "ppapi/proxy/dispatch_reply_message.h"
#include "ppapi/proxy/ppapi_messages.h"

namespace content {

PepperESDataSourceHost::PepperESDataSourceHost(BrowserPpapiHost* host,
                                               PP_Instance instance,
                                               PP_Resource resource)
    : PepperMediaDataSourceHost(host, instance, resource),
      host_(host),
      private_(
          PepperMediaPlayerPrivateFactory::GetInstance().CreateESDataSource()) {
}

PepperESDataSourceHost::~PepperESDataSourceHost() {}

PepperESDataSourcePrivate* PepperESDataSourceHost::MediaDataSourcePrivate() {
  return private_.get();
}

int32_t PepperESDataSourceHost::OnResourceMessageReceived(
    const IPC::Message& msg,
    ppapi::host::HostMessageContext* context) {
  PPAPI_BEGIN_MESSAGE_MAP(PepperESDataSourceHost, msg)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_ESDataSource_AddStream,
                                    OnAddStream)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_ESDataSource_SetDuration,
                                    OnSetDuration)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(PpapiHostMsg_ESDataSource_SetEndOfStream,
                                      OnSetEndOfStream)
  PPAPI_END_MESSAGE_MAP()
  LOG(ERROR) << "Resource message unresolved";
  return PP_ERROR_FAILED;
}

int32_t PepperESDataSourceHost::OnAddStream(
    ppapi::host::HostMessageContext* context,
    PP_ElementaryStream_Type_Samsung type) {
  switch (type) {
    case PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_VIDEO:
      private_->AddVideoStream(
          base::Bind(&PepperESDataSourceHost::OnVideoStreamAdded, AsWeakPtr(),
                     context->MakeReplyMessageContext()));
      break;
    case PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_AUDIO:
      private_->AddAudioStream(
          base::Bind(&PepperESDataSourceHost::OnAudioStreamAdded, AsWeakPtr(),
                     context->MakeReplyMessageContext()));
      break;
    default:
      LOG(ERROR) << "Unsupported elementary stream type = "
                 << static_cast<int>(type);
      return PP_ERROR_BADARGUMENT;
  }

  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperESDataSourceHost::OnSetDuration(
    ppapi::host::HostMessageContext* context,
    PP_TimeDelta duration) {
  private_->SetDuration(
      duration,
      base::Bind(&ReplyCallback<PpapiPluginMsg_ESDataSource_SetDurationReply>,
                 pp_instance(), context->MakeReplyMessageContext()));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperESDataSourceHost::OnSetEndOfStream(
    ppapi::host::HostMessageContext* context) {
  private_->SetEndOfStream(base::Bind(
      &ReplyCallback<PpapiPluginMsg_ESDataSource_SetEndOfStreamReply>,
      pp_instance(), context->MakeReplyMessageContext()));
  return PP_OK_COMPLETIONPENDING;
}

void PepperESDataSourceHost::OnAudioStreamAdded(
    ppapi::host::ReplyMessageContext context,
    int32_t result,
    scoped_refptr<PepperAudioElementaryStreamPrivate> stream) {
  int pending_host_resource_id = 0;
  if (result == PP_OK) {
    auto stream_host = PepperAudioElementaryStreamHost::Create(
        host_, pp_instance(), 0, stream);
    if (stream_host)
      pending_host_resource_id =
          host()->AddPendingResourceHost(std::move(stream_host));
  }

  context.params.set_result(result);
  host()->SendReply(context, PpapiPluginMsg_ESDataSource_AddStreamReply(
                                 PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_AUDIO,
                                 pending_host_resource_id));
}

void PepperESDataSourceHost::OnVideoStreamAdded(
    ppapi::host::ReplyMessageContext context,
    int32_t result,
    scoped_refptr<PepperVideoElementaryStreamPrivate> stream) {
  int pending_host_resource_id = 0;
  if (result == PP_OK) {
    auto stream_host = PepperVideoElementaryStreamHost::Create(
        host_, pp_instance(), 0, stream);
    if (stream_host)
      pending_host_resource_id =
          host()->AddPendingResourceHost(std::move(stream_host));
  }

  context.params.set_result(result);
  host()->SendReply(context, PpapiPluginMsg_ESDataSource_AddStreamReply(
                                 PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_VIDEO,
                                 pending_host_resource_id));
}

}  // namespace content
