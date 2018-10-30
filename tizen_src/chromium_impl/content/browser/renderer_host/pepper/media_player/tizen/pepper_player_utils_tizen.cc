// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_player_utils_tizen.h"

#include <iomanip>
#include <sstream>

namespace content {

namespace pepper_player_utils {

bool IsValid(PP_ElementaryStream_Type_Samsung type) {
  return type > PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_UNKNOWN &&
         type < PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_MAX;
}

std::string ToString(PP_ElementaryStream_Type_Samsung type) {
  switch (type) {
    case PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_UNKNOWN:
      return "PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_UNKNOWN (-1)";
    case PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_VIDEO:
      return "PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_VIDEO (0)";
    case PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_AUDIO:
      return "PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_AUDIO (1)";
    case PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_TEXT:
      return "PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_TEXT (2)";
    case PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_MAX:
      return "PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_MAX (3)";
    default:
      return "<invalid PP_ElementaryStream_Type_Samsung value: " +
             std::to_string(static_cast<int32_t>(type)) + ">";
  }
}

std::string ToHexString(size_t size, const uint8_t* data) {
  std::ostringstream oss;
  oss << std::hex;
  for (size_t i = 0; i < size; ++i)
    oss << " 0x" << std::setw(2) << std::setfill('0')
        << static_cast<unsigned>(data[i]);

  return oss.str();
}

std::string ToHexString(const std::vector<uint8_t>& data) {
  return ToHexString(data.size(), data.data());
}

std::string ToHexString(size_t size, const void* data) {
  return ToHexString(size, reinterpret_cast<const uint8_t*>(data));
}

unsigned GetChannelsCount(PP_ChannelLayout_Samsung layout) {
  switch (layout) {
    case PP_CHANNEL_LAYOUT_SAMSUNG_NONE:
    case PP_CHANNEL_LAYOUT_SAMSUNG_UNSUPPORTED:
      return 0;
    case PP_CHANNEL_LAYOUT_SAMSUNG_MONO:
      return 1;
    case PP_CHANNEL_LAYOUT_SAMSUNG_STEREO:
      return 2;
    case PP_CHANNEL_LAYOUT_SAMSUNG_2_1:
    case PP_CHANNEL_LAYOUT_SAMSUNG_SURROUND:
      return 3;
    case PP_CHANNEL_LAYOUT_SAMSUNG_4_0:
    case PP_CHANNEL_LAYOUT_SAMSUNG_2_2:
    case PP_CHANNEL_LAYOUT_SAMSUNG_QUAD:
      return 4;
    case PP_CHANNEL_LAYOUT_SAMSUNG_5_0:
      return 5;
    case PP_CHANNEL_LAYOUT_SAMSUNG_5_1:
      return 6;
    case PP_CHANNEL_LAYOUT_SAMSUNG_5_0_BACK:
      return 5;
    case PP_CHANNEL_LAYOUT_SAMSUNG_5_1_BACK:
      return 6;
    case PP_CHANNEL_LAYOUT_SAMSUNG_7_0:
      return 7;
    case PP_CHANNEL_LAYOUT_SAMSUNG_7_1:
    case PP_CHANNEL_LAYOUT_SAMSUNG_7_1_WIDE:
      return 8;
    case PP_CHANNEL_LAYOUT_SAMSUNG_STEREO_DOWNMIX:
      return 2;
    case PP_CHANNEL_LAYOUT_SAMSUNG_2POINT1:
      return 3;
    case PP_CHANNEL_LAYOUT_SAMSUNG_3_1:
      return 4;
    case PP_CHANNEL_LAYOUT_SAMSUNG_4_1:
      return 5;
    case PP_CHANNEL_LAYOUT_SAMSUNG_6_0:
    case PP_CHANNEL_LAYOUT_SAMSUNG_6_0_FRONT:
    case PP_CHANNEL_LAYOUT_SAMSUNG_HEXAGONAL:
      return 6;
    case PP_CHANNEL_LAYOUT_SAMSUNG_6_1:
    case PP_CHANNEL_LAYOUT_SAMSUNG_6_1_BACK:
    case PP_CHANNEL_LAYOUT_SAMSUNG_6_1_FRONT:
    case PP_CHANNEL_LAYOUT_SAMSUNG_7_0_FRONT:
      return 7;
    case PP_CHANNEL_LAYOUT_SAMSUNG_7_1_WIDE_BACK:
    case PP_CHANNEL_LAYOUT_SAMSUNG_OCTAGONAL:
      return 8;
    case PP_CHANNEL_LAYOUT_SAMSUNG_DISCRETE:
      return 1;
    default:
      return 0;
  }
}

const char* GetCodecMimeType(PP_VideoCodec_Type_Samsung pp_type) {
  switch (pp_type) {
    case PP_VIDEOCODEC_TYPE_SAMSUNG_H264:
      return "video/x-h264";
    case PP_VIDEOCODEC_TYPE_SAMSUNG_H265:
      return "video/x-h265";
    case PP_VIDEOCODEC_TYPE_SAMSUNG_MPEG2:
    case PP_VIDEOCODEC_TYPE_SAMSUNG_MPEG4:
      return "video/mpeg";
    case PP_VIDEOCODEC_TYPE_SAMSUNG_VP8:
      return "video/x-vp8";
    case PP_VIDEOCODEC_TYPE_SAMSUNG_VP9:
      return "video/x-vp9";
    case PP_VIDEOCODEC_TYPE_SAMSUNG_WMV1:
    case PP_VIDEOCODEC_TYPE_SAMSUNG_WMV2:
    case PP_VIDEOCODEC_TYPE_SAMSUNG_WMV3:
      return "video/x-wmv";
    default:
      return "";
  }
}

const char* GetCodecMimeType(PP_AudioCodec_Type_Samsung pp_type) {
  switch (pp_type) {
    case PP_AUDIOCODEC_TYPE_SAMSUNG_AAC:
      return "audio/mpeg";
    case PP_AUDIOCODEC_TYPE_SAMSUNG_MP2:
      return "audio/mpeg";
    case PP_AUDIOCODEC_TYPE_SAMSUNG_MP3:
      return "audio/mpeg";
    case PP_AUDIOCODEC_TYPE_SAMSUNG_PCM:
      return "audio/x-raw-int";
    case PP_AUDIOCODEC_TYPE_SAMSUNG_VORBIS:
      return "audio/x-vorbis";
    case PP_AUDIOCODEC_TYPE_SAMSUNG_FLAC:
      return "audio/x-flac";
    case PP_AUDIOCODEC_TYPE_SAMSUNG_AMR_NB:
      return "audio/AMR";
    case PP_AUDIOCODEC_TYPE_SAMSUNG_AMR_WB:
      return "audio/AMR-WB";
    case PP_AUDIOCODEC_TYPE_SAMSUNG_PCM_MULAW:
      return "audio/x-mulaw";
    case PP_AUDIOCODEC_TYPE_SAMSUNG_GSM_MS:
      return "audio/ms-gsm";
    case PP_AUDIOCODEC_TYPE_SAMSUNG_PCM_S16BE:
      return "audio/x-raw";
    case PP_AUDIOCODEC_TYPE_SAMSUNG_PCM_S24BE:
      return "audio/x-raw";
    case PP_AUDIOCODEC_TYPE_SAMSUNG_OPUS:
      return "audio/ogg";
    case PP_AUDIOCODEC_TYPE_SAMSUNG_EAC3:
      return "audio/x-eac3";
    case PP_AUDIOCODEC_TYPE_SAMSUNG_DTS:
      return "audio/x-dts";
    case PP_AUDIOCODEC_TYPE_SAMSUNG_AC3:
      return "audio/x-ac3";
    case PP_AUDIOCODEC_TYPE_SAMSUNG_WMAV1:
      return "audio/x-ms-wma";
    case PP_AUDIOCODEC_TYPE_SAMSUNG_WMAV2:
      return "audio/x-ms-wma";
    default:
      return "";
  }
}

unsigned GetCodecVersion(PP_VideoCodec_Type_Samsung pp_type) {
  switch (pp_type) {
    case PP_VIDEOCODEC_TYPE_SAMSUNG_MPEG2:
      return 2;
    case PP_VIDEOCODEC_TYPE_SAMSUNG_MPEG4:
      return 4;
    case PP_VIDEOCODEC_TYPE_SAMSUNG_WMV1:
      return 1;
    case PP_VIDEOCODEC_TYPE_SAMSUNG_WMV2:
      return 2;
    case PP_VIDEOCODEC_TYPE_SAMSUNG_WMV3:
      return 3;
    case PP_VIDEOCODEC_TYPE_SAMSUNG_H264:
      return 4;
    default:
      return 0;
  }
}

unsigned GetCodecVersion(PP_AudioCodec_Type_Samsung pp_type) {
  switch (pp_type) {
    case PP_AUDIOCODEC_TYPE_SAMSUNG_AAC:
      return 4;
    case PP_AUDIOCODEC_TYPE_SAMSUNG_MP3:
      return 1;
    case PP_AUDIOCODEC_TYPE_SAMSUNG_MP2:
      return 1;
    default:
      return 0;
  }
}

void GetSampleFormat(PP_SampleFormat_Samsung pp_format,
                     unsigned* width,
                     unsigned* depth,
                     unsigned* endianness,
                     bool* signedness) {
  *endianness = 1234;
  switch (pp_format) {
    case PP_SAMPLEFORMAT_SAMSUNG_UNKNOWN:
      *width = 0;
      *depth = 0;
      *signedness = false;
      break;
    case PP_SAMPLEFORMAT_SAMSUNG_U8:
      *width = 8;
      *depth = 8;
      *signedness = false;
      break;
    case PP_SAMPLEFORMAT_SAMSUNG_S16:
      *width = 16;
      *depth = 16;
      *signedness = true;
      break;
    case PP_SAMPLEFORMAT_SAMSUNG_S32:
    case PP_SAMPLEFORMAT_SAMSUNG_F32:
      *width = 32;
      *depth = 32;
      *signedness = true;
      break;
    case PP_SAMPLEFORMAT_SAMSUNG_PLANARS16:
      *width = 16;
      *depth = 16;
      *signedness = true;
      break;
    case PP_SAMPLEFORMAT_SAMSUNG_PLANARF32:
      *width = 32;
      *depth = 32;
      *signedness = true;
      break;
    default:
      *width = 0;
      *depth = 0;
      *signedness = false;
      break;
  }
}

// NOLINTNEXTLINE(runtime/int)
PP_TimeTicks MilisecondsToPPTimeTicks(int mm_player_time_ticks_ms) {
  return static_cast<PP_TimeTicks>(mm_player_time_ticks_ms / 1e3);
}

// NOLINTNEXTLINE(runtime/int)
PP_TimeTicks MilisecondsToPPTimeTicks(unsigned long mm_player_time_ticks_ms) {
  return static_cast<PP_TimeTicks>(mm_player_time_ticks_ms / 1e3);
}

PP_TimeTicks NanosecondsToPPTimeTicks(
    unsigned long long mm_player_time_ticks_ns /* NOLINT(runtime/int) */) {
  return static_cast<PP_TimeTicks>(mm_player_time_ticks_ns / 1e9);
}

}  // namespace pepper_player_utils

}  // namespace content
