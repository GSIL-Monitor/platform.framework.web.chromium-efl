// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/media_player/audio_elementary_stream_resource.h"

#include <string>
#include "base/logging.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/proxy/resource_callback_helpers.h"

namespace ppapi {
namespace proxy {

AudioElementaryStreamResource* AudioElementaryStreamResource::Create(
    Connection connection,
    PP_Instance instance,
    int pending_host_id) {
  auto stream = new AudioElementaryStreamResource(connection, instance);
  stream->AttachToPendingHost(BROWSER, pending_host_id);

  return stream;
}

AudioElementaryStreamResource::~AudioElementaryStreamResource() {}

thunk::PPB_AudioElementaryStream_API*
AudioElementaryStreamResource::AsPPB_AudioElementaryStream_API() {
  return this;
}

PP_ElementaryStream_Type_Samsung
AudioElementaryStreamResource::GetStreamType() {
  return PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_AUDIO;
}

int32_t AudioElementaryStreamResource::InitializeDone(
    PP_StreamInitializationMode mode,
    scoped_refptr<TrackedCallback> callback) {
  if (TrackedCallback::IsPending(initialize_callback_)) {
    LOG(ERROR) << "Initialization pending";
    return PP_ERROR_INPROGRESS;
  }

  initialize_callback_ = callback;
  Call<PpapiPluginMsg_AudioElementaryStream_InitializeDoneReply>(
      BROWSER, PpapiHostMsg_AudioElementaryStream_InitializeDone(mode, config_),
      base::Bind(&OnReply, initialize_callback_));
  return PP_OK_COMPLETIONPENDING;
}

PP_AudioCodec_Type_Samsung AudioElementaryStreamResource::GetAudioCodecType() {
  return config_.codec;
}

void AudioElementaryStreamResource::SetAudioCodecType(
    PP_AudioCodec_Type_Samsung codec) {
  config_.codec = codec;
}

PP_AudioCodec_Profile_Samsung
AudioElementaryStreamResource::GetAudioCodecProfile() {
  return config_.profile;
}

void AudioElementaryStreamResource::SetAudioCodecProfile(
    PP_AudioCodec_Profile_Samsung profile) {
  config_.profile = profile;
}

PP_SampleFormat_Samsung AudioElementaryStreamResource::GetSampleFormat() {
  return config_.sample_format;
}

void AudioElementaryStreamResource::SetSampleFormat(
    PP_SampleFormat_Samsung format) {
  config_.sample_format = format;
}

PP_ChannelLayout_Samsung AudioElementaryStreamResource::GetChannelLayout() {
  return config_.channel_layout;
}

void AudioElementaryStreamResource::SetChannelLayout(
    PP_ChannelLayout_Samsung layout) {
  config_.channel_layout = layout;
}

int32_t AudioElementaryStreamResource::GetBitsPerChannel() {
  return config_.bits_per_channel;
}

void AudioElementaryStreamResource::SetBitsPerChannel(int32_t bpc) {
  config_.bits_per_channel = bpc;
}

int32_t AudioElementaryStreamResource::GetSamplesPerSecond() {
  return config_.samples_per_second;
}

void AudioElementaryStreamResource::SetSamplesPerSecond(int32_t sps) {
  config_.samples_per_second = sps;
}

void AudioElementaryStreamResource::SetCodecExtraData(uint32_t data_size,
                                                      const void* data) {
  config_.extra_data.resize(data_size);
  DCHECK(config_.extra_data.size() == data_size);
  memcpy(config_.extra_data.data(), data, data_size);
}

AudioElementaryStreamResource::AudioElementaryStreamResource(
    Connection connection,
    PP_Instance instance)
    : ElementaryStreamResource<thunk::PPB_AudioElementaryStream_API>(connection,
                                                                     instance) {
}

}  // namespace proxy
}  // namespace ppapi
