// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_MEDIA_PLAYER_AUDIO_ELEMENTARY_STREAM_RESOURCE_H_
#define PPAPI_PROXY_MEDIA_PLAYER_AUDIO_ELEMENTARY_STREAM_RESOURCE_H_

#include "ppapi/proxy/media_player/elementary_stream_resource.h"
#include "ppapi/shared_impl/media_player/media_codecs_configs.h"
#include "ppapi/thunk/ppb_media_data_source_api.h"

namespace ppapi {
namespace proxy {

/*
 * Implementation of plugin side Resource proxy for Audio Elementary Stream
 * PPAPI,
 * see ppapi/api/samsung/ppb_media_data_source_samsung.idl
 * for full description of class and its methods.
 */
class AudioElementaryStreamResource
    : public ElementaryStreamResource<thunk::PPB_AudioElementaryStream_API> {
 public:
  static AudioElementaryStreamResource* Create(Connection,
                                               PP_Instance,
                                               int pending_host_id);
  virtual ~AudioElementaryStreamResource();

  // PluginResource overrides.
  thunk::PPB_AudioElementaryStream_API* AsPPB_AudioElementaryStream_API()
      override;

  // PPBElementaryStreamAPI specific implementation
  PP_ElementaryStream_Type_Samsung GetStreamType() override;
  int32_t InitializeDone(PP_StreamInitializationMode,
                         scoped_refptr<TrackedCallback>) override;

  // PPBAudioElementaryStreamAPI implementation
  PP_AudioCodec_Type_Samsung GetAudioCodecType() override;
  void SetAudioCodecType(PP_AudioCodec_Type_Samsung) override;

  PP_AudioCodec_Profile_Samsung GetAudioCodecProfile() override;
  void SetAudioCodecProfile(PP_AudioCodec_Profile_Samsung) override;

  PP_SampleFormat_Samsung GetSampleFormat() override;
  void SetSampleFormat(PP_SampleFormat_Samsung) override;

  PP_ChannelLayout_Samsung GetChannelLayout() override;
  void SetChannelLayout(PP_ChannelLayout_Samsung) override;

  int32_t GetBitsPerChannel() override;
  void SetBitsPerChannel(int32_t) override;

  int32_t GetSamplesPerSecond() override;
  void SetSamplesPerSecond(int32_t) override;

  void SetCodecExtraData(uint32_t data_size, const void* data) override;

 private:
  AudioElementaryStreamResource(Connection, PP_Instance);

  scoped_refptr<TrackedCallback> initialize_callback_;
  AudioCodecConfig config_;
};

}  // namespace proxy
}  // namespace ppapi

#endif  // PPAPI_PROXY_MEDIA_PLAYER_AUDIO_ELEMENTARY_STREAM_RESOURCE_H_
