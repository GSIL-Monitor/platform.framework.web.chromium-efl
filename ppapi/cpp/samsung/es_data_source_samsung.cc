// Copyright (c) 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/cpp/samsung/es_data_source_samsung.h"

#include "ppapi/c/pp_errors.h"
#include "ppapi/cpp/instance_handle.h"
#include "ppapi/cpp/module_impl.h"
#include "ppapi/cpp/samsung/elementary_stream_listener_samsung.h"
#include "ppapi/cpp/samsung/media_data_source_samsung.h"

#define CALL_ELEMENTARY_STREAM_COMMON(Fn, cb, ...)                  \
  do {                                                              \
    if (has_interface<PPB_ElementaryStream_Samsung_1_1>()) {        \
      return get_interface<PPB_ElementaryStream_Samsung_1_1>()->Fn( \
          __VA_ARGS__);                                             \
    } else if (has_interface<PPB_ElementaryStream_Samsung_1_0>()) { \
      return get_interface<PPB_ElementaryStream_Samsung_1_0>()->Fn( \
          __VA_ARGS__);                                             \
    }                                                               \
    return cb.MayForce(PP_ERROR_NOINTERFACE);                       \
  } while (0)

namespace pp {

namespace {

template <>
const char* interface_name<PPB_ElementaryStream_Samsung_1_0>() {
  return PPB_ELEMENTARYSTREAM_SAMSUNG_INTERFACE_1_0;
}

template <>
const char* interface_name<PPB_ElementaryStream_Samsung_1_1>() {
  return PPB_ELEMENTARYSTREAM_SAMSUNG_INTERFACE_1_1;
}

template <>
const char* interface_name<PPB_AudioElementaryStream_Samsung_1_0>() {
  return PPB_AUDIOELEMENTARYSTREAM_SAMSUNG_INTERFACE_1_0;
}

template <>
const char* interface_name<PPB_VideoElementaryStream_Samsung_1_0>() {
  return PPB_VIDEOELEMENTARYSTREAM_SAMSUNG_INTERFACE_1_0;
}

template <>
const char* interface_name<PPB_ESDataSource_Samsung_1_0>() {
  return PPB_ESDATASOURCE_SAMSUNG_INTERFACE_1_0;
}

void ElementaryStreamListenerWrapper_OnNeedData(int32_t max_bytes,
    void* user_data) {
  if (!user_data) return;

  ElementaryStreamListener_Samsung* listener =
      static_cast<ElementaryStreamListener_Samsung*>(user_data);
  listener->OnNeedData(max_bytes);
}

void ElementaryStreamListenerWrapper_OnEnoughData(void* user_data) {
  if (!user_data) return;

  ElementaryStreamListener_Samsung* listener =
      static_cast<ElementaryStreamListener_Samsung*>(user_data);
  listener->OnEnoughData();
}

void ElementaryStreamListenerWrapper_OnSeekData(PP_TimeTicks new_position,
    void* user_data) {
  if (!user_data) return;

  ElementaryStreamListener_Samsung* listener =
      static_cast<ElementaryStreamListener_Samsung*>(user_data);
  listener->OnSeekData(new_position);
}

const PPP_ElementaryStreamListener_Samsung_1_0* GetListenerWrapper() {
  static const PPP_ElementaryStreamListener_Samsung_1_0 listener = {
      &ElementaryStreamListenerWrapper_OnNeedData,
      &ElementaryStreamListenerWrapper_OnEnoughData,
      &ElementaryStreamListenerWrapper_OnSeekData
  };
  return &listener;
}

}  // namespace

// ElementaryStream_Samsung implementation

ElementaryStream_Samsung::ElementaryStream_Samsung(
    const ElementaryStream_Samsung& other)
    : Resource(other) {
}

ElementaryStream_Samsung& ElementaryStream_Samsung::operator=(
    const ElementaryStream_Samsung& other) {
  Resource::operator=(other);
  return *this;
}

ElementaryStream_Samsung::~ElementaryStream_Samsung() {
}

int32_t ElementaryStream_Samsung::InitializeDone(
    const CompletionCallback& callback) {
  return ElementaryStream_Samsung::InitializeDone(
      PP_STREAMINITIALIZATIONMODE_FULL, callback);
}

int32_t ElementaryStream_Samsung::InitializeDone(
    PP_StreamInitializationMode mode,
    const CompletionCallback& callback) {
  if (has_interface<PPB_ElementaryStream_Samsung_1_1>()) {
    return get_interface<PPB_ElementaryStream_Samsung_1_1>()->InitializeDone(
        pp_resource(), mode, callback.pp_completion_callback());
  }

  if (has_interface<PPB_ElementaryStream_Samsung_1_0>()) {
    if (mode != PP_STREAMINITIALIZATIONMODE_FULL)
      return PP_ERROR_NOTSUPPORTED;

    return get_interface<PPB_ElementaryStream_Samsung_1_0>()->InitializeDone(
        pp_resource(), callback.pp_completion_callback());
  }

  return callback.MayForce(PP_ERROR_NOINTERFACE);
}

int32_t ElementaryStream_Samsung::AppendPacket(
    const PP_ESPacket& packet,
    const CompletionCallback& callback) {
  CALL_ELEMENTARY_STREAM_COMMON(AppendPacket, callback, pp_resource(), &packet,
                                callback.pp_completion_callback());
}

int32_t ElementaryStream_Samsung::AppendEncryptedPacket(
    const PP_ESPacket& packet,
    const PP_ESPacketEncryptionInfo& encryption_info,
    const CompletionCallback& callback) {
  CALL_ELEMENTARY_STREAM_COMMON(AppendEncryptedPacket, callback, pp_resource(),
                                &packet, &encryption_info,
                                callback.pp_completion_callback());
}

int32_t ElementaryStream_Samsung::AppendTrustZonePacket(
    const PP_ESPacket& packet,
    const PP_TrustZoneReference& handle,
    const CompletionCallback& callback) {
  if (has_interface<PPB_ElementaryStream_Samsung_1_1>()) {
    return get_interface<PPB_ElementaryStream_Samsung_1_1>()
        ->AppendTrustZonePacket(pp_resource(), &packet, &handle,
                                callback.pp_completion_callback());
  }

  return callback.MayForce(PP_ERROR_NOINTERFACE);
}

int32_t ElementaryStream_Samsung::Flush(const CompletionCallback& callback) {
  CALL_ELEMENTARY_STREAM_COMMON(Flush, callback, pp_resource(),
                                callback.pp_completion_callback());
}

int32_t ElementaryStream_Samsung::SetDRMInitData(
    uint32_t type_size, const void* type,
    uint32_t init_data_size, const void* init_data,
    const CompletionCallback& callback) {
  std::string type_str(reinterpret_cast<const char*>(type), type_size);
  CALL_ELEMENTARY_STREAM_COMMON(SetDRMInitData, callback, pp_resource(),
                                type_str.c_str(), init_data_size, init_data,
                                callback.pp_completion_callback());
}

int32_t ElementaryStream_Samsung::SetDRMInitData(
    const std::string& type,
    uint32_t init_data_size, const void* init_data,
    const CompletionCallback& callback) {
  CALL_ELEMENTARY_STREAM_COMMON(SetDRMInitData, callback, pp_resource(),
                                type.c_str(), init_data_size, init_data,
                                callback.pp_completion_callback());
}

ElementaryStream_Samsung::ElementaryStream_Samsung()
    : Resource() {
}

ElementaryStream_Samsung::ElementaryStream_Samsung(PP_Resource resource)
    : Resource(resource) {
}

ElementaryStream_Samsung::ElementaryStream_Samsung(const Resource& resource)
    : Resource(resource) {
}

ElementaryStream_Samsung::ElementaryStream_Samsung(
     PassRef, PP_Resource resource)
    : Resource(PASS_REF, resource) {
}

// AudioElementaryStream_Samsung implementation

AudioElementaryStream_Samsung::AudioElementaryStream_Samsung()
    : ElementaryStream_Samsung() {
}

AudioElementaryStream_Samsung::AudioElementaryStream_Samsung(
    PP_Resource resource)
    : ElementaryStream_Samsung(resource) {
}

AudioElementaryStream_Samsung::AudioElementaryStream_Samsung(
    PassRef, PP_Resource resource)
    : ElementaryStream_Samsung(PASS_REF, resource) {
}

AudioElementaryStream_Samsung::AudioElementaryStream_Samsung(
    const AudioElementaryStream_Samsung& other)
    : ElementaryStream_Samsung(other) {
}

AudioElementaryStream_Samsung& AudioElementaryStream_Samsung::operator=(
    const AudioElementaryStream_Samsung& other) {
  ElementaryStream_Samsung::operator=(other);
  return *this;
}

AudioElementaryStream_Samsung::~AudioElementaryStream_Samsung() {
}

PP_ElementaryStream_Type_Samsung
AudioElementaryStream_Samsung::GetStreamType() const {
  return PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_AUDIO;
}

PP_AudioCodec_Type_Samsung
AudioElementaryStream_Samsung::GetAudioCodecType() const {
  if (has_interface<PPB_AudioElementaryStream_Samsung_1_0>()) {
    return get_interface<PPB_AudioElementaryStream_Samsung_1_0>()->
        GetAudioCodecType(pp_resource());
  }

  return PP_AUDIOCODEC_TYPE_SAMSUNG_UNKNOWN;
}

void AudioElementaryStream_Samsung::SetAudioCodecType(
    PP_AudioCodec_Type_Samsung audio_codec) {
  if (has_interface<PPB_AudioElementaryStream_Samsung_1_0>()) {
    get_interface<PPB_AudioElementaryStream_Samsung_1_0>()->
        SetAudioCodecType(pp_resource(), audio_codec);
  }
}

PP_AudioCodec_Profile_Samsung
AudioElementaryStream_Samsung::GetAudioCodecProfile() const {
  if (has_interface<PPB_AudioElementaryStream_Samsung_1_0>()) {
    return get_interface<PPB_AudioElementaryStream_Samsung_1_0>()->
        GetAudioCodecProfile(pp_resource());
  }

  return PP_AUDIOCODEC_PROFILE_SAMSUNG_UNKNOWN;
}

void AudioElementaryStream_Samsung::SetAudioCodecProfile(
    PP_AudioCodec_Profile_Samsung profile) {
  if (has_interface<PPB_AudioElementaryStream_Samsung_1_0>()) {
    get_interface<PPB_AudioElementaryStream_Samsung_1_0>()->
        SetAudioCodecProfile(pp_resource(), profile);
  }
}

PP_SampleFormat_Samsung AudioElementaryStream_Samsung::GetSampleFormat() const {
  if (has_interface<PPB_AudioElementaryStream_Samsung_1_0>()) {
    return get_interface<PPB_AudioElementaryStream_Samsung_1_0>()->
        GetSampleFormat(pp_resource());
  }

  return PP_SAMPLEFORMAT_SAMSUNG_UNKNOWN;
}

void AudioElementaryStream_Samsung::SetSampleFormat(
  PP_SampleFormat_Samsung sample_format) {
  if (has_interface<PPB_AudioElementaryStream_Samsung_1_0>()) {
    get_interface<PPB_AudioElementaryStream_Samsung_1_0>()->
        SetSampleFormat(pp_resource(), sample_format);
  }
}

PP_ChannelLayout_Samsung
AudioElementaryStream_Samsung::GetChannelLayout() const {
  if (has_interface<PPB_AudioElementaryStream_Samsung_1_0>()) {
    return get_interface<PPB_AudioElementaryStream_Samsung_1_0>()->
        GetChannelLayout(pp_resource());
  }

  return PP_CHANNEL_LAYOUT_SAMSUNG_NONE;
}

void AudioElementaryStream_Samsung::SetChannelLayout(
    PP_ChannelLayout_Samsung channel_layout) {
  if (has_interface<PPB_AudioElementaryStream_Samsung_1_0>()) {
    get_interface<PPB_AudioElementaryStream_Samsung_1_0>()->
        SetChannelLayout(pp_resource(), channel_layout);
  }
}

int32_t AudioElementaryStream_Samsung::GetBitsPerChannel() const {
  if (has_interface<PPB_AudioElementaryStream_Samsung_1_0>()) {
    return get_interface<PPB_AudioElementaryStream_Samsung_1_0>()->
        GetBitsPerChannel(pp_resource());
  }

  return -1;
}

void AudioElementaryStream_Samsung::SetBitsPerChannel(
    int32_t bits_per_channel) {
  if (has_interface<PPB_AudioElementaryStream_Samsung_1_0>()) {
    get_interface<PPB_AudioElementaryStream_Samsung_1_0>()->
        SetBitsPerChannel(pp_resource(), bits_per_channel);
  }
}

int32_t AudioElementaryStream_Samsung::GetSamplesPerSecond() const {
  if (has_interface<PPB_AudioElementaryStream_Samsung_1_0>()) {
    return get_interface<PPB_AudioElementaryStream_Samsung_1_0>()->
        GetSamplesPerSecond(pp_resource());
  }

  return -1;
}

void AudioElementaryStream_Samsung::SetSamplesPerSecond(
    int32_t samples_per_second) {
  if (has_interface<PPB_AudioElementaryStream_Samsung_1_0>()) {
    get_interface<PPB_AudioElementaryStream_Samsung_1_0>()->
          SetSamplesPerSecond(pp_resource(), samples_per_second);
  }
}

void AudioElementaryStream_Samsung::SetCodecExtraData(
    uint32_t extra_data_size,
    const void* extra_data) {
  if (has_interface<PPB_AudioElementaryStream_Samsung_1_0>()) {
    get_interface<PPB_AudioElementaryStream_Samsung_1_0>()->
        SetCodecExtraData(pp_resource(), extra_data_size, extra_data);
  }
}

// VideoElementaryStream_Samsung implementation

VideoElementaryStream_Samsung::VideoElementaryStream_Samsung()
    : ElementaryStream_Samsung() {
}

VideoElementaryStream_Samsung::VideoElementaryStream_Samsung(
    PP_Resource resource)
    : ElementaryStream_Samsung(resource) {
}

VideoElementaryStream_Samsung::VideoElementaryStream_Samsung(
    PassRef, PP_Resource resource)
    : ElementaryStream_Samsung(PASS_REF, resource) {
}

VideoElementaryStream_Samsung::VideoElementaryStream_Samsung(
    const VideoElementaryStream_Samsung& other)
    : ElementaryStream_Samsung(other) {
}

VideoElementaryStream_Samsung& VideoElementaryStream_Samsung::operator=(
    const VideoElementaryStream_Samsung& other) {
  ElementaryStream_Samsung::operator=(other);
  return *this;
}

VideoElementaryStream_Samsung::~VideoElementaryStream_Samsung() {
}

PP_ElementaryStream_Type_Samsung
VideoElementaryStream_Samsung::GetStreamType() const {
  return PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_VIDEO;
}

PP_VideoCodec_Type_Samsung
VideoElementaryStream_Samsung::GetVideoCodecType() const {
  if (has_interface<PPB_VideoElementaryStream_Samsung_1_0>()) {
    return get_interface<PPB_VideoElementaryStream_Samsung_1_0>()->
        GetVideoCodecType(pp_resource());
  }

  return PP_VIDEOCODEC_TYPE_SAMSUNG_UNKNOWN;
}

void VideoElementaryStream_Samsung::SetVideoCodecType(
    PP_VideoCodec_Type_Samsung video_codec) {
  if (has_interface<PPB_VideoElementaryStream_Samsung_1_0>()) {
    get_interface<PPB_VideoElementaryStream_Samsung_1_0>()->
        SetVideoCodecType(pp_resource(), video_codec);
  }
}

PP_VideoCodec_Profile_Samsung
VideoElementaryStream_Samsung::GetVideoCodecProfile() const {
  if (has_interface<PPB_VideoElementaryStream_Samsung_1_0>()) {
    return get_interface<PPB_VideoElementaryStream_Samsung_1_0>()->
        GetVideoCodecProfile(pp_resource());
  }

  return PP_VIDEOCODEC_PROFILE_SAMSUNG_UNKNOWN;
}

void VideoElementaryStream_Samsung::SetVideoCodecProfile(
    PP_VideoCodec_Profile_Samsung video_codec) {
  if (has_interface<PPB_VideoElementaryStream_Samsung_1_0>()) {
    get_interface<PPB_VideoElementaryStream_Samsung_1_0>()->
        SetVideoCodecProfile(pp_resource(), video_codec);
  }
}

PP_VideoFrame_Format_Samsung
VideoElementaryStream_Samsung::GetVideoFrameFormat() const {
  if (has_interface<PPB_VideoElementaryStream_Samsung_1_0>()) {
    return get_interface<PPB_VideoElementaryStream_Samsung_1_0>()->
        GetVideoFrameFormat(pp_resource());
  }

  return PP_VIDEOFRAME_FORMAT_SAMSUNG_INVALID;
}

void VideoElementaryStream_Samsung::SetVideoFrameFormat(
    PP_VideoFrame_Format_Samsung frame_format) {
  if (has_interface<PPB_VideoElementaryStream_Samsung_1_0>()) {
    get_interface<PPB_VideoElementaryStream_Samsung_1_0>()->
        SetVideoFrameFormat(pp_resource(), frame_format);
  }
}

PP_Size VideoElementaryStream_Samsung::GetVideoFrameSize() const {
  PP_Size size = PP_MakeSize(-1, -1);
  if (has_interface<PPB_VideoElementaryStream_Samsung_1_0>()) {
    get_interface<PPB_VideoElementaryStream_Samsung_1_0>()->
      GetVideoFrameSize(pp_resource(), &size);
  }

  return size;
}

void VideoElementaryStream_Samsung::SetVideoFrameSize(const PP_Size& size) {
  if (has_interface<PPB_VideoElementaryStream_Samsung_1_0>()) {
    get_interface<PPB_VideoElementaryStream_Samsung_1_0>()->
        SetVideoFrameSize(pp_resource(), &size);
  }
}

void VideoElementaryStream_Samsung::GetFrameRate(
    uint32_t* numerator, uint32_t* denominator) const {
  if (!numerator || !denominator) {
    if (numerator) *numerator = 0;
    if (denominator) *denominator = 0;
  } else if (has_interface<PPB_VideoElementaryStream_Samsung_1_0>()) {
    get_interface<PPB_VideoElementaryStream_Samsung_1_0>()->
        GetFrameRate(pp_resource(), numerator, denominator);
  } else {
    *numerator = 0;
    *denominator = 0;
  }
}

void VideoElementaryStream_Samsung::SetFrameRate(
    uint32_t numerator, uint32_t denominator) {
  if (has_interface<PPB_VideoElementaryStream_Samsung_1_0>()) {
    get_interface<PPB_VideoElementaryStream_Samsung_1_0>()->
        SetFrameRate(pp_resource(), numerator, denominator);
  }
}

void VideoElementaryStream_Samsung::SetCodecExtraData(
    uint32_t extra_data_size, const void* extra_data) {
  if (has_interface<PPB_VideoElementaryStream_Samsung_1_0>()) {
    get_interface<PPB_VideoElementaryStream_Samsung_1_0>()->
        SetCodecExtraData(pp_resource(), extra_data_size, extra_data);
  }
}

// ESDataSource implementation

ESDataSource_Samsung::ESDataSource_Samsung(const InstanceHandle& instance) {
  if (has_interface<PPB_ESDataSource_Samsung_1_0>()) {
    PassRefFromConstructor(
        get_interface<PPB_ESDataSource_Samsung_1_0>()->Create(
            instance.pp_instance()));
  }
}

ESDataSource_Samsung::ESDataSource_Samsung(const ESDataSource_Samsung& other)
    : MediaDataSource_Samsung(other) {
}

ESDataSource_Samsung::ESDataSource_Samsung(PP_Resource resource)
    : MediaDataSource_Samsung(resource) {
}

ESDataSource_Samsung::ESDataSource_Samsung(PassRef, PP_Resource resource)
    : MediaDataSource_Samsung(PASS_REF, resource) {
}

ESDataSource_Samsung& ESDataSource_Samsung::operator=(
    const ESDataSource_Samsung& other) {
  MediaDataSource_Samsung::operator=(other);
  return *this;
}

ESDataSource_Samsung::~ESDataSource_Samsung() {
}

int32_t ESDataSource_Samsung::SetDuration(PP_TimeDelta duration,
                                          const CompletionCallback& callback) {
  if (has_interface<PPB_ESDataSource_Samsung_1_0>()) {
    return get_interface<PPB_ESDataSource_Samsung_1_0>()->SetDuration(
        pp_resource(), duration, callback.pp_completion_callback());
  }

  return callback.MayForce(PP_ERROR_NOINTERFACE);
}

int32_t ESDataSource_Samsung::SetEndOfStream(
    const CompletionCallback& callback) {

  if (has_interface<PPB_ESDataSource_Samsung_1_0>()) {
    return get_interface<PPB_ESDataSource_Samsung_1_0>()->SetEndOfStream(
        pp_resource(), callback.pp_completion_callback());
  }

  return callback.MayForce(PP_ERROR_NOINTERFACE);
}

int32_t ESDataSource_Samsung::AddStream(
    PP_ElementaryStream_Type_Samsung stream_type,
    PP_Resource* stream,
    ElementaryStreamListener_Samsung* listener,
    const CompletionCallback& callback) {

  if (has_interface<PPB_ESDataSource_Samsung_1_0>()) {
    return get_interface<PPB_ESDataSource_Samsung_1_0>()->AddStream(
        pp_resource(), stream_type, GetListenerWrapper(), listener, stream,
        callback.pp_completion_callback());
  }

  return callback.MayForce(PP_ERROR_NOINTERFACE);
}

}  // namespace pp
