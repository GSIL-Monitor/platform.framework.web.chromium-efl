// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_video_elementary_stream_private_tizen.h"

#include <utility>

#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_player_utils_tizen.h"

using std::make_pair;
using std::pair;
using std::vector;
using content::pepper_player_utils::ToHexString;

#define RETURN_IF_TOO_SHORT(cond)           \
  do {                                      \
    if (cond) {                             \
      LOG(ERROR) << "Extra data too short"; \
      return;                               \
    }                                       \
  } while (false)

namespace content {

namespace {

vector<uint8_t> WriteAsBytesMSB(uint32_t val, int32_t max_bytes) {
  vector<uint8_t> ret(max_bytes, 0);

  for (--max_bytes; max_bytes >= 0 && val > 0; --max_bytes) {
    ret[max_bytes] = static_cast<uint8_t>(val & 0xFFu);
    val >>= 8;
  }

  return ret;
}

void AppendData(vector<uint8_t>* to_vec, const vector<uint8_t>& from_vec) {
  to_vec->insert(to_vec->end(), from_vec.begin(), from_vec.end());
}

void AppendData(vector<uint8_t>* to_vec, uint32_t size, const uint8_t* data) {
  to_vec->insert(to_vec->end(), data, data + size);
}

template <typename T>
T ReadUnsigned(const uint8_t** data) {
  T out = 0;
  for (size_t i = 0; i < sizeof(T); ++i) {
    out <<= 8;
    out |= *(*data)++;
  }
  return out;
}

pair<vector<vector<uint8_t>>, bool> ReadH264ParameterSets(const uint8_t** data,
                                                          uint8_t sets_count,
                                                          const uint8_t* end) {
  auto out = make_pair(vector<vector<uint8_t>>(sets_count), true);
  const uint8_t* extra_data = *data;
  for (int i = 0; i < sets_count; i++) {
    if (end < extra_data + 2) {
      LOG(ERROR) << "extra data too short";
      return make_pair(vector<vector<uint8_t>>{}, false);
    }

    uint32_t length = ReadUnsigned<uint16_t>(&extra_data);
    if (end < extra_data + length) {
      LOG(ERROR) << "extra data too short";
      return make_pair(vector<vector<uint8_t>>{}, false);
    }

    out.first[i].clear();
    AppendData(&out.first[i], length, extra_data);
    extra_data += length;
  }

  *data = extra_data;
  return out;
}

void OnSubmitCodecExtraData(int32_t result) {
  if (result != PP_OK)
    LOG(ERROR) << "Submitting codec extradata failed with error: " << result;
}

}  // namespace

// static
scoped_refptr<PepperVideoElementaryStreamPrivateTizen>
PepperVideoElementaryStreamPrivateTizen::Create(
    PepperESDataSourcePrivate* source) {
  return scoped_refptr<PepperVideoElementaryStreamPrivateTizen>{
      new PepperVideoElementaryStreamPrivateTizen(source)};
}

PepperVideoElementaryStreamPrivateTizen::
    PepperVideoElementaryStreamPrivateTizen(PepperESDataSourcePrivate* source)
    : PepperElementaryStreamPrivateTizen<PepperVideoElementaryStreamPrivate,
                                         PepperTizenVideoStreamTraits>(source) {
}

void PepperVideoElementaryStreamPrivateTizen::InitializeDone(
    PP_StreamInitializationMode mode,
    const ppapi::VideoCodecConfig& config,
    const base::Callback<void(int32_t)>& callback) {
  ExtractCodecExtraData(config);
  BaseClass::InitializeDone(mode, config, callback);
}

void PepperVideoElementaryStreamPrivateTizen::AppendPacket(
    std::unique_ptr<ppapi::ESPacket> packet,
    const base::Callback<void(int32_t)>& callback) {
  DCHECK(packet != nullptr);
  SubmitCodecExtraData(packet.get());
  BaseClass::AppendPacket(std::move(packet), callback);
}

void PepperVideoElementaryStreamPrivateTizen::AppendEncryptedPacket(
    std::unique_ptr<ppapi::EncryptedESPacket> packet,
    const base::Callback<void(int32_t)>& callback) {
  DCHECK(packet != nullptr);
  SubmitCodecExtraData(packet.get());
  BaseClass::AppendEncryptedPacket(std::move(packet), callback);
}

void PepperVideoElementaryStreamPrivateTizen::SubmitCodecExtraData(
    const ppapi::ESPacket* packet) {
  if (video_extra_data_.empty() || !packet->KeyFrame())
    return;

  // In case of H264 videos some streams may have sent SPSes and PPSes
  // together with video data, but fortunately our decoder deals with
  // redundant SPSes and PPSes well.
  PP_ESPacket pp_pkt;
  pp_pkt.pts = packet->Pts();
  pp_pkt.dts = packet->Dts();
  pp_pkt.duration = 0;
  pp_pkt.is_key_frame = PP_TRUE;
  pp_pkt.size = static_cast<uint32_t>(video_extra_data_.size());
  pp_pkt.buffer = video_extra_data_.data();

  BaseClass::AppendPacket(
      std::unique_ptr<ppapi::ESPacket>(new ppapi::ESPacket(pp_pkt)),
      base::Bind(OnSubmitCodecExtraData));
}

PepperVideoElementaryStreamPrivateTizen::
    ~PepperVideoElementaryStreamPrivateTizen() {}

void PepperVideoElementaryStreamPrivateTizen::ExtractCodecExtraData(
    const ppapi::VideoCodecConfig& config) {
  video_extra_data_.clear();
  switch (config.codec) {
    case PP_VIDEOCODEC_TYPE_SAMSUNG_H264:
      ExtractH264ExtraData(config);
      break;
    case PP_VIDEOCODEC_TYPE_SAMSUNG_H265:
      ExtractH265ExtraData(config);
      break;
    default:
      break;
  }

  LOG(INFO) << "codec: " << config.codec;
  LOG(INFO) << "codec_extra_data(length: " << config.extra_data.size()
            << "): " << ToHexString(config.extra_data);
  LOG(INFO) << "video_extra_data(length: " << video_extra_data_.size()
            << "): " << ToHexString(video_extra_data_);
}

// TODO(a.bujalski) Add support SVC (ISO/IEC 14496-15 ch. 6.4.2.2) data.
// TODO(a.bujalski) Add support MVC (ISO/IEC 14496-15 ch. 7.5.2.2) data.
// In case of H264 as VideoCodecConfig::extraData we will have avcC box, which
// according to ISO/IEC 14496-15 chapter 5.3.3.1.2 has following structure:
//
// aligned(8) class AVCDecoderConfigurationRecord {
//   unsigned int(8) configurationVersion = 1;
//   unsigned int(8) AVCProfileIndication;
//   unsigned int(8) profile_compatibility;
//   unsigned int(8) AVCLevelIndication;
//   bit(6) reserved = '111111'b;
//   unsigned int(2) lengthSizeMinusOne;
//   bit(3) reserved = '111'b;
//   unsigned int(5) numOfSequenceParameterSets;
//   for (i=0; i< numOfSequenceParameterSets; i++) {
//     unsigned int(16) sequenceParameterSetLength ;
//     bit(8*sequenceParameterSetLength) sequenceParameterSetNALUnit;
//   }
//   unsigned int(8) numOfPictureParameterSets;
//   for (i=0; i< numOfPictureParameterSets; i++) {
//     unsigned int(16) pictureParameterSetLength;
//     bit(8*pictureParameterSetLength) pictureParameterSetNALUnit;
//   }
//   if( profile_idc == 100 || profile_idc == 110 ||
//       profile_idc == 122 || profile_idc == 144 )
//   {
//     bit(6) reserved = '111111'b;
//     unsigned int(2) chroma_format;
//     bit(5) reserved = '11111'b;
//     unsigned int(3) bit_depth_luma_minus8;
//     bit(5) reserved = '11111'b;
//     unsigned int(3) bit_depth_chroma_minus8;
//     unsigned int(8) numOfSequenceParameterSetExt;
//     for (i=0; i< numOfSequenceParameterSetExt; i++) {
//       unsigned int(16) sequenceParameterSetExtLength;
//       bit(8*sequenceParameterSetExtLength) sequenceParameterSetExtNALUnit;
//     }
//   }
// }
//
// If we want to change representations with different codec extra
// configuration in adaptive streaming scenarios, then we have to modify each
// packet data by inserting before video samples SPSes and PPSes in the
// following way:
//   for each SPS:
//     write length of SPS on lengthSizeMinusOne + 1 bytes in MSB format
//       (BigEndian)
//     write SPS NAL data (without any modifications)
//   for each PPS: (do similar operation as for PPS)
//     write length of PPS on lengthSizeMinusOne + 1 bytes in MSB format
//     (BigEndian)
//     write PPS NAL data (without any modifications)
//   append video packet data
//
// For example:
// - VideoCodecConfig::extra_data_:
//       01 4D 40 20 FF E1 00 0C
//       67 4D 40 20 96 52 80 A0 0B 76 02 05
//       01 00 04 68 EF 38 80
// - length_size: 4
// - SPS count 1, SPS data: 67 4D 40 20 96 52 80 A0 0B 76 02 05
// - PPS count 1, PPS data: 68 EF 38 80
// - modified packet structure (in hex):
//   00 00 00 0C 67 4D 40 20 96 52 80 A0 0B 76 02 05 00 00 00 04 68 EF 38 80
//  |           |                                   |           |
//  |           |                                   |           |
//  |           |                                   |           | PPS NAL
//  |           |                                   |
//  |           |                                   | PPS length (4 bytes)
//  |           |
//  |           | SPS NAL
//  |
//  | SPS length (4 bytes)
//  after that header original ES packet bytes are appended
void PepperVideoElementaryStreamPrivateTizen::ExtractH264ExtraData(
    const ppapi::VideoCodecConfig& config) {
  const uint8_t* extra_data = config.extra_data.data();
  size_t extra_data_size = config.extra_data.size();

  if (extra_data_size < 6) {  // Min first 5 byte + num_sps
    LOG(ERROR) << "extra_data is too short to pass valid SPS/PPS header";
    return;
  }

  const uint8_t* end = extra_data + extra_data_size;
  uint8_t version = *extra_data++;
  uint8_t profile_indication = *extra_data++;
  uint8_t profile_compatibility = *extra_data++;
  uint8_t avc_level = *extra_data++;
  ALLOW_UNUSED_LOCAL(version);
  ALLOW_UNUSED_LOCAL(profile_indication);
  ALLOW_UNUSED_LOCAL(profile_compatibility);
  ALLOW_UNUSED_LOCAL(avc_level);

  uint8_t length_size = *extra_data++;
  if ((length_size & 0xFCu) != 0xFCu) {
    // Be liberal in what you accept..., so just log error
    LOG(WARNING) << "Not all reserved bits in length size filed are set to 1";
  }

  length_size = (length_size & 0x3u) + 1;
  uint8_t num_sps = *extra_data++;
  if ((num_sps & 0xE0u) != 0xE0u) {
    // Be liberal in what you accept..., so just log error
    LOG(WARNING) << "Wrong SPS count format.";
  }

  num_sps &= 0x1Fu;
  auto sps_list = ReadH264ParameterSets(&extra_data, num_sps, end);
  if (!sps_list.second)
    return;

  if (end <= extra_data) {
    LOG(ERROR) << "Extra data too short";
    return;
  }

  uint8_t num_pps = *extra_data++;
  auto pps_list = ReadH264ParameterSets(&extra_data, num_pps, end);
  if (!pps_list.second)
    return;

  video_extra_data_.clear();
  for (const auto& sps : sps_list.first) {
    AppendData(&video_extra_data_, WriteAsBytesMSB(sps.size(), length_size));
    AppendData(&video_extra_data_, sps);
  }

  for (const auto& pps : pps_list.first) {
    AppendData(&video_extra_data_, WriteAsBytesMSB(pps.size(), length_size));
    AppendData(&video_extra_data_, pps);
  }
}

// In case of H265 as VideoCodecConfig::extraData we will have box, which
// according to ISO/IES 14496-15 chapter 8.3.3.1.2 has following structure:
//
// aligned(8) class HEVCDecoderConfigurationRecord {
//   unsigned int(8) configurationVersion = 1;
//   unsigned int(2) general_profile_space;
//   unsigned int(1) general_tier_flag;
//   unsigned int(5) general_profile_idc;
//   unsigned int(32) general_profile_compatibility_flags;
//   unsigned int(48) general_constraint_indicator_flags;
//   unsigned int(8) general_level_idc;
//   bit(4) reserved = ‘1111’b;
//   unsigned int(12) min_spatial_segmentation_idc;
//   bit(6) reserved = ‘111111’b;
//   unsigned int(2) parallelismType;
//   bit(6) reserved = ‘111111’b;
//   unsigned int(2) chromaFormat;
//   bit(5) reserved = ‘11111’b;
//   unsigned int(3) bitDepthLumaMinus8;
//   bit(5) reserved = ‘11111’b;
//   unsigned int(3) bitDepthChromaMinus8;
//   bit(16) avgFrameRate;
//   bit(2) constantFrameRate;
//   bit(3) numTemporalLayers;
//   bit(1) temporalIdNested;
//   unsigned int(2) lengthSizeMinusOne;
//   unsigned int(8) numOfArrays;
//   for (j=0; j < numOfArrays; j++) {
//     bit(1) array_completeness;
//     unsigned int(1) reserved = 0;
//     unsigned int(6) NAL_unit_type;
//     unsigned int(16) numNalus;
//       for (i=0; i< numNalus; i++) {
//         unsigned int(16) nalUnitLength;
//         bit(8*nalUnitLength) nalUnit;
//       }
//     }
//   }
//
// "NAL_unit_type indicates the type of the NAL units in the following array
// (which must be all of that type); it takes a value as defined
// in ISO/IEC 23008‐2; it is restricted to take one of the
// values indicating a VPS, SPS, PPS, or SEI NAL unit;"
// (from ISO/IES 14496-15 chapter 8.3.3.1.2)
//
// If we want to change representations with different codec extra
// configuration in adaptive streaming scenarios, then we have to modify each
// packet data by inserting before video samples extracted NALs in the
// following way:
//   for each nalUnit:
//     write nalUnitLength on lengthSizeMinusOne + 1 bytes in MSB format
//       (BigEndian)
//     write nalUnit (without any modifications)
//   append video packet data
void PepperVideoElementaryStreamPrivateTizen::ExtractH265ExtraData(
    const ppapi::VideoCodecConfig& config) {
  constexpr uint8_t kBytesToSkip = 21u;
  const uint8_t* extra_data = config.extra_data.data();
  size_t extra_data_size = config.extra_data.size();

  RETURN_IF_TOO_SHORT(extra_data_size < kBytesToSkip + 2);

  const uint8_t* end = extra_data + extra_data_size;
  extra_data += kBytesToSkip;
  uint8_t length_size = (*extra_data++ & 0x3u) + 1;
  uint8_t num_of_arrays = *extra_data++;

  std::vector<uint8_t> extracted_data;
  for (uint8_t j = 0; j < num_of_arrays; ++j) {
    RETURN_IF_TOO_SHORT(end < extra_data + 3);

    uint8_t nal_unit_type = *extra_data++ & 0x3Fu;
    ALLOW_UNUSED_LOCAL(nal_unit_type);

    uint16_t num_nalus = ReadUnsigned<uint16_t>(&extra_data);
    for (uint16_t i = 0; i < num_nalus; ++i) {
      RETURN_IF_TOO_SHORT(end < extra_data + 2);
      uint16_t nal_unit_length = ReadUnsigned<uint16_t>(&extra_data);
      RETURN_IF_TOO_SHORT(end < extra_data + nal_unit_length);

      AppendData(&extracted_data,
                 WriteAsBytesMSB(nal_unit_length, length_size));
      AppendData(&extracted_data, nal_unit_length, extra_data);
      extra_data += nal_unit_length;
    }
  }

  std::swap(video_extra_data_, extracted_data);
}

}  // namespace content
