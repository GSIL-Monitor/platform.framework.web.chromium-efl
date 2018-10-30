// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_VIDEO_ELEMENTARY_STREAM_PRIVATE_TIZEN_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_VIDEO_ELEMENTARY_STREAM_PRIVATE_TIZEN_H_

#include <vector>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "content/browser/renderer_host/pepper/media_player/pepper_video_elementary_stream_private.h"
#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_elementary_stream_private_tizen.h"
#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_player_adapter_base.h"

namespace content {

struct PepperTizenVideoStreamTraits {
  typedef ppapi::VideoCodecConfig CodecConfig;
  typedef PepperVideoStreamInfo StreamInfo;
  static constexpr PP_ElementaryStream_Type_Samsung Type =
      PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_VIDEO;
  static int32_t SetStreamInfo(PepperPlayerAdapterInterface* player,
                               const StreamInfo& player_stream_info) {
    return player->SetVideoStreamInfo(player_stream_info);
  }
};

class PepperVideoElementaryStreamPrivateTizen
    : public PepperElementaryStreamPrivateTizen<
          PepperVideoElementaryStreamPrivate,
          PepperTizenVideoStreamTraits> {
 public:
  static scoped_refptr<PepperVideoElementaryStreamPrivateTizen> Create(
      PepperESDataSourcePrivate* es_data_source);
  ~PepperVideoElementaryStreamPrivateTizen() override;

  void InitializeDone(PP_StreamInitializationMode mode,
                      const ppapi::VideoCodecConfig& config,
                      const base::Callback<void(int32_t)>& callback) override;

  void AppendPacket(std::unique_ptr<ppapi::ESPacket> packet,
                    const base::Callback<void(int32_t)>& callback) override;

  void AppendEncryptedPacket(
      std::unique_ptr<ppapi::EncryptedESPacket> packet,
      const base::Callback<void(int32_t)>& callback) override;

 private:
  using BaseClass =
      PepperElementaryStreamPrivateTizen<PepperVideoElementaryStreamPrivate,
                                         PepperTizenVideoStreamTraits>;

  explicit PepperVideoElementaryStreamPrivateTizen(
      PepperESDataSourcePrivate* es_data_source);

  void SubmitCodecExtraData(const ppapi::ESPacket* packet);
  void ExtractCodecExtraData(const ppapi::VideoCodecConfig& config);
  void ExtractH264ExtraData(const ppapi::VideoCodecConfig& config);
  void ExtractH265ExtraData(const ppapi::VideoCodecConfig& config);

  std::vector<uint8_t> video_extra_data_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_VIDEO_ELEMENTARY_STREAM_PRIVATE_TIZEN_H_
