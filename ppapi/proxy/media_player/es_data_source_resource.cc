// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/media_player/es_data_source_resource.h"

#include <memory>
#include <utility>
#include "base/logging.h"
#include "ppapi/proxy/media_player/audio_elementary_stream_resource.h"
#include "ppapi/proxy/media_player/video_elementary_stream_resource.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/proxy/resource_callback_helpers.h"
#include "ppapi/shared_impl/ppapi_globals.h"
#include "ppapi/shared_impl/resource_tracker.h"

namespace ppapi {
namespace proxy {

ESDataSourceResource::ESDataSourceResource(Connection connection,
                                           PP_Instance instance)
    : MediaDataSourceResource(connection, instance) {
  SendCreate(BROWSER, PpapiHostMsg_ESDataSource_Create());
}

ESDataSourceResource::~ESDataSourceResource() {
  for (int i = 0; i < PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_MAX; ++i)
    PostAbortIfNecessary(add_stream_params_[i].callback);

  PostAbortIfNecessary(set_duration_callback_);
  PostAbortIfNecessary(set_end_of_stream_callback_);
}

thunk::PPB_ESDataSource_API* ESDataSourceResource::AsPPB_ESDataSource_API() {
  return this;
}

int32_t ESDataSourceResource::AddStream(
    PP_ElementaryStream_Type_Samsung type,
    const PPP_ElementaryStreamListener_Samsung_1_0* listener,
    void* user_data,
    PP_Resource* out_resource,
    scoped_refptr<TrackedCallback> callback) {
  if (type <= PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_UNKNOWN ||
      type >= PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_MAX) {
    LOG(ERROR) << "Incorrect elementary stream type = "
               << static_cast<int>(type);
    return PP_ERROR_BADARGUMENT;
  }

  if (!out_resource || !listener) {
    LOG(ERROR) << "Invalid PP_Resource or"
                  " PPP_ElementaryStreamListener_Samsung_1_0";
    return PP_ERROR_BADARGUMENT;
  }

  if (!PpapiGlobals::Get()->GetCurrentMessageLoop()) {
    LOG(ERROR) << "Cannot get current message loop";
    return PP_ERROR_NO_MESSAGE_LOOP;
  }

  if (TrackedCallback::IsPending(add_stream_params_[type].callback)) {
    LOG(ERROR) << "AddStream is already in progress";
    return PP_ERROR_INPROGRESS;
  }

  if (streams_[type]) {
    *out_resource = streams_[type]->GetReference();
    return PP_OK;
  }

  add_stream_params_[type].callback = callback;
  add_stream_params_[type].listener.reset(
      new ElementaryStreamListener(listener, user_data));
  add_stream_params_[type].out_resource = out_resource;

  Call<PpapiPluginMsg_ESDataSource_AddStreamReply>(
      BROWSER, PpapiHostMsg_ESDataSource_AddStream(type),
      base::Bind(&ESDataSourceResource::OnAddStreamReply,
                 base::Unretained(this)));
  return PP_OK_COMPLETIONPENDING;
}

int32_t ESDataSourceResource::SetDuration(
    PP_TimeDelta duration,
    scoped_refptr<TrackedCallback> callback) {
  if (TrackedCallback::IsPending(set_duration_callback_)) {
    LOG(ERROR) << "SetDuration is already in progress";
    return PP_ERROR_INPROGRESS;
  }

  set_duration_callback_ = callback;
  Call<PpapiPluginMsg_ESDataSource_SetDurationReply>(
      BROWSER, PpapiHostMsg_ESDataSource_SetDuration(duration),
      base::Bind(&OnReply,
                 scoped_refptr<TrackedCallback>(set_duration_callback_)));
  return PP_OK_COMPLETIONPENDING;
}

int32_t ESDataSourceResource::SetEndOfStream(
    scoped_refptr<TrackedCallback> callback) {
  if (TrackedCallback::IsPending(set_end_of_stream_callback_)) {
    LOG(ERROR) << "SetEndOfStream is already in progress";
    return PP_ERROR_INPROGRESS;
  }

  set_end_of_stream_callback_ = callback;
  Call<PpapiPluginMsg_ESDataSource_SetEndOfStreamReply>(
      BROWSER, PpapiHostMsg_ESDataSource_SetEndOfStream(),
      base::Bind(&OnReply,
                 scoped_refptr<TrackedCallback>(set_end_of_stream_callback_)));
  return PP_OK_COMPLETIONPENDING;
}

void ESDataSourceResource::OnAddStreamReply(
    const ResourceMessageReplyParams& params,
    PP_ElementaryStream_Type_Samsung type,
    int pending_host_id) {
  if (!TrackedCallback::IsPending(add_stream_params_[type].callback)) {
    LOG(WARNING) << "No callback pending for this reply";
    return;
  }

  *add_stream_params_[type].out_resource = 0;
  if (params.result() == PP_OK) {
    scoped_refptr<ElementaryStreamResourceBase> stream;
    switch (type) {
      case PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_VIDEO:
        stream = VideoElementaryStreamResource::Create(
            connection(), pp_instance(), pending_host_id);
        break;
      case PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_AUDIO:
        stream = AudioElementaryStreamResource::Create(
            connection(), pp_instance(), pending_host_id);
        break;
      default:
        LOG(WARNING) << "Unknown stream type = " << static_cast<int>(type);
        return;
    }
    stream->SetListener(std::move(add_stream_params_[type].listener));
    *add_stream_params_[type].out_resource = stream->GetReference();
    streams_[type] = std::move(stream);
  }

  add_stream_params_[type].callback->Run(params.result());
  add_stream_params_[type] = AddStreamParams();
}

}  // namespace proxy
}  // namespace ppapi
