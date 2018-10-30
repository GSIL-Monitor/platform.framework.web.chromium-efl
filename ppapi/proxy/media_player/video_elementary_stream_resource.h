// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_MEDIA_PLAYER_VIDEO_ELEMENTARY_STREAM_RESOURCE_H_
#define PPAPI_PROXY_MEDIA_PLAYER_VIDEO_ELEMENTARY_STREAM_RESOURCE_H_

#include "ppapi/proxy/media_player/elementary_stream_resource.h"
#include "ppapi/shared_impl/media_player/media_codecs_configs.h"
#include "ppapi/thunk/ppb_media_data_source_api.h"

namespace ppapi {
namespace proxy {

/*
 * Implementation of plugin side Resource proxy for Video Elementary Stream
 * PPAPI,
 * see ppapi/api/samsung/ppb_media_data_source_samsung.idl
 * for full description of class and its methods.
 */
class VideoElementaryStreamResource
    : public ElementaryStreamResource<thunk::PPB_VideoElementaryStream_API> {
 public:
  static VideoElementaryStreamResource* Create(Connection,
                                               PP_Instance,
                                               int pending_host_id);
  VideoElementaryStreamResource(Connection, PP_Instance);
  virtual ~VideoElementaryStreamResource();

  // PluginResource overrides.
  thunk::PPB_VideoElementaryStream_API* AsPPB_VideoElementaryStream_API()
      override;

  // PPBElementaryStreamAPI specific implementation
  PP_ElementaryStream_Type_Samsung GetStreamType() override;
  int32_t InitializeDone(PP_StreamInitializationMode,
                         scoped_refptr<TrackedCallback>) override;

  // PPBVideoElementaryStreamAPI implementation
  PP_VideoCodec_Type_Samsung GetVideoCodecType() override;
  void SetVideoCodecType(PP_VideoCodec_Type_Samsung) override;

  PP_VideoCodec_Profile_Samsung GetVideoCodecProfile() override;
  void SetVideoCodecProfile(PP_VideoCodec_Profile_Samsung) override;

  PP_VideoFrame_Format_Samsung GetVideoFrameFormat() override;
  void SetVideoFrameFormat(PP_VideoFrame_Format_Samsung) override;

  void GetVideoFrameSize(PP_Size* size) override;
  void SetVideoFrameSize(const PP_Size*) override;

  void GetFrameRate(uint32_t* numerator, uint32_t* denominator) override;
  void SetFrameRate(uint32_t numerator, uint32_t denominator) override;

  void SetCodecExtraData(uint32_t data_size, const void* data) override;

 private:
  scoped_refptr<TrackedCallback> initialize_callback_;
  VideoCodecConfig config_;
};

}  // namespace proxy
}  // namespace ppapi

#endif  // PPAPI_PROXY_MEDIA_PLAYER_VIDEO_ELEMENTARY_STREAM_RESOURCE_H_
