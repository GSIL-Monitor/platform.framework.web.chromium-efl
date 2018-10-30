// Copyright (c) 2016 Samsung Electronics. All rights reserved.

// From samsung/ppb_media_data_source_samsung.idl modified Mon Aug  1 15:04:43
// 2016.

#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/samsung/ppb_media_data_source_samsung.h"
#include "ppapi/shared_impl/tracked_callback.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppapi_thunk_export.h"
#include "ppapi/thunk/ppb_media_data_source_api.h"

namespace ppapi {
namespace thunk {

namespace {

namespace ppb_media_data_source_thunk {

PP_Bool IsMediaDataSource(PP_Resource resource) {
  VLOG(4) << "PPB_MediaDataSource_Samsung::IsMediaDataSource()";
  EnterResource<PPB_MediaDataSource_API> enter(resource, true);
  return PP_FromBool(enter.succeeded());
}

}  // namespace ppb_media_data_source_thunk

namespace ppb_url_data_source_thunk {

PP_Resource CreateURLDataSource(PP_Instance instance, const char* url) {
  VLOG(4) << "PPB_URLDataSource_Samsung::Create()";
  EnterResourceCreation enter(instance);
  if (enter.failed())
    return 0;
  return enter.functions()->CreateURLDataSource(instance, url);
}

PP_Bool IsURLDataSource(PP_Resource resource) {
  VLOG(4) << "PPB_URLDataSource_Samsung::IsURLDataSource()";
  EnterResource<PPB_URLDataSource_API> enter(resource, true);
  return PP_FromBool(enter.succeeded());
}

int32_t GetStreamingProperty(PP_Resource resource,
                             PP_StreamingProperty type,
                             struct PP_Var* value,
                             struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_URLDataSource_Samsung::GetStreamingProperty()";
  EnterResource<PPB_URLDataSource_API> enter(resource, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(
      enter.object()->GetStreamingProperty(type, value, enter.callback()));
}

int32_t SetStreamingProperty(PP_Resource resource,
                             PP_StreamingProperty type,
                             struct PP_Var value,
                             struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_URLDataSource_Samsung::SetStreamingProperty()";
  EnterResource<PPB_URLDataSource_API> enter(resource, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(
      enter.object()->SetStreamingProperty(type, value, enter.callback()));
}

}  // namespace ppb_url_data_source_thunk

namespace ppb_es_data_source_thunk {

PP_Resource CreateESDataSource(PP_Instance instance) {
  VLOG(4) << "PPB_ESDataSource_Samsung::Create()";
  EnterResourceCreation enter(instance);
  if (enter.failed())
    return 0;
  return enter.functions()->CreateESDataSource(instance);
}

PP_Bool IsESDataSource(PP_Resource resource) {
  VLOG(4) << "PPB_ESDataSource_Samsung::IsESDataSource()";
  EnterResource<PPB_ESDataSource_API> enter(resource, true);
  return PP_FromBool(enter.succeeded());
}

int32_t AddStream(
    PP_Resource data_source,
    PP_ElementaryStream_Type_Samsung stream_type,
    const struct PPP_ElementaryStreamListener_Samsung_1_0* listener,
    void* user_data,
    PP_Resource* stream,
    struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_ESDataSource_Samsung::AddStream()";
  EnterResource<PPB_ESDataSource_API> enter(data_source, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(enter.object()->AddStream(
      stream_type, listener, user_data, stream, enter.callback()));
}

int32_t SetDuration(PP_Resource data_source,
                    PP_TimeDelta duration,
                    struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_ESDataSource_Samsung::SetDuration()";
  EnterResource<PPB_ESDataSource_API> enter(data_source, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(
      enter.object()->SetDuration(duration, enter.callback()));
}

int32_t SetEndOfStream(PP_Resource data_source,
                       struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_ESDataSource_Samsung::SetEndOfStream()";
  EnterResource<PPB_ESDataSource_API> enter(data_source, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(enter.object()->SetEndOfStream(enter.callback()));
}

}  // namespace ppb_es_data_source_thunk

namespace ppb_elementary_stream_thunk {

PP_Bool IsElementaryStream(PP_Resource resource) {
  VLOG(4) << "PPB_ElementaryStream_Samsung::IsElementaryStream()";
  EnterResource<PPB_ElementaryStream_API> enter(resource, true);
  return PP_FromBool(enter.succeeded());
}

PP_ElementaryStream_Type_Samsung GetStreamType(PP_Resource resource) {
  VLOG(4) << "PPB_ElementaryStream_Samsung::GetStreamType()";
  EnterResource<PPB_ElementaryStream_API> enter(resource, true);
  if (enter.failed())
    return PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_UNKNOWN;
  return enter.object()->GetStreamType();
}

int32_t InitializeDone_1_0(PP_Resource stream,
                           struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_ElementaryStream_Samsung::InitializeDone()";
  EnterResource<PPB_ElementaryStream_API> enter(stream, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(enter.object()->InitializeDone(
      PP_STREAMINITIALIZATIONMODE_FULL, enter.callback()));
}

int32_t InitializeDone(PP_Resource stream,
                       PP_StreamInitializationMode mode,
                       struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_ElementaryStream_Samsung::InitializeDone()";
  EnterResource<PPB_ElementaryStream_API> enter(stream, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(
      enter.object()->InitializeDone(mode, enter.callback()));
}

int32_t AppendPacket(PP_Resource stream,
                     const struct PP_ESPacket* packet,
                     struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_ElementaryStream_Samsung::AppendPacket()";
  EnterResource<PPB_ElementaryStream_API> enter(stream, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(
      enter.object()->AppendPacket(packet, enter.callback()));
}

int32_t AppendEncryptedPacket(
    PP_Resource stream,
    const struct PP_ESPacket* packet,
    const struct PP_ESPacketEncryptionInfo* encryption_info,
    struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_ElementaryStream_Samsung::AppendEncryptedPacket()";
  EnterResource<PPB_ElementaryStream_API> enter(stream, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(enter.object()->AppendEncryptedPacket(
      packet, encryption_info, enter.callback()));
}

int32_t AppendTrustZonePacket(PP_Resource stream,
                              const struct PP_ESPacket* packet,
                              const struct PP_TrustZoneReference* handle,
                              struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_ElementaryStream_Samsung::AppendTrustZonePacket()";
  EnterResource<PPB_ElementaryStream_API> enter(stream, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(
      enter.object()->AppendTrustZonePacket(packet, handle, enter.callback()));
}

int32_t Flush(PP_Resource stream, struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_ElementaryStream_Samsung::Flush()";
  EnterResource<PPB_ElementaryStream_API> enter(stream, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(enter.object()->Flush(enter.callback()));
}

int32_t SetDRMInitData(PP_Resource stream,
                       const char* type,
                       uint32_t init_data_size,
                       const void* init_data,
                       struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_ElementaryStream_Samsung::SetDRMInitData()";
  EnterResource<PPB_ElementaryStream_API> enter(stream, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(enter.object()->SetDRMInitData(
      strlen(type), type, init_data_size, init_data, enter.callback()));
}

}  // namespace ppb_elementary_stream_thunk

namespace ppb_audio_elementary_stream_thunk {

PP_Bool IsAudioElementaryStream(PP_Resource resource) {
  VLOG(4) << "PPB_AudioElementaryStream_Samsung::IsAudioElementaryStream()";
  EnterResource<PPB_AudioElementaryStream_API> enter(resource, true);
  return PP_FromBool(enter.succeeded());
}

PP_AudioCodec_Type_Samsung GetAudioCodecType(PP_Resource stream) {
  VLOG(4) << "PPB_AudioElementaryStream_Samsung::GetAudioCodecType()";
  EnterResource<PPB_AudioElementaryStream_API> enter(stream, true);
  if (enter.failed())
    return PP_AUDIOCODEC_TYPE_SAMSUNG_UNKNOWN;
  return enter.object()->GetAudioCodecType();
}

void SetAudioCodecType(PP_Resource stream,
                       PP_AudioCodec_Type_Samsung audio_codec) {
  VLOG(4) << "PPB_AudioElementaryStream_Samsung::SetAudioCodecType()";
  EnterResource<PPB_AudioElementaryStream_API> enter(stream, true);
  if (enter.failed())
    return;
  enter.object()->SetAudioCodecType(audio_codec);
}

PP_AudioCodec_Profile_Samsung GetAudioCodecProfile(PP_Resource stream) {
  VLOG(4) << "PPB_AudioElementaryStream_Samsung::GetAudioCodecProfile()";
  EnterResource<PPB_AudioElementaryStream_API> enter(stream, true);
  if (enter.failed())
    return PP_AUDIOCODEC_PROFILE_SAMSUNG_UNKNOWN;
  return enter.object()->GetAudioCodecProfile();
}

void SetAudioCodecProfile(PP_Resource stream,
                          PP_AudioCodec_Profile_Samsung profile) {
  VLOG(4) << "PPB_AudioElementaryStream_Samsung::SetAudioCodecProfile()";
  EnterResource<PPB_AudioElementaryStream_API> enter(stream, true);
  if (enter.failed())
    return;
  enter.object()->SetAudioCodecProfile(profile);
}

PP_SampleFormat_Samsung GetSampleFormat(PP_Resource stream) {
  VLOG(4) << "PPB_AudioElementaryStream_Samsung::GetSampleFormat()";
  EnterResource<PPB_AudioElementaryStream_API> enter(stream, true);
  if (enter.failed())
    return PP_SAMPLEFORMAT_SAMSUNG_UNKNOWN;
  return enter.object()->GetSampleFormat();
}

void SetSampleFormat(PP_Resource stream,
                     PP_SampleFormat_Samsung sample_format) {
  VLOG(4) << "PPB_AudioElementaryStream_Samsung::SetSampleFormat()";
  EnterResource<PPB_AudioElementaryStream_API> enter(stream, true);
  if (enter.failed())
    return;
  enter.object()->SetSampleFormat(sample_format);
}

PP_ChannelLayout_Samsung GetChannelLayout(PP_Resource stream) {
  VLOG(4) << "PPB_AudioElementaryStream_Samsung::GetChannelLayout()";
  EnterResource<PPB_AudioElementaryStream_API> enter(stream, true);
  if (enter.failed())
    return PP_CHANNEL_LAYOUT_SAMSUNG_NONE;
  return enter.object()->GetChannelLayout();
}

void SetChannelLayout(PP_Resource stream,
                      PP_ChannelLayout_Samsung channel_layout) {
  VLOG(4) << "PPB_AudioElementaryStream_Samsung::SetChannelLayout()";
  EnterResource<PPB_AudioElementaryStream_API> enter(stream, true);
  if (enter.failed())
    return;
  enter.object()->SetChannelLayout(channel_layout);
}

int32_t GetBitsPerChannel(PP_Resource stream) {
  VLOG(4) << "PPB_AudioElementaryStream_Samsung::GetBitsPerChannel()";
  EnterResource<PPB_AudioElementaryStream_API> enter(stream, true);
  if (enter.failed())
    return enter.retval();
  return enter.object()->GetBitsPerChannel();
}

void SetBitsPerChannel(PP_Resource stream, int32_t bits_per_channel) {
  VLOG(4) << "PPB_AudioElementaryStream_Samsung::SetBitsPerChannel()";
  EnterResource<PPB_AudioElementaryStream_API> enter(stream, true);
  if (enter.failed())
    return;
  enter.object()->SetBitsPerChannel(bits_per_channel);
}

int32_t GetSamplesPerSecond(PP_Resource stream) {
  VLOG(4) << "PPB_AudioElementaryStream_Samsung::GetSamplesPerSecond()";
  EnterResource<PPB_AudioElementaryStream_API> enter(stream, true);
  if (enter.failed())
    return enter.retval();
  return enter.object()->GetSamplesPerSecond();
}

void SetSamplesPerSecond(PP_Resource stream, int32_t samples_per_second) {
  VLOG(4) << "PPB_AudioElementaryStream_Samsung::SetSamplesPerSecond()";
  EnterResource<PPB_AudioElementaryStream_API> enter(stream, true);
  if (enter.failed())
    return;
  enter.object()->SetSamplesPerSecond(samples_per_second);
}

void SetCodecExtraData(PP_Resource stream,
                       uint32_t extra_data_size,
                       const void* extra_data) {
  VLOG(4) << "PPB_AudioElementaryStream_Samsung::SetCodecExtraData()";
  EnterResource<PPB_AudioElementaryStream_API> enter(stream, true);
  if (enter.failed())
    return;
  enter.object()->SetCodecExtraData(extra_data_size, extra_data);
}

}  // namespace ppb_audio_elementary_stream_thunk

namespace ppb_video_elementary_stream_thunk {

PP_Bool IsVideoElementaryStream(PP_Resource resource) {
  VLOG(4) << "PPB_VideoElementaryStream_Samsung::IsVideoElementaryStream()";
  EnterResource<PPB_VideoElementaryStream_API> enter(resource, true);
  return PP_FromBool(enter.succeeded());
}

PP_VideoCodec_Type_Samsung GetVideoCodecType(PP_Resource stream) {
  VLOG(4) << "PPB_VideoElementaryStream_Samsung::GetVideoCodecType()";
  EnterResource<PPB_VideoElementaryStream_API> enter(stream, true);
  if (enter.failed())
    return PP_VIDEOCODEC_TYPE_SAMSUNG_UNKNOWN;
  return enter.object()->GetVideoCodecType();
}

void SetVideoCodecType(PP_Resource stream,
                       PP_VideoCodec_Type_Samsung video_codec) {
  VLOG(4) << "PPB_VideoElementaryStream_Samsung::SetVideoCodecType()";
  EnterResource<PPB_VideoElementaryStream_API> enter(stream, true);
  if (enter.failed())
    return;
  enter.object()->SetVideoCodecType(video_codec);
}

PP_VideoCodec_Profile_Samsung GetVideoCodecProfile(PP_Resource stream) {
  VLOG(4) << "PPB_VideoElementaryStream_Samsung::GetVideoCodecProfile()";
  EnterResource<PPB_VideoElementaryStream_API> enter(stream, true);
  if (enter.failed())
    return PP_VIDEOCODEC_PROFILE_SAMSUNG_UNKNOWN;
  return enter.object()->GetVideoCodecProfile();
}

void SetVideoCodecProfile(PP_Resource stream,
                          PP_VideoCodec_Profile_Samsung profile) {
  VLOG(4) << "PPB_VideoElementaryStream_Samsung::SetVideoCodecProfile()";
  EnterResource<PPB_VideoElementaryStream_API> enter(stream, true);
  if (enter.failed())
    return;
  enter.object()->SetVideoCodecProfile(profile);
}

PP_VideoFrame_Format_Samsung GetVideoFrameFormat(PP_Resource stream) {
  VLOG(4) << "PPB_VideoElementaryStream_Samsung::GetVideoFrameFormat()";
  EnterResource<PPB_VideoElementaryStream_API> enter(stream, true);
  if (enter.failed())
    return PP_VIDEOFRAME_FORMAT_SAMSUNG_INVALID;
  return enter.object()->GetVideoFrameFormat();
}

void SetVideoFrameFormat(PP_Resource stream,
                         PP_VideoFrame_Format_Samsung frame_format) {
  VLOG(4) << "PPB_VideoElementaryStream_Samsung::SetVideoFrameFormat()";
  EnterResource<PPB_VideoElementaryStream_API> enter(stream, true);
  if (enter.failed())
    return;
  enter.object()->SetVideoFrameFormat(frame_format);
}

void GetVideoFrameSize(PP_Resource stream, struct PP_Size* size) {
  VLOG(4) << "PPB_VideoElementaryStream_Samsung::GetVideoFrameSize()";
  EnterResource<PPB_VideoElementaryStream_API> enter(stream, true);
  if (enter.failed())
    return;
  enter.object()->GetVideoFrameSize(size);
}

void SetVideoFrameSize(PP_Resource stream, const struct PP_Size* size) {
  VLOG(4) << "PPB_VideoElementaryStream_Samsung::SetVideoFrameSize()";
  EnterResource<PPB_VideoElementaryStream_API> enter(stream, true);
  if (enter.failed())
    return;
  enter.object()->SetVideoFrameSize(size);
}

void GetFrameRate(PP_Resource stream,
                  uint32_t* numerator,
                  uint32_t* denominator) {
  VLOG(4) << "PPB_VideoElementaryStream_Samsung::GetFrameRate()";
  EnterResource<PPB_VideoElementaryStream_API> enter(stream, true);
  if (enter.failed())
    return;
  enter.object()->GetFrameRate(numerator, denominator);
}

void SetFrameRate(PP_Resource stream,
                  uint32_t numerator,
                  uint32_t denominator) {
  VLOG(4) << "PPB_VideoElementaryStream_Samsung::SetFrameRate()";
  EnterResource<PPB_VideoElementaryStream_API> enter(stream, true);
  if (enter.failed())
    return;
  enter.object()->SetFrameRate(numerator, denominator);
}

void SetCodecExtraData(PP_Resource stream,
                       uint32_t extra_data_size,
                       const void* extra_data) {
  VLOG(4) << "PPB_VideoElementaryStream_Samsung::SetCodecExtraData()";
  EnterResource<PPB_VideoElementaryStream_API> enter(stream, true);
  if (enter.failed())
    return;
  enter.object()->SetCodecExtraData(extra_data_size, extra_data);
}

}  // namespace ppb_video_elementary_stream_thunk

const PPB_MediaDataSource_Samsung_1_0 g_ppb_mediadatasource_samsung_thunk_1_0 =
    {&ppb_media_data_source_thunk::IsMediaDataSource};

const PPB_URLDataSource_Samsung_1_0 g_ppb_urldatasource_samsung_thunk_1_0 = {
    &ppb_url_data_source_thunk::CreateURLDataSource,
    &ppb_url_data_source_thunk::IsURLDataSource,
    &ppb_url_data_source_thunk::GetStreamingProperty,
    &ppb_url_data_source_thunk::SetStreamingProperty};

const PPB_ESDataSource_Samsung_1_0 g_ppb_esdatasource_samsung_thunk_1_0 = {
    &ppb_es_data_source_thunk::CreateESDataSource,
    &ppb_es_data_source_thunk::IsESDataSource,
    &ppb_es_data_source_thunk::AddStream,
    &ppb_es_data_source_thunk::SetDuration,
    &ppb_es_data_source_thunk::SetEndOfStream};

const PPB_ElementaryStream_Samsung_1_0
    g_ppb_elementarystream_samsung_thunk_1_0 = {
        &ppb_elementary_stream_thunk::IsElementaryStream,
        &ppb_elementary_stream_thunk::GetStreamType,
        &ppb_elementary_stream_thunk::InitializeDone_1_0,
        &ppb_elementary_stream_thunk::AppendPacket,
        &ppb_elementary_stream_thunk::AppendEncryptedPacket,
        &ppb_elementary_stream_thunk::Flush,
        &ppb_elementary_stream_thunk::SetDRMInitData};

const PPB_ElementaryStream_Samsung_1_1
    g_ppb_elementarystream_samsung_thunk_1_1 = {
        &ppb_elementary_stream_thunk::IsElementaryStream,
        &ppb_elementary_stream_thunk::GetStreamType,
        &ppb_elementary_stream_thunk::InitializeDone,
        &ppb_elementary_stream_thunk::AppendPacket,
        &ppb_elementary_stream_thunk::AppendEncryptedPacket,
        &ppb_elementary_stream_thunk::AppendTrustZonePacket,
        &ppb_elementary_stream_thunk::Flush,
        &ppb_elementary_stream_thunk::SetDRMInitData};

const PPB_AudioElementaryStream_Samsung_1_0
    g_ppb_audioelementarystream_samsung_thunk_1_0 = {
        &ppb_audio_elementary_stream_thunk::IsAudioElementaryStream,
        &ppb_audio_elementary_stream_thunk::GetAudioCodecType,
        &ppb_audio_elementary_stream_thunk::SetAudioCodecType,
        &ppb_audio_elementary_stream_thunk::GetAudioCodecProfile,
        &ppb_audio_elementary_stream_thunk::SetAudioCodecProfile,
        &ppb_audio_elementary_stream_thunk::GetSampleFormat,
        &ppb_audio_elementary_stream_thunk::SetSampleFormat,
        &ppb_audio_elementary_stream_thunk::GetChannelLayout,
        &ppb_audio_elementary_stream_thunk::SetChannelLayout,
        &ppb_audio_elementary_stream_thunk::GetBitsPerChannel,
        &ppb_audio_elementary_stream_thunk::SetBitsPerChannel,
        &ppb_audio_elementary_stream_thunk::GetSamplesPerSecond,
        &ppb_audio_elementary_stream_thunk::SetSamplesPerSecond,
        &ppb_audio_elementary_stream_thunk::SetCodecExtraData};

const PPB_VideoElementaryStream_Samsung_1_0
    g_ppb_videoelementarystream_samsung_thunk_1_0 = {
        &ppb_video_elementary_stream_thunk::IsVideoElementaryStream,
        &ppb_video_elementary_stream_thunk::GetVideoCodecType,
        &ppb_video_elementary_stream_thunk::SetVideoCodecType,
        &ppb_video_elementary_stream_thunk::GetVideoCodecProfile,
        &ppb_video_elementary_stream_thunk::SetVideoCodecProfile,
        &ppb_video_elementary_stream_thunk::GetVideoFrameFormat,
        &ppb_video_elementary_stream_thunk::SetVideoFrameFormat,
        &ppb_video_elementary_stream_thunk::GetVideoFrameSize,
        &ppb_video_elementary_stream_thunk::SetVideoFrameSize,
        &ppb_video_elementary_stream_thunk::GetFrameRate,
        &ppb_video_elementary_stream_thunk::SetFrameRate,
        &ppb_video_elementary_stream_thunk::SetCodecExtraData};

}  // namespace

PPAPI_THUNK_EXPORT const PPB_MediaDataSource_Samsung_1_0*
GetPPB_MediaDataSource_Samsung_1_0_Thunk() {
  return &g_ppb_mediadatasource_samsung_thunk_1_0;
}

PPAPI_THUNK_EXPORT const PPB_URLDataSource_Samsung_1_0*
GetPPB_URLDataSource_Samsung_1_0_Thunk() {
  return &g_ppb_urldatasource_samsung_thunk_1_0;
}

PPAPI_THUNK_EXPORT const PPB_ESDataSource_Samsung_1_0*
GetPPB_ESDataSource_Samsung_1_0_Thunk() {
  return &g_ppb_esdatasource_samsung_thunk_1_0;
}

PPAPI_THUNK_EXPORT const PPB_ElementaryStream_Samsung_1_0*
GetPPB_ElementaryStream_Samsung_1_0_Thunk() {
  return &g_ppb_elementarystream_samsung_thunk_1_0;
}

PPAPI_THUNK_EXPORT const PPB_ElementaryStream_Samsung_1_1*
GetPPB_ElementaryStream_Samsung_1_1_Thunk() {
  return &g_ppb_elementarystream_samsung_thunk_1_1;
}

PPAPI_THUNK_EXPORT const PPB_AudioElementaryStream_Samsung_1_0*
GetPPB_AudioElementaryStream_Samsung_1_0_Thunk() {
  return &g_ppb_audioelementarystream_samsung_thunk_1_0;
}

PPAPI_THUNK_EXPORT const PPB_VideoElementaryStream_Samsung_1_0*
GetPPB_VideoElementaryStream_Samsung_1_0_Thunk() {
  return &g_ppb_videoelementarystream_samsung_thunk_1_0;
}

}  // namespace thunk
}  // namespace ppapi
