// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/media_player/pepper_url_data_source_host.h"

#include "base/logging.h"
#include "content/browser/renderer_host/pepper/browser_host_callback_helpers.h"
#include "content/browser/renderer_host/pepper/media_player/pepper_media_player_private_factory.h"
#include "content/browser/renderer_host/pepper/media_player/pepper_url_helper.h"
#include "content/public/browser/browser_ppapi_host.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/host/host_message_context.h"
#include "ppapi/host/ppapi_host.h"
#include "ppapi/proxy/dispatch_reply_message.h"
#include "ppapi/proxy/host_dispatcher.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/shared_impl/ppapi_globals.h"
#include "ppapi/shared_impl/var_tracker.h"

namespace content {

PepperURLDataSourceHost::PepperURLDataSourceHost(BrowserPpapiHost* host,
                                                 PP_Instance instance,
                                                 PP_Resource resource,
                                                 const std::string& url)
    : PepperMediaDataSourceHost(host, instance, resource) {
  std::string resolved_url =
      PepperUrlHelper::ResolveURL(host, pp_instance(), url).spec();
  private_ = PepperMediaPlayerPrivateFactory::GetInstance().CreateURLDataSource(
      resolved_url);
}

PepperURLDataSourceHost::~PepperURLDataSourceHost() {}

PepperURLDataSourcePrivate* PepperURLDataSourceHost::MediaDataSourcePrivate() {
  return private_.get();
}

int32_t PepperURLDataSourceHost::OnResourceMessageReceived(
    const IPC::Message& msg,
    ppapi::host::HostMessageContext* context) {
  PPAPI_BEGIN_MESSAGE_MAP(PepperURLDataSourceHost, msg)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL(
      PpapiHostMsg_URLDataSource_SetStreamingProperty, OnSetStreamingProperty)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL(
      PpapiHostMsg_URLDataSource_GetStreamingProperty, OnGetStreamingProperty)
  PPAPI_END_MESSAGE_MAP()
  LOG(ERROR) << "Resource message unresolved";
  return PP_ERROR_FAILED;
}

int32_t PepperURLDataSourceHost::OnSetStreamingProperty(
    ppapi::host::HostMessageContext* context,
    PP_StreamingProperty property,
    std::string value) {
  if (!private_) {
    LOG(ERROR) << "URLDataSource implementation is not set";
    return PP_ERROR_FAILED;
  }

  private_->SetStreamingProperty(
      property, value,
      base::Bind(&ReplyCallback<
                     PpapiPluginMsg_URLDataSource_SetStreamingPropertyReply>,
                 pp_instance(), context->MakeReplyMessageContext()));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperURLDataSourceHost::OnGetStreamingProperty(
    ppapi::host::HostMessageContext* context,
    PP_StreamingProperty property) {
  if (!private_) {
    LOG(ERROR) << "URLDataSource implementation is not set";
    return PP_ERROR_FAILED;
  }

  private_->GetStreamingProperty(
      property,
      base::Bind(&PepperURLDataSourceHost::OnGetStreamingPropertyCompleted,
                 base::Unretained(this), context->MakeReplyMessageContext()));

  return PP_OK_COMPLETIONPENDING;
}

void PepperURLDataSourceHost::OnGetStreamingPropertyCompleted(
    ppapi::host::ReplyMessageContext reply_context,
    int32_t result,
    const std::string& value) {
  reply_context.params.set_result(result);
  host()->SendReply(
      reply_context,
      PpapiPluginMsg_URLDataSource_GetStreamingPropertyReply(value));
}

}  // namespace content
