// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/formats/mp4/avc.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "base/logging.h"
#include "media/base/decrypt_config.h"
#include "media/formats/mp4/box_definitions.h"
#include "media/formats/mp4/box_reader.h"

namespace media {
namespace mp4 {

static const uint8_t kAnnexBStartCode[] = {0, 0, 0, 1};
static const int kAnnexBStartCodeSize = 4;

static bool ConvertAVCToAnnexBInPlaceForLengthSize4(std::vector<uint8_t>* buf) {
  const int kLengthSize = 4;
  size_t pos = 0;
  while (pos + kLengthSize < buf->size()) {
    uint32_t nal_length = (*buf)[pos];
    nal_length = (nal_length << 8) + (*buf)[pos+1];
    nal_length = (nal_length << 8) + (*buf)[pos+2];
    nal_length = (nal_length << 8) + (*buf)[pos+3];

    if (nal_length == 0) {
      DVLOG(3) << "nal_length is 0";
      return false;
    }

    std::copy(kAnnexBStartCode, kAnnexBStartCode + kAnnexBStartCodeSize,
              buf->begin() + pos);
    pos += kLengthSize + nal_length;
  }
  return pos == buf->size();
}

// static
int AVC::FindSubsampleIndex(const std::vector<uint8_t>& buffer,
                            const std::vector<SubsampleEntry>* subsamples,
                            const uint8_t* ptr) {
  DCHECK(ptr >= &buffer[0]);
  DCHECK(ptr <= &buffer[buffer.size()-1]);
  if (!subsamples || subsamples->empty())
    return 0;

  const uint8_t* p = &buffer[0];
  for (size_t i = 0; i < subsamples->size(); ++i) {
    p += (*subsamples)[i].clear_bytes + (*subsamples)[i].cypher_bytes;
    if (p > ptr)
      return i;
  }
  NOTREACHED();
  return 0;
}

// static
bool AVC::ConvertFrameToAnnexB(int length_size,
                               std::vector<uint8_t>* buffer,
                               std::vector<SubsampleEntry>* subsamples) {
  RCHECK(length_size == 1 || length_size == 2 || length_size == 4);
  DVLOG(5) << __func__ << " length_size=" << length_size
           << " buffer->size()=" << buffer->size()
           << " subsamples=" << (subsamples ? subsamples->size() : 0);

  if (length_size == 4)
    return ConvertAVCToAnnexBInPlaceForLengthSize4(buffer);

  std::vector<uint8_t> temp;
  temp.swap(*buffer);
  buffer->reserve(temp.size() + 32);

  size_t pos = 0;
  while (pos + length_size < temp.size()) {
    int nal_length = temp[pos];
    if (length_size == 2) nal_length = (nal_length << 8) + temp[pos+1];
    pos += length_size;

    if (nal_length == 0) {
      DVLOG(3) << "nal_length is 0";
      return false;
    }

    RCHECK(pos + nal_length <= temp.size());
    buffer->insert(buffer->end(), kAnnexBStartCode,
                   kAnnexBStartCode + kAnnexBStartCodeSize);
    if (subsamples && !subsamples->empty()) {
      uint8_t* buffer_pos = &(*(buffer->end() - kAnnexBStartCodeSize));
      int subsample_index = FindSubsampleIndex(*buffer, subsamples, buffer_pos);
      // We've replaced NALU size value with an AnnexB start code.
      int size_adjustment = kAnnexBStartCodeSize - length_size;
      (*subsamples)[subsample_index].clear_bytes += size_adjustment;
    }
    buffer->insert(buffer->end(), temp.begin() + pos,
                   temp.begin() + pos + nal_length);
    pos += nal_length;
  }
  return pos == temp.size();
}

// static
bool AVC::InsertParamSetsAnnexB(const AVCDecoderConfigurationRecord& avc_config,
                                std::vector<uint8_t>* buffer,
                                std::vector<SubsampleEntry>* subsamples) {
  std::unique_ptr<H264Parser> parser(new H264Parser());
  const uint8_t* start = &(*buffer)[0];
  parser->SetEncryptedStream(start, buffer->size(), *subsamples);

  H264NALU nalu;
  if (parser->AdvanceToNextNALU(&nalu) != H264Parser::kOk)
    return false;

  std::vector<uint8_t>::iterator config_insert_point = buffer->begin();

  if (nalu.nal_unit_type == H264NALU::kAUD) {
    // Move insert point to just after the AUD.
    config_insert_point += (nalu.data + nalu.size) - start;
  }

  // Clear |parser| and |start| since they aren't needed anymore and
  // will hold stale pointers once the insert happens.
  parser.reset();
  start = NULL;

  std::vector<uint8_t> param_sets;
  RCHECK(AVC::ConvertConfigToAnnexB(avc_config, &param_sets));

  if (subsamples && !subsamples->empty()) {
    int subsample_index = FindSubsampleIndex(*buffer, subsamples,
                                             &(*config_insert_point));
    // Update the size of the subsample where SPS/PPS is to be inserted.
    (*subsamples)[subsample_index].clear_bytes += param_sets.size();
  }

  buffer->insert(config_insert_point,
                 param_sets.begin(), param_sets.end());
  return true;
}

// static
bool AVC::ConvertConfigToAnnexB(const AVCDecoderConfigurationRecord& avc_config,
                                std::vector<uint8_t>* buffer) {
  DCHECK(buffer->empty());
  buffer->clear();
  int total_size = 0;
  for (size_t i = 0; i < avc_config.sps_list.size(); i++)
    total_size += avc_config.sps_list[i].size() + kAnnexBStartCodeSize;
  for (size_t i = 0; i < avc_config.pps_list.size(); i++)
    total_size += avc_config.pps_list[i].size() + kAnnexBStartCodeSize;
  buffer->reserve(total_size);

  for (size_t i = 0; i < avc_config.sps_list.size(); i++) {
    buffer->insert(buffer->end(), kAnnexBStartCode,
                kAnnexBStartCode + kAnnexBStartCodeSize);
    buffer->insert(buffer->end(), avc_config.sps_list[i].begin(),
                avc_config.sps_list[i].end());
  }

  for (size_t i = 0; i < avc_config.pps_list.size(); i++) {
    buffer->insert(buffer->end(), kAnnexBStartCode,
                   kAnnexBStartCode + kAnnexBStartCodeSize);
    buffer->insert(buffer->end(), avc_config.pps_list[i].begin(),
                   avc_config.pps_list[i].end());
  }
  return true;
}


// static
std::vector<SubsampleEntry> AVC::CreateNALSubsamples(std::vector<uint8_t>* buffer)
// Sparse encrypted NAL units do not require emulation prevention byte application after encryption
// since their boundaries are delineated by NALu length headers.  However, when converted to annexb
// that information is lost, so unless subsample ranges are provided to the parser
// the raw bitstream cannot be safely parsed. This function will create a subsample array
// of the NALu ranges that can be used to guard against start code emulation while populating the
// true encryption subsample ranges.
{
  const int kLengthSize = 4;
  size_t pos = 0;
  std::vector<SubsampleEntry> subsamples;

  while (pos + kLengthSize < buffer->size() - 2) {
    uint32_t nal_length = (*buffer)[pos];
    nal_length = (nal_length << 8) + (*buffer)[pos+1];
    nal_length = (nal_length << 8) + (*buffer)[pos+2];
    nal_length = (nal_length << 8) + (*buffer)[pos+3];

    subsamples.emplace_back();
    subsamples.back().clear_bytes = kLengthSize + 2; // parser wants 2 bytes beyond length
    subsamples.back().cypher_bytes = nal_length - 2;

    if (nal_length == 0) {
      DVLOG(3) << "nal_length is 0";
      break;
    }

    pos += kLengthSize + nal_length;
  }
  return subsamples;
}

// static
bool AVC::CreateSubsampleEncryptionEntries(media::H264Parser& parser,
                                           std::vector<uint8_t>* buffer,
                                           std::vector<SubsampleEntry>* subsamples,
                                           std::vector<SubsampleEntry>& nal_subsamples) {

  media::H264Parser::Result result;
  media::H264NALU nalu;

  unsigned char *data = buffer->data();
  unsigned char *nal_offset = data;
  uint32_t clear_bytes = 0;
  off_t data_size = buffer->size();
  uint32_t total_used = 0;

  parser.SetEncryptedStream(data, data_size, nal_subsamples);
  subsamples->clear();

  while ((result = parser.AdvanceToNextNALU(&nalu)) != media::H264Parser::kEOStream){
    if (result == media::H264Parser::kOk) {
      if (nalu.nal_unit_type == media::H264NALU::kNonIDRSlice || nalu.nal_unit_type == media::H264NALU::kIDRSlice) {
        media::H264SliceHeader slice_header;
        parser.ParseSliceHeader(nalu, &slice_header);

        subsamples->emplace_back();
        subsamples->back().clear_bytes = nalu.data - nal_offset + slice_header.header_byte_aligned_size_with_epb + clear_bytes;
        subsamples->back().cypher_bytes = nalu.size - slice_header.header_byte_aligned_size_with_epb;

        total_used += subsamples->back().clear_bytes;
        total_used += subsamples->back().cypher_bytes;

        clear_bytes = 0;
      } else {
        if (nalu.nal_unit_type == media::H264NALU::kSPS) {
          int sps_id = 0;
          result = parser.ParseSPS(&sps_id);
          DCHECK(result == media::H264Parser::kOk);
        } else if (nalu.nal_unit_type == media::H264NALU::kPPS) {
          int pps_id = 0;
          result = parser.ParsePPS(&pps_id);
          DCHECK(result == media::H264Parser::kOk);
        }
        clear_bytes += nalu.nalu_size_with_start_code;
      }
      nal_offset += nalu.nalu_size_with_start_code;
    } else {
      VLOG(1) << "Unexpected H264 parser result " << result;
      return false;
    }
  }

  if (clear_bytes > 0) {
    subsamples->emplace_back();
    subsamples->back().clear_bytes = clear_bytes;
    subsamples->back().cypher_bytes = 0;
    total_used += clear_bytes;
  }

  if (total_used != buffer->size()) {
    VLOG(1) << "Didn't use enough";
    return false;
  }

  return true;
}

// Verifies AnnexB NALU order according to ISO/IEC 14496-10 Section 7.4.1.2.3
bool AVC::IsValidAnnexB(const std::vector<uint8_t>& buffer,
                        const std::vector<SubsampleEntry>& subsamples) {
  return IsValidAnnexB(&buffer[0], buffer.size(), subsamples);
}

bool AVC::IsValidAnnexB(const uint8_t* buffer,
                        size_t size,
                        const std::vector<SubsampleEntry>& subsamples) {
  DVLOG(3) << __func__;
  DCHECK(buffer);

  if (size == 0)
    return true;

  H264Parser parser;
  parser.SetEncryptedStream(buffer, size, subsamples);

  typedef enum {
    kAUDAllowed,
    kBeforeFirstVCL,  // VCL == nal_unit_types 1-5
    kAfterFirstVCL,
    kEOStreamAllowed,
    kNoMoreDataAllowed,
  } NALUOrderState;

  H264NALU nalu;
  NALUOrderState order_state = kAUDAllowed;
  int last_nalu_type = H264NALU::kUnspecified;
  bool done = false;
  while (!done) {
    switch (parser.AdvanceToNextNALU(&nalu)) {
      case H264Parser::kOk:
        DVLOG(3) << "nal_unit_type " << nalu.nal_unit_type;

        switch (nalu.nal_unit_type) {
          case H264NALU::kAUD:
            if (order_state > kAUDAllowed) {
              DVLOG(1) << "Unexpected AUD in order_state " << order_state;
              return false;
            }
            order_state = kBeforeFirstVCL;
            break;

          case H264NALU::kSEIMessage:
          case H264NALU::kReserved14:
          case H264NALU::kReserved15:
          case H264NALU::kReserved16:
          case H264NALU::kReserved17:
          case H264NALU::kReserved18:
          case H264NALU::kPPS:
          case H264NALU::kSPS:
            if (order_state > kBeforeFirstVCL) {
              DVLOG(1) << "Unexpected NALU type " << nalu.nal_unit_type
                       << " in order_state " << order_state;
              return false;
            }
            order_state = kBeforeFirstVCL;
            break;

          case H264NALU::kSPSExt:
            if (last_nalu_type != H264NALU::kSPS) {
              DVLOG(1) << "SPS extension does not follow an SPS.";
              return false;
            }
            break;

          case H264NALU::kNonIDRSlice:
          case H264NALU::kSliceDataA:
          case H264NALU::kSliceDataB:
          case H264NALU::kSliceDataC:
          case H264NALU::kIDRSlice:
            if (order_state > kAfterFirstVCL) {
              DVLOG(1) << "Unexpected VCL in order_state " << order_state;
              return false;
            }
            order_state = kAfterFirstVCL;
            break;

          case H264NALU::kCodedSliceAux:
            if (order_state != kAfterFirstVCL) {
              DVLOG(1) << "Unexpected extension in order_state " << order_state;
              return false;
            }
            break;

          case H264NALU::kEOSeq:
            if (order_state != kAfterFirstVCL) {
              DVLOG(1) << "Unexpected EOSeq in order_state " << order_state;
              return false;
            }
            order_state = kEOStreamAllowed;
            break;

          case H264NALU::kEOStream:
            if (order_state < kAfterFirstVCL) {
              DVLOG(1) << "Unexpected EOStream in order_state " << order_state;
              return false;
            }
            order_state = kNoMoreDataAllowed;
            break;

          case H264NALU::kFiller:
          case H264NALU::kUnspecified:
            if (!(order_state >= kAfterFirstVCL &&
                  order_state < kEOStreamAllowed)) {
              DVLOG(1) << "Unexpected NALU type " << nalu.nal_unit_type
                       << " in order_state " << order_state;
              return false;
            }
            break;

          default:
            DCHECK_GE(nalu.nal_unit_type, 20);
            if (nalu.nal_unit_type >= 20 && nalu.nal_unit_type <= 31 &&
                order_state != kAfterFirstVCL) {
              DVLOG(1) << "Unexpected NALU type " << nalu.nal_unit_type
                       << " in order_state " << order_state;
              return false;
            }
        }
        last_nalu_type = nalu.nal_unit_type;
        break;

      case H264Parser::kInvalidStream:
        return false;

      case H264Parser::kUnsupportedStream:
        NOTREACHED() << "AdvanceToNextNALU() returned kUnsupportedStream!";
        return false;

      case H264Parser::kEOStream:
        done = true;
    }
  }

  return order_state >= kAfterFirstVCL;
}

AVCBitstreamConverter::AVCBitstreamConverter(
    std::unique_ptr<AVCDecoderConfigurationRecord> avc_config)
    : avc_config_(std::move(avc_config)), parser_(new H264Parser()) {
  DCHECK(avc_config_);
#if BUILDFLAG(ENABLE_DOLBY_VISION_DEMUXING)
  disable_validation_ = false;
#endif  // BUILDFLAG(ENABLE_DOLBY_VISION_DEMUXING)
}

AVCBitstreamConverter::~AVCBitstreamConverter() {
}

bool AVCBitstreamConverter::ConvertFrame(
    std::vector<uint8_t>* frame_buf,
    bool is_keyframe,
    std::vector<SubsampleEntry>* subsamples,
    bool need_subsamples) const {

  std::vector<SubsampleEntry> nal_subsamples;
  if (need_subsamples) {
    // has to be done before ConvertFrameToAnnexB
    nal_subsamples = AVC::CreateNALSubsamples(frame_buf);
  }

  // Convert the AVC NALU length fields to Annex B headers, as expected by
  // decoding libraries. Since this may enlarge the size of the buffer, we also
  // update the clear byte count for each subsample if encryption is used to
  // account for the difference in size between the length prefix and Annex B
  // start code.
  RCHECK(AVC::ConvertFrameToAnnexB(avc_config_->length_size, frame_buf,
                                   subsamples));

  if (is_keyframe) {
    // If this is a keyframe, we (re-)inject SPS and PPS headers at the start of
    // a frame. If subsample info is present, we also update the clear byte
    // count for that first subsample.
    RCHECK(AVC::InsertParamSetsAnnexB(
        *avc_config_, frame_buf,
        need_subsamples ? &nal_subsamples : subsamples));
  }

  if (need_subsamples) {
    // call after InsertParamSetsAnnexB so the parser can pick up the parameter
    // sets
    AVC::CreateSubsampleEncryptionEntries(*parser_, frame_buf, subsamples,
                                          nal_subsamples);
  }

  return true;
}

bool AVCBitstreamConverter::IsValid(
    std::vector<uint8_t>* frame_buf,
    std::vector<SubsampleEntry>* subsamples) const {
#if BUILDFLAG(ENABLE_DOLBY_VISION_DEMUXING)
  if (disable_validation_)
    return true;
#endif  // BUILDFLAG(ENABLE_DOLBY_VISION_DEMUXING)
  return AVC::IsValidAnnexB(*frame_buf, *subsamples);
}

}  // namespace mp4
}  // namespace media
