// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_THUNK_PPB_MEDIA_DATA_SOURCE_API_H_
#define PPAPI_THUNK_PPB_MEDIA_DATA_SOURCE_API_H_

#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_size.h"
#include "ppapi/c/pp_time.h"
#include "ppapi/c/samsung/pp_media_player_samsung.h"
#include "ppapi/c/samsung/ppb_media_data_source_samsung.h"
#include "ppapi/thunk/ppapi_thunk_export.h"

namespace ppapi {

class TrackedCallback;

namespace thunk {

/*
 * File defining "virtual" interfaces for Media Data Source PPAPI,
 * see ppapi/api/samsung/ppb_media_data_source_samsung.idl
 * for full description of classes and their methods.
 */

class PPAPI_THUNK_EXPORT PPB_MediaDataSource_API {
 public:
  virtual ~PPB_MediaDataSource_API() {}
};

class PPAPI_THUNK_EXPORT PPB_URLDataSource_API
    : public PPB_MediaDataSource_API {
 public:
  virtual ~PPB_URLDataSource_API() {}

  virtual int32_t GetStreamingProperty(PP_StreamingProperty,
                                       PP_Var* value,
                                       scoped_refptr<TrackedCallback>) = 0;
  virtual int32_t SetStreamingProperty(PP_StreamingProperty,
                                       PP_Var value,
                                       scoped_refptr<TrackedCallback>) = 0;
};

class PPAPI_THUNK_EXPORT PPB_ESDataSource_API : public PPB_MediaDataSource_API {
 public:
  virtual ~PPB_ESDataSource_API() {}

  virtual int32_t AddStream(PP_ElementaryStream_Type_Samsung,
                            const PPP_ElementaryStreamListener_Samsung_1_0*,
                            void* user_data,
                            PP_Resource* out_stream,
                            scoped_refptr<TrackedCallback>) = 0;
  virtual int32_t SetDuration(PP_TimeDelta, scoped_refptr<TrackedCallback>) = 0;
  virtual int32_t SetEndOfStream(scoped_refptr<TrackedCallback>) = 0;
};

class PPAPI_THUNK_EXPORT PPB_ElementaryStream_API {
 public:
  virtual ~PPB_ElementaryStream_API() {}

  virtual PP_ElementaryStream_Type_Samsung GetStreamType() = 0;

  virtual int32_t InitializeDone(PP_StreamInitializationMode,
                                 scoped_refptr<TrackedCallback>) = 0;
  virtual int32_t AppendPacket(const PP_ESPacket*,
                               scoped_refptr<TrackedCallback>) = 0;
  virtual int32_t AppendEncryptedPacket(const PP_ESPacket*,
                                        const PP_ESPacketEncryptionInfo*,
                                        scoped_refptr<TrackedCallback>) = 0;
  virtual int32_t AppendTrustZonePacket(const PP_ESPacket*,
                                        const PP_TrustZoneReference*,
                                        scoped_refptr<TrackedCallback>) = 0;
  virtual int32_t Flush(scoped_refptr<TrackedCallback>) = 0;

  virtual int32_t SetDRMInitData(uint32_t type_size,
                                 const void* type,
                                 uint32_t init_data_size,
                                 const void* init_data,
                                 scoped_refptr<TrackedCallback>) = 0;
};

class PPAPI_THUNK_EXPORT PPB_AudioElementaryStream_API
    : public PPB_ElementaryStream_API {
 public:
  virtual ~PPB_AudioElementaryStream_API() {}

  virtual PP_AudioCodec_Type_Samsung GetAudioCodecType() = 0;
  virtual void SetAudioCodecType(PP_AudioCodec_Type_Samsung) = 0;

  virtual PP_AudioCodec_Profile_Samsung GetAudioCodecProfile() = 0;
  virtual void SetAudioCodecProfile(PP_AudioCodec_Profile_Samsung) = 0;

  virtual PP_SampleFormat_Samsung GetSampleFormat() = 0;
  virtual void SetSampleFormat(PP_SampleFormat_Samsung) = 0;

  virtual PP_ChannelLayout_Samsung GetChannelLayout() = 0;
  virtual void SetChannelLayout(PP_ChannelLayout_Samsung) = 0;

  virtual int32_t GetBitsPerChannel() = 0;
  virtual void SetBitsPerChannel(int32_t) = 0;

  virtual int32_t GetSamplesPerSecond() = 0;
  virtual void SetSamplesPerSecond(int32_t) = 0;

  virtual void SetCodecExtraData(uint32_t data_size, const void* data) = 0;
};

class PPAPI_THUNK_EXPORT PPB_VideoElementaryStream_API
    : public PPB_ElementaryStream_API {
 public:
  virtual ~PPB_VideoElementaryStream_API() {}

  virtual PP_VideoCodec_Type_Samsung GetVideoCodecType() = 0;
  virtual void SetVideoCodecType(PP_VideoCodec_Type_Samsung) = 0;

  virtual PP_VideoCodec_Profile_Samsung GetVideoCodecProfile() = 0;
  virtual void SetVideoCodecProfile(PP_VideoCodec_Profile_Samsung) = 0;

  virtual PP_VideoFrame_Format_Samsung GetVideoFrameFormat() = 0;
  virtual void SetVideoFrameFormat(PP_VideoFrame_Format_Samsung) = 0;

  virtual void GetVideoFrameSize(PP_Size* size) = 0;
  virtual void SetVideoFrameSize(const PP_Size*) = 0;

  virtual void GetFrameRate(uint32_t* numerator, uint32_t* denominator) = 0;
  virtual void SetFrameRate(uint32_t numerator, uint32_t denominator) = 0;

  virtual void SetCodecExtraData(uint32_t data_size, const void* data) = 0;
};

}  // namespace thunk
}  // namespace ppapi

#endif  // PPAPI_THUNK_PPB_MEDIA_DATA_SOURCE_API_H_
