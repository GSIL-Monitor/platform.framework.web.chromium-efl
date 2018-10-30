// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/media_player/video_elementary_stream_resource.h"

#include <vector>
#include "base/logging.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/proxy/resource_callback_helpers.h"

namespace ppapi {
namespace proxy {

VideoElementaryStreamResource* VideoElementaryStreamResource::Create(
    Connection connection,
    PP_Instance instance,
    int pending_host_id) {
  VideoElementaryStreamResource* stream =
      new VideoElementaryStreamResource(connection, instance);
  stream->AttachToPendingHost(BROWSER, pending_host_id);
  return stream;
}

VideoElementaryStreamResource::~VideoElementaryStreamResource() {}

thunk::PPB_VideoElementaryStream_API*
VideoElementaryStreamResource::AsPPB_VideoElementaryStream_API() {
  return this;
}

PP_ElementaryStream_Type_Samsung
VideoElementaryStreamResource::GetStreamType() {
  return PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_VIDEO;
}

int32_t VideoElementaryStreamResource::InitializeDone(
    PP_StreamInitializationMode mode,
    scoped_refptr<TrackedCallback> callback) {
  if (TrackedCallback::IsPending(initialize_callback_)) {
    LOG(ERROR) << "Initialization is already in progress";
    return PP_ERROR_INPROGRESS;
  }

  initialize_callback_ = callback;
  Call<PpapiPluginMsg_VideoElementaryStream_InitializeDoneReply>(
      BROWSER, PpapiHostMsg_VideoElementaryStream_InitializeDone(mode, config_),
      base::Bind(&OnReply, callback));
  return PP_OK_COMPLETIONPENDING;
}

PP_VideoCodec_Type_Samsung VideoElementaryStreamResource::GetVideoCodecType() {
  return config_.codec;
}

void VideoElementaryStreamResource::SetVideoCodecType(
    PP_VideoCodec_Type_Samsung codec) {
  config_.codec = codec;
}

PP_VideoCodec_Profile_Samsung
VideoElementaryStreamResource::GetVideoCodecProfile() {
  return config_.profile;
}

void VideoElementaryStreamResource::SetVideoCodecProfile(
    PP_VideoCodec_Profile_Samsung profile) {
  config_.profile = profile;
}

PP_VideoFrame_Format_Samsung
VideoElementaryStreamResource::GetVideoFrameFormat() {
  return config_.frame_format;
}

void VideoElementaryStreamResource::SetVideoFrameFormat(
    PP_VideoFrame_Format_Samsung format) {
  config_.frame_format = format;
}

void VideoElementaryStreamResource::GetVideoFrameSize(PP_Size* out_size) {
  if (!out_size)
    return;

  *out_size = config_.frame_size;
}

void VideoElementaryStreamResource::SetVideoFrameSize(const PP_Size* size) {
  if (!size)
    return;

  config_.frame_size = *size;
}

void VideoElementaryStreamResource::GetFrameRate(uint32_t* numerator,
                                                 uint32_t* denominator) {
  if (!numerator || !denominator)
    return;

  *numerator = config_.frame_rate.first;
  *denominator = config_.frame_rate.second;
}

void VideoElementaryStreamResource::SetFrameRate(uint32_t numerator,
                                                 uint32_t denominator) {
  config_.frame_rate = std::make_pair(numerator, denominator);
}

void VideoElementaryStreamResource::SetCodecExtraData(uint32_t data_size,
                                                      const void* data) {
  config_.extra_data =
      std::vector<uint8_t>(reinterpret_cast<const uint8_t*>(data),
                           reinterpret_cast<const uint8_t*>(data) + data_size);
}

VideoElementaryStreamResource::VideoElementaryStreamResource(
    Connection connection,
    PP_Instance instance)
    : ElementaryStreamResource<thunk::PPB_VideoElementaryStream_API>(connection,
                                                                     instance) {
}

}  // namespace proxy
}  // namespace ppapi
