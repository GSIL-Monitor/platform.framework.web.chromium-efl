// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_SHARED_IMPL_MEDIA_PLAYER_MEDIA_CODECS_CONFIGS_H_
#define PPAPI_SHARED_IMPL_MEDIA_PLAYER_MEDIA_CODECS_CONFIGS_H_

#include <utility>
#include <vector>

#include "ppapi/c/samsung/pp_media_codecs_samsung.h"
#include "ppapi/c/samsung/pp_media_player_samsung.h"

namespace ppapi {

struct AudioCodecConfig {
  PP_AudioCodec_Type_Samsung codec = PP_AUDIOCODEC_TYPE_SAMSUNG_UNKNOWN;
  PP_AudioCodec_Profile_Samsung profile = PP_AUDIOCODEC_PROFILE_SAMSUNG_UNKNOWN;
  PP_SampleFormat_Samsung sample_format = PP_SAMPLEFORMAT_SAMSUNG_UNKNOWN;
  PP_ChannelLayout_Samsung channel_layout = PP_CHANNEL_LAYOUT_SAMSUNG_NONE;
  int32_t bits_per_channel = -1;
  int32_t samples_per_second = -1;
  std::vector<uint8_t> extra_data;

  bool IsValid() const { return codec != PP_AUDIOCODEC_TYPE_SAMSUNG_UNKNOWN; }
};

struct VideoCodecConfig {
  PP_VideoCodec_Type_Samsung codec = PP_VIDEOCODEC_TYPE_SAMSUNG_UNKNOWN;
  PP_VideoCodec_Profile_Samsung profile = PP_VIDEOCODEC_PROFILE_SAMSUNG_UNKNOWN;
  PP_VideoFrame_Format_Samsung frame_format =
      PP_VIDEOFRAME_FORMAT_SAMSUNG_INVALID;
  PP_Size frame_size = PP_MakeSize(-1, -1);
  std::pair<uint32_t, uint32_t> frame_rate = std::make_pair(0, 1);
  std::vector<uint8_t> extra_data;

  bool IsValid() const { return codec != PP_VIDEOCODEC_TYPE_SAMSUNG_UNKNOWN; }
};

}  // namespace ppapi

#endif  // PPAPI_SHARED_IMPL_MEDIA_PLAYER_MEDIA_CODECS_CONFIGS_H_
