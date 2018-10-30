// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_mmplayer_adapter.h"

#include <capi-system-info/system_info.h>
#include <Ecore.h>

#include <algorithm>
#include <limits>
#include <sstream>
#include <unordered_map>
#include <utility>

#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/string_util.h"
#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_player_utils_tizen.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/samsung/pp_media_codecs_samsung.h"

namespace content {

using pepper_player_utils::ToString;
using pepper_player_utils::GetSampleFormat;
using pepper_player_utils::MilisecondsToPPTimeTicks;
using pepper_player_utils::NanosecondsToPPTimeTicks;
using pepper_player_utils::GetChannelsCount;
using pepper_player_utils::GetCodecVersion;
using pepper_player_utils::GetCodecMimeType;

namespace {
constexpr int32_t kFHDWidth = 1920;
constexpr int32_t kFHDHeight = 1080;
constexpr uint32_t kBitratesRequestCount = 64;
constexpr bool kAccurateSeek = true;
// buffer max size 32MB
constexpr uint32_t kBufferMaxSize = 32 * 1024 * 1024;
// Adapter's buffer size for one track.
constexpr uint32_t kInternalBufferSize = 32 * 1024 * 1024;
// buffer min threshold 10%
constexpr uint32_t kBufferMinThreshold = 10;

template <typename To, typename From>
To ClampToMax(From val) {
  return static_cast<To>(
      std::min(val, static_cast<From>(std::numeric_limits<To>::max())));
}

inline int32_t GetClipForLetterboxSize(float media_clipped_axis,
                                       float viewport_ortho_axis,
                                       float media_ortho_axis) {
  return static_cast<int32_t>(media_clipped_axis * viewport_ortho_axis /
                              media_ortho_axis);
}

inline int32_t GetClipForLetterboxPosition(int32_t viewport_position,
                                           int32_t viewport_length,
                                           int32_t media_length) {
  return viewport_position + ((viewport_length - media_length) / 2);
}

int32_t ConvertToPPError(int error_code) {
  switch (error_code) {
    case PLAYER_ERROR_NONE:
      return PP_OK;
    case PLAYER_ERROR_OUT_OF_MEMORY:
      return PP_ERROR_NOMEMORY;
    case PLAYER_ERROR_INVALID_PARAMETER:
      return PP_ERROR_BADARGUMENT;
    case PLAYER_ERROR_NOT_SUPPORTED_FORMAT:
    case PLAYER_ERROR_NOT_SUPPORTED_FILE:
      return PP_ERROR_FAILED;
    case PLAYER_ERROR_INVALID_STATE:
      return PP_ERROR_FAILED;
    case PLAYER_ERROR_INVALID_OPERATION:
      return PP_ERROR_NOTSUPPORTED;
    case PLAYER_ERROR_FILE_NO_SPACE_ON_DEVICE:
      return PP_ERROR_NOSPACE;
    case PLAYER_ERROR_NO_SUCH_FILE:
      return PP_ERROR_FILENOTFOUND;
    case PLAYER_ERROR_SEEK_FAILED:
      return PP_ERROR_FAILED;
    case PLAYER_ERROR_INVALID_URI:
      return PP_ERROR_ADDRESS_INVALID;
    case PLAYER_ERROR_CONNECTION_FAILED:
      return PP_ERROR_CONNECTION_FAILED;
    case PLAYER_ERROR_SOUND_POLICY:
    case PLAYER_ERROR_DRM_EXPIRED:
    case PLAYER_ERROR_DRM_INFO:
    case PLAYER_ERROR_DRM_NO_LICENSE:
    case PLAYER_ERROR_DRM_FUTURE_USE:
      return PP_ERROR_FAILED;
    case PLAYER_ERROR_DRM_NOT_PERMITTED:
      return PP_ERROR_NOTSUPPORTED;
    case PLAYER_ERROR_RESOURCE_LIMIT:
      return PP_ERROR_NOQUOTA;
    case PLAYER_ERROR_SYNC_PLAY_SERVER_DOWN:
      return PP_ERROR_CONNECTION_CLOSED;
    default:
      return PP_ERROR_FAILED;
  }
}

player_stream_type_e ConvertStreamType(
    PP_ElementaryStream_Type_Samsung stream_type) {
  switch (stream_type) {
    case PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_UNKNOWN:
      return PLAYER_STREAM_TYPE_DEFAULT;
    case PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_VIDEO:
      return PLAYER_STREAM_TYPE_VIDEO;
    case PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_AUDIO:
      return PLAYER_STREAM_TYPE_AUDIO;
    case PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_TEXT:
      return PLAYER_STREAM_TYPE_TEXT;
    default:
      NOTREACHED();
      return PLAYER_STREAM_TYPE_DEFAULT;
  }
}

// NOLINTNEXTLINE(runtime/int)
inline PP_TimeDelta ConvertToPPTimeDelta(int mm_player_time_delta) {
  return static_cast<PP_TimeDelta>(mm_player_time_delta / 1e3);
}

// NOLINTNEXTLINE(runtime/int)
inline int ConvertToMMPlayerTimeDelta(PP_TimeDelta pp_time_delta) {
  return pp_time_delta * 1e3;
}

// NOLINTNEXTLINE(runtime/int)
inline int ConvertToMMPlayerTimeTicks(PP_TimeTicks pp_time_ticks) {
  return pp_time_ticks * 1e3;
}

inline uint64_t ConvertToNanoseconds(PP_TimeTicks pp_time) {
  return static_cast<uint64_t>(max(pp_time, 0.0) * 1e9);
}

player_stream_type_e ConvertToMMPlayerTrackType(
    PP_ElementaryStream_Type_Samsung pp_type) {
  switch (pp_type) {
    case PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_VIDEO:
      return PLAYER_STREAM_TYPE_VIDEO;
    case PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_AUDIO:
      return PLAYER_STREAM_TYPE_AUDIO;
    case PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_TEXT:
      return PLAYER_STREAM_TYPE_TEXT;
    default:
      NOTREACHED();
      return static_cast<player_stream_type_e>(-1);
  }
}

bool IsUTF8(const std::string& encoding) {
  return base::EqualsCaseInsensitiveASCII(encoding, "UTF-8") ||
         base::EqualsCaseInsensitiveASCII(encoding, "csUTF8");
}

player_subtitle_encoding_e ConvertToMMPlayerSubtitleEncoding(
    const std::string& encoding) {
  static const std::unordered_map<std::string, player_subtitle_encoding_e>
      encoding_map{
          {"windows-1250", PLAYER_CENTRAL_EUROPE},
          {"cswindows1250", PLAYER_CENTRAL_EUROPE},
          {"windows-1251", PLAYER_CYRILLIC_ALPHABET},
          {"cswindows1251", PLAYER_CYRILLIC_ALPHABET},
          {"windows-1252", PLAYER_WESTERN_EUROPE},
          {"cswindows1252", PLAYER_WESTERN_EUROPE},
          {"windows-1253", PLAYER_GREEK},
          {"cswindows1253", PLAYER_GREEK},
          {"windows-1254", PLAYER_TURKISH},
          {"cswindows1254", PLAYER_TURKISH},
          {"windows-1255", PLAYER_HEBREW},
          {"cswindows1255", PLAYER_HEBREW},
          {"windows-1256", PLAYER_ARABIC},
          {"cswindows1256", PLAYER_ARABIC},
          {"windows-1257", PLAYER_BALTIC},
          {"cswindows1257", PLAYER_BALTIC},
          {"windows-1258", PLAYER_VIETNAMESE},
          {"cswindows1258", PLAYER_VIETNAMESE},
          {"CP949", PLAYER_KOREAN},
          {"UHC", PLAYER_KOREAN},
          {"windows-874", PLAYER_THAI},
          {"cswindows874", PLAYER_THAI},
          {"GBK", PLAYER_SIMPLIFIED_CHINA},
          {"CP936", PLAYER_SIMPLIFIED_CHINA},
          {"MS936", PLAYER_SIMPLIFIED_CHINA},
          {"windows-936", PLAYER_SIMPLIFIED_CHINA},
          {"csGBK", PLAYER_SIMPLIFIED_CHINA},
          {"Big5-HKSCS", PLAYER_TRADITIONAL_CHINA},
          {"csBig5HKSCS", PLAYER_TRADITIONAL_CHINA},
      };

  auto it = encoding_map.find(encoding);
  if (it != encoding_map.end())
    return it->second;

  return PLAYER_SUBTITLE_ENCODING;
}

PP_MediaPlayerState ConvertToPPState(player_state_e mm_player_state) {
  switch (mm_player_state) {
    case PLAYER_STATE_NONE:
      return PP_MEDIAPLAYERSTATE_NONE;
    case PLAYER_STATE_IDLE:
      return PP_MEDIAPLAYERSTATE_UNINITIALIZED;
    case PLAYER_STATE_READY:
      return PP_MEDIAPLAYERSTATE_READY;
    case PLAYER_STATE_PLAYING:
      return PP_MEDIAPLAYERSTATE_PLAYING;
    case PLAYER_STATE_PAUSED:
      return PP_MEDIAPLAYERSTATE_PAUSED;
    default:
      NOTREACHED();
      return PP_MEDIAPLAYERSTATE_NONE;
  }
}

inline PP_AudioTrackInfo ConvertToPPAudioTrackInfo(
    const player_audio_track_info& mm_player_info) {
  PP_AudioTrackInfo result;
  result.index = mm_player_info.index;
  size_t size =
      std::min(sizeof(mm_player_info.language), sizeof(result.language));
  memcpy(result.language, mm_player_info.language, size);
  result.language[sizeof(result.language) - 1] = '\0';
  return result;
}

inline PP_TextTrackInfo ConvertToPPTextTrackInfo(
    const player_sub_track_info& mm_player_info) {
  PP_TextTrackInfo result;
  result.index = mm_player_info.track_num;
  size_t size =
      std::min(sizeof(mm_player_info.track_lang), sizeof(result.language));
  memcpy(result.language, mm_player_info.track_lang, size);
  result.language[sizeof(result.language) - 1] = '\0';
  return result;
}

inline PP_VideoTrackInfo ConvertToPPVideoTrackInfo(
    const player_video_track_info& mm_player_info) {
  PP_VideoTrackInfo result;
  result.index = 0;
  result.bitrate = mm_player_info.bitrate;
  result.size = PP_MakeSize(mm_player_info.width, mm_player_info.height);
  return result;
}

PP_MediaPlayerError ConvertToPPMediaPlayerError(int mm_player_error_code) {
  switch (mm_player_error_code) {
    case PLAYER_ERROR_NONE:
      return PP_MEDIAPLAYERERROR_NONE;
    case PLAYER_ERROR_OUT_OF_MEMORY:
      return PP_MEDIAPLAYERERROR_RESOURCE;
    case PLAYER_ERROR_INVALID_PARAMETER:
      return PP_MEDIAPLAYERERROR_BAD_ARGUMENT;
    case PLAYER_ERROR_NO_SUCH_FILE:
      return PP_MEDIAPLAYERERROR_RESOURCE;
    case PLAYER_ERROR_INVALID_OPERATION:
      return PP_MEDIAPLAYERERROR_BAD_ARGUMENT;
    case PLAYER_ERROR_FILE_NO_SPACE_ON_DEVICE:
      return PP_MEDIAPLAYERERROR_RESOURCE;
    case PLAYER_ERROR_SEEK_FAILED:
      return PP_MEDIAPLAYERERROR_UNKNOWN;
    case PLAYER_ERROR_NOT_SUPPORTED_FORMAT:
    case PLAYER_ERROR_NOT_SUPPORTED_FILE:
      return PP_MEDIAPLAYERERROR_UNSUPPORTED_CONTAINER;
    case PLAYER_ERROR_INVALID_URI:
      return PP_MEDIAPLAYERERROR_BAD_ARGUMENT;
    case PLAYER_ERROR_SOUND_POLICY:
      return PP_MEDIAPLAYERERROR_UNKNOWN;
    case PLAYER_ERROR_CONNECTION_FAILED:
      return PP_MEDIAPLAYERERROR_NETWORK;
    case PLAYER_ERROR_VIDEO_CAPTURE_FAILED:
      return PP_MEDIAPLAYERERROR_UNKNOWN;
    case PLAYER_ERROR_DRM_EXPIRED:
    case PLAYER_ERROR_DRM_NO_LICENSE:
    case PLAYER_ERROR_DRM_FUTURE_USE:
    case PLAYER_ERROR_DRM_NOT_PERMITTED:
      return PP_MEDIAPLAYERERROR_DECRYPT;
    case PLAYER_ERROR_RESOURCE_LIMIT:
      return PP_MEDIAPLAYERERROR_RESOURCE;
    case PLAYER_ERROR_STREAMING_PLAYER:
      return PP_MEDIAPLAYERERROR_UNKNOWN;
    case PLAYER_ERROR_AUDIO_CODEC_NOT_SUPPORTED:
    case PLAYER_ERROR_VIDEO_CODEC_NOT_SUPPORTED:
      return PP_MEDIAPLAYERERROR_UNSUPPORTED_CODEC;
    case PLAYER_ERROR_NOT_SUPPORTED_SUBTITLE:
      return PP_MEDIAPLAYERERROR_UNSUPPORTED_SUBTITLE_FORMAT;
    case PLAYER_ERROR_NO_AUTH:
    case PLAYER_ERROR_GENEREIC:
      return PP_MEDIAPLAYERERROR_UNKNOWN;
    case PLAYER_ERROR_DRM_INFO:
      return PP_MEDIAPLAYERERROR_DECRYPT;
    case PLAYER_ERROR_SYNC_PLAY_NETWORK_EXCEPTION:
    case PLAYER_ERROR_SYNC_PLAY_SERVER_DOWN:
      return PP_MEDIAPLAYERERROR_NETWORK;
    case PLAYER_ERROR_SERVICE_DISCONNECTED:
      return PP_MEDIAPLAYERERROR_RESOURCE;
    default:
      return PP_MEDIAPLAYERERROR_UNKNOWN;
  }
}

media_format_mimetype_e ConvertToMediaFormatMimeTypeH264(
    PP_VideoCodec_Profile_Samsung pp_profile) {
  // As of 2017-07-25 MM Player supports only following codecs:
  // MEDIA_FORMAT_H264_SP
  // MEDIA_FORMAT_H264_MP
  // MEDIA_FORMAT_H264_HP
  // See __convert_media_format_video_mime_to_str function
  switch (pp_profile) {
    case PP_VIDEOCODEC_PROFILE_SAMSUNG_H264_BASELINE:
    case PP_VIDEOCODEC_PROFILE_SAMSUNG_H264_SCALABLEBASELINE:
      return MEDIA_FORMAT_H264_SP;
    case PP_VIDEOCODEC_PROFILE_SAMSUNG_H264_MAIN:
      return MEDIA_FORMAT_H264_MP;
    case PP_VIDEOCODEC_PROFILE_SAMSUNG_H264_EXTENDED:
    case PP_VIDEOCODEC_PROFILE_SAMSUNG_H264_HIGH:
    case PP_VIDEOCODEC_PROFILE_SAMSUNG_H264_HIGH10:
    case PP_VIDEOCODEC_PROFILE_SAMSUNG_H264_HIGH422:
    case PP_VIDEOCODEC_PROFILE_SAMSUNG_H264_HIGH444PREDICTIVE:
    case PP_VIDEOCODEC_PROFILE_SAMSUNG_H264_SCALABLEHIGH:
    case PP_VIDEOCODEC_PROFILE_SAMSUNG_H264_STEREOHIGH:
    case PP_VIDEOCODEC_PROFILE_SAMSUNG_H264_MULTIVIEWHIGH:
      return MEDIA_FORMAT_H264_HP;
    default:
      return MEDIA_FORMAT_H264_MP;
  }
}

media_format_mimetype_e ConvertToMediaFormatMimeTypeMPEG2(
    PP_VideoCodec_Profile_Samsung /* pp_profile */) {
  // As of 2017-07-25 MM Player supports only following codecs:
  // MEDIA_FORMAT_MPEG2_SP
  // See __convert_media_format_video_mime_to_str function
  return MEDIA_FORMAT_MPEG2_SP;
}

media_format_mimetype_e ConvertToMediaFormatMimeTypeMPEG4(
    PP_VideoCodec_Profile_Samsung /* pp_profile */) {
  // As of 2017-07-25 MM Player supports only following codecs:
  // MEDIA_FORMAT_MPEG4_SP
  // See __convert_media_format_video_mime_to_str function
  return MEDIA_FORMAT_MPEG4_SP;
}

media_format_mimetype_e ConvertToMediaFormatMimeType(
    PP_VideoCodec_Type_Samsung pp_codec,
    PP_VideoCodec_Profile_Samsung pp_profile) {
  switch (pp_codec) {
    case PP_VIDEOCODEC_TYPE_SAMSUNG_H264:
      return ConvertToMediaFormatMimeTypeH264(pp_profile);
    case PP_VIDEOCODEC_TYPE_SAMSUNG_VC1:
      return MEDIA_FORMAT_VC1;
    case PP_VIDEOCODEC_TYPE_SAMSUNG_MPEG2:
      return ConvertToMediaFormatMimeTypeMPEG2(pp_profile);
    case PP_VIDEOCODEC_TYPE_SAMSUNG_MPEG4:
      return ConvertToMediaFormatMimeTypeMPEG4(pp_profile);
    case PP_VIDEOCODEC_TYPE_SAMSUNG_THEORA:
      LOG(ERROR) << "Theora codec not supported";
      return MEDIA_FORMAT_MAX;
    case PP_VIDEOCODEC_TYPE_SAMSUNG_VP8:
      return MEDIA_FORMAT_VP8;
    case PP_VIDEOCODEC_TYPE_SAMSUNG_VP9:
      return MEDIA_FORMAT_VP9;
    case PP_VIDEOCODEC_TYPE_SAMSUNG_H263:
      return MEDIA_FORMAT_H263;
    case PP_VIDEOCODEC_TYPE_SAMSUNG_WMV1:
      LOG(WARNING) << "WMV1 codec not supported directly, trying VC1";
      return MEDIA_FORMAT_VC1;
    case PP_VIDEOCODEC_TYPE_SAMSUNG_WMV2:
      LOG(WARNING) << "WMV1 codec not supported directly, trying VC1";
      return MEDIA_FORMAT_VC1;
    case PP_VIDEOCODEC_TYPE_SAMSUNG_WMV3:
      return MEDIA_FORMAT_VC1;
    case PP_VIDEOCODEC_TYPE_SAMSUNG_INDEO3:
      LOG(ERROR) << "INDEO3 codec not supported";
      return MEDIA_FORMAT_MAX;
    case PP_VIDEOCODEC_TYPE_SAMSUNG_H265:
      return MEDIA_FORMAT_HEVC;
    default:
      LOG(ERROR) << "Unknown video codec: " << pp_codec;
      return MEDIA_FORMAT_MAX;
  }
}

media_format_mimetype_e ConvertToMediaFormatMimeTypeAAC(
    PP_AudioCodec_Profile_Samsung /* pp_profile */) {
  // As of 2017-07-25 MM Player supports only following codecs:
  // MEDIA_FORMAT_AAC
  // See __convert_media_format_video_mime_to_str function
  return MEDIA_FORMAT_AAC;
}

media_format_mimetype_e ConvertToMediaFormatMimeType(
    PP_AudioCodec_Type_Samsung pp_codec,
    PP_AudioCodec_Profile_Samsung pp_profile) {
  switch (pp_codec) {
    case PP_AUDIOCODEC_TYPE_SAMSUNG_AAC:
      return ConvertToMediaFormatMimeTypeAAC(pp_profile);
    case PP_AUDIOCODEC_TYPE_SAMSUNG_MP3:
      return MEDIA_FORMAT_MP3;
    case PP_AUDIOCODEC_TYPE_SAMSUNG_PCM:
      return MEDIA_FORMAT_PCM;
    case PP_AUDIOCODEC_TYPE_SAMSUNG_VORBIS:
      return MEDIA_FORMAT_VORBIS;
    case PP_AUDIOCODEC_TYPE_SAMSUNG_FLAC:
      return MEDIA_FORMAT_FLAC;
    case PP_AUDIOCODEC_TYPE_SAMSUNG_AMR_NB:
      return MEDIA_FORMAT_AMR_NB;
    case PP_AUDIOCODEC_TYPE_SAMSUNG_AMR_WB:
      return MEDIA_FORMAT_AMR_WB;
    case PP_AUDIOCODEC_TYPE_SAMSUNG_PCM_MULAW:
      return MEDIA_FORMAT_PCMU;
    case PP_AUDIOCODEC_TYPE_SAMSUNG_GSM_MS:
      return MEDIA_FORMAT_G729;
    case PP_AUDIOCODEC_TYPE_SAMSUNG_PCM_S16BE:
      return MEDIA_FORMAT_PCM_S16BE;
    case PP_AUDIOCODEC_TYPE_SAMSUNG_PCM_S24BE:
      return MEDIA_FORMAT_PCM_S24BE;
    case PP_AUDIOCODEC_TYPE_SAMSUNG_OPUS:
      LOG(WARNING) << "OPUS codec not supported";
      return MEDIA_FORMAT_MAX;
    case PP_AUDIOCODEC_TYPE_SAMSUNG_EAC3:
      return MEDIA_FORMAT_EAC3;
    case PP_AUDIOCODEC_TYPE_SAMSUNG_MP2:
      return MEDIA_FORMAT_MP2;
    case PP_AUDIOCODEC_TYPE_SAMSUNG_DTS:
      return MEDIA_FORMAT_DTS;
    case PP_AUDIOCODEC_TYPE_SAMSUNG_AC3:
      return MEDIA_FORMAT_AC3;
    case PP_AUDIOCODEC_TYPE_SAMSUNG_WMAV1:
      return MEDIA_FORMAT_WMAV1;
    case PP_AUDIOCODEC_TYPE_SAMSUNG_WMAV2:
      return MEDIA_FORMAT_WMAV2;
    default:
      LOG(ERROR) << "Unknown audio codec: " << pp_codec;
      return MEDIA_FORMAT_MAX;
  }
}

drm_type_es ConvertToESDRMType(PP_MediaPlayerDRMType drm_type) {
  if (drm_type == PP_MEDIAPLAYERDRMTYPE_UNKNOWN)
    return ES_DRM_TYPE_NONE;

  return ES_DRM_TYPE_EME;
}

player_vr360_mode_e ConvertToPlayerVr360Mode(
    PP_MediaPlayerVr360Mode vr360_mode) {
  switch (vr360_mode) {
    case PP_MEDIAPLAYERVR360MODE_OFF:
      return PLAYER_VR360_MODE_OFF;
    case PP_MEDIAPLAYERVR360MODE_ON:
      return PLAYER_VR360_MODE_ON;
    default:
      LOG(ERROR) << "Unknown Vr360 mode: " << vr360_mode;
      return PLAYER_VR360_MODE_OFF;
  }
}

void PlayerOnDRMErrorPrinter(int error_code,
                             char* error_string,
                             void* user_data) {
  LOG(ERROR) << "Got drm error: " << error_code << " (0x" << std::hex
             << error_code << "), user_data: " << user_data;
}

std::string DescribePlayerError(int error_code) {
  std::ostringstream oss;
  oss << error_code << " (0x" << std::hex << error_code << std::dec << "),"
      << " string: '"
      << media::GetString(static_cast<player_error_e>(error_code));
  return oss.str();
}

void PlayerOnErrorPrinter(int error_code, void* user_data) {
  LOG(ERROR) << "Got player error: " << DescribePlayerError(error_code)
             << "' user_data: " << user_data
             << " as pp_error: " << ConvertToPPError(error_code);
}

inline void ExecuteAndResetCallback(int32_t code,
                                    base::Callback<void(int32_t)>* callback) {
  if (!callback->is_null()) {
    callback->Run(code);
    callback->Reset();
  }
}

}  // namespace

class BufferedPacket {
 public:
  virtual ~BufferedPacket() = default;

  virtual size_t PacketSize() const = 0;

  virtual int ProcessPacket(player_h player) {
    int ret = player_push_media_stream(player, Get());

    if (ret == PLAYER_ERROR_NONE) {
      PacketAppended();
    } else if (ret != PLAYER_ERROR_BUFFER_SPACE) {
      uint64_t pts, dts, duration;
      media_packet_get_pts(Get(), &pts);
      media_packet_get_dts(Get(), &dts);
      media_packet_get_duration(Get(), &duration);
      LOG(WARNING) << "Problem appending packet"
                   << " pts: " << pts << " dts: " << dts
                   << " duration: " << duration
                   << " MMPlayer error: " << DescribePlayerError(ret);
    }

    return ret;
  }

  media_format_h GetFormat() { return format_->GetMediaFormat(); }

 protected:
  explicit BufferedPacket(scoped_refptr<MMPlayerMediaFormat> format)
      : format_(std::move(format)) {}

  virtual void PacketAppended() = 0;
  virtual media_packet_h Get() = 0;

 private:
  scoped_refptr<MMPlayerMediaFormat> format_;
  DISALLOW_COPY_AND_ASSIGN(BufferedPacket);
};

class MMPacketBuffer {
 public:
  MMPacketBuffer() : size_(0) {}

  bool PushBack(std::unique_ptr<BufferedPacket> pkt) {
    auto packet_size = pkt->PacketSize();
    if (size_ + packet_size > kInternalBufferSize)
      return false;
    size_ += packet_size;
    packets_.emplace(std::move(pkt));
    return true;
  }

  bool IsEmpty() const { return packets_.empty(); }

  BufferedPacket* PeekFront() {
    if (packets_.empty())
      return nullptr;
    return packets_.front().get();
  }

  void PopFront() {
    if (!packets_.empty()) {
      size_ -= packets_.front()->PacketSize();
      packets_.pop();
    }
  }

  void Flush() {
    std::queue<std::unique_ptr<BufferedPacket>> empty_queue;
    packets_.swap(empty_queue);
    size_ = 0;
  }

 private:
  size_t size_;
  std::queue<std::unique_ptr<BufferedPacket>> packets_;
};

namespace {

class BufferedConfiguration : public BufferedPacket {
 public:
  static std::unique_ptr<BufferedConfiguration> Create(
      player_stream_type_e stream_type,
      scoped_refptr<MMPlayerMediaFormat> format) {
    return std::make_unique<BufferedConfiguration>(stream_type,
                                                   std::move(format));
  }

  virtual ~BufferedConfiguration() = default;

  size_t PacketSize() const override { return 0; }

  int ProcessPacket(player_h player) override {
    int ret = player_set_media_stream_info(player, stream_type_, GetFormat());
    LOG_IF(WARNING, ret != PLAYER_ERROR_NONE)
        << "Problem updating config:"
        << " type: " << stream_type_
        << " config: " << reinterpret_cast<void*>(GetFormat())
        << " MMPlayer error: " << DescribePlayerError(ret);
    return ret;
  }

 private:
  friend std::unique_ptr<BufferedConfiguration> std::make_unique<
      BufferedConfiguration>(player_stream_type_e&,
                             scoped_refptr<content::MMPlayerMediaFormat>&&);

  BufferedConfiguration(player_stream_type_e stream_type,
                        scoped_refptr<MMPlayerMediaFormat> format)
      : BufferedPacket(std::move(format)), stream_type_(stream_type) {}

  void PacketAppended() override {}
  media_packet_h Get() override { return nullptr; }

  player_stream_type_e stream_type_;
};

class EOSPacket : public BufferedPacket {
 public:
  static std::unique_ptr<EOSPacket> Create(
      scoped_refptr<MMPlayerMediaFormat> format) {
    media_packet_h pkt_handle;
    int error = media_packet_create_alloc(format->GetMediaFormat(), nullptr,
                                          nullptr, &pkt_handle);
    if (error != MEDIA_PACKET_ERROR_NONE) {
      LOG(ERROR) << "Failed to create packet: " << error;
      return {};
    }
    media::ScopedMediaPacket pkt{pkt_handle};

    error = media_packet_set_flags(pkt.get(), MEDIA_PACKET_END_OF_STREAM);
    if (error != MEDIA_PACKET_ERROR_NONE) {
      LOG(ERROR) << "Failed to set EOS flag: " << error;
      return {};
    }

    return std::make_unique<EOSPacket>(std::move(format), std::move(pkt));
  }

  ~EOSPacket() override = default;

  void PacketAppended() override {
    // NOP
  }

  media_packet_h Get() override { return pkt_.get(); }

  size_t PacketSize() const override { return 0; }

 private:
  friend std::unique_ptr<EOSPacket> std::make_unique<EOSPacket>(
      scoped_refptr<content::MMPlayerMediaFormat>&&,
      media::ScopedMediaPacket&&);

  explicit EOSPacket(scoped_refptr<MMPlayerMediaFormat> format,
                     media::ScopedMediaPacket pkt)
      : BufferedPacket(std::move(format)), pkt_(std::move(pkt)) {}

  media::ScopedMediaPacket pkt_;
};

class ClearPacket : public BufferedPacket {
 public:
  static std::unique_ptr<ClearPacket> Create(
      scoped_refptr<MMPlayerMediaFormat> format,
      const ppapi::ESPacket* packet) {
    // Need to copy data as media_packet_create_from_external_memory only
    // wraps non-owning pointer
    std::vector<uint8_t> data(
        static_cast<const uint8_t*>(packet->Data()),
        static_cast<const uint8_t*>(packet->Data()) + packet->DataSize());

    media_packet_h pkt_handle;
    int error = media_packet_create_from_external_memory(
        format->GetMediaFormat(), data.data(), data.size(), nullptr, nullptr,
        &pkt_handle);
    if (error != MEDIA_PACKET_ERROR_NONE) {
      LOG(ERROR) << "Failed to create packet: " << error;
      return {};
    }

    media::ScopedMediaPacket pkt{pkt_handle};
    media_packet_set_pts(pkt.get(), ConvertToNanoseconds(packet->Pts()));
    media_packet_set_dts(pkt.get(), ConvertToNanoseconds(packet->Dts()));
    media_packet_set_duration(pkt.get(),
                              ConvertToNanoseconds(packet->Duration()));

    return std::make_unique<ClearPacket>(std::move(format), std::move(pkt),
                                         std::move(data));
  }

  ~ClearPacket() override = default;

  void PacketAppended() override {
    // NOP
  }

  media_packet_h Get() override { return pkt_.get(); }

  size_t PacketSize() const override { return data_.size(); }

 private:
  friend std::unique_ptr<ClearPacket> std::make_unique<ClearPacket>(
      scoped_refptr<content::MMPlayerMediaFormat>&&,
      media::ScopedMediaPacket&&,
      std::vector<uint8_t>&&);

  ClearPacket(scoped_refptr<MMPlayerMediaFormat> format,
              media::ScopedMediaPacket pkt,
              std::vector<uint8_t>&& data)
      : BufferedPacket(std::move(format)),
        data_(std::move(data)),
        pkt_(std::move(pkt)) {
    void* pkt_buf = nullptr;
    media_packet_get_buffer_data_ptr(pkt_.get(), &pkt_buf);
    DCHECK_EQ(pkt_buf, data_.data());
  }

  std::vector<uint8_t> data_;
  media::ScopedMediaPacket pkt_;
};

class EncryptedPacket : public BufferedPacket {
 public:
  static std::unique_ptr<EncryptedPacket> Create(
      scoped_refptr<MMPlayerMediaFormat> format,
      const ppapi::ESPacket* packet,
      ScopedHandleAndSize handle) {
    media_packet_h pkt_handle;
    int error = media_packet_create(format->GetMediaFormat(), nullptr, nullptr,
                                    &pkt_handle);
    if (error != MEDIA_PACKET_ERROR_NONE) {
      LOG(ERROR) << "Failed to create packet: " << error;
      return {};
    }

    media::ScopedMediaPacket pkt{pkt_handle};
    media_packet_set_pts(pkt.get(), ConvertToNanoseconds(packet->Pts()));
    media_packet_set_dts(pkt.get(), ConvertToNanoseconds(packet->Dts()));
    media_packet_set_duration(pkt.get(),
                              ConvertToNanoseconds(packet->Duration()));
    media_packet_set_buffer_size(pkt.get(), handle->size);

    // see media_source_player_capi.cc
    auto drm_info = std::make_unique<es_player_drm_info>();
    memset(drm_info.get(), 0, sizeof(es_player_drm_info));
    drm_info->drm_type = ES_DRM_TYPE_EME;
    drm_info->algorithm = DRMB_AES128_CTR;
    drm_info->format = DRMB_FORMAT_FMP4;
    drm_info->phase = DRMB_PHASE_NONE;
    drm_info->pKID = nullptr;
    drm_info->uKIDLen = 0u;
    drm_info->pIV = nullptr;
    drm_info->uIVLen = 0u;
    drm_info->pKey = nullptr;
    drm_info->uKeyLen = 0u;
    drm_info->pSubData = nullptr;
    drm_info->drm_handle = 0u;
    drm_info->tz_handle = handle->handle;
    media_packet_set_extra(pkt.get(), drm_info.get());

    return std::make_unique<EncryptedPacket>(std::move(format), std::move(pkt),
                                             std::move(handle),
                                             std::move(drm_info));
  }

  ~EncryptedPacket() override = default;

  void PacketAppended() override { ClearHandle(handle_.get()); }

  media_packet_h Get() override { return pkt_.get(); }

  size_t PacketSize() const override { return handle_->size; }

 private:
  friend std::unique_ptr<EncryptedPacket> std::make_unique<EncryptedPacket>(
      scoped_refptr<content::MMPlayerMediaFormat>&&,
      media::ScopedMediaPacket&&,
      content::ScopedHandleAndSize&&,
      std::unique_ptr<es_player_drm_info>&&);

  EncryptedPacket(scoped_refptr<MMPlayerMediaFormat> format,
                  media::ScopedMediaPacket pkt,
                  ScopedHandleAndSize handle,
                  std::unique_ptr<es_player_drm_info> drm_info)
      : BufferedPacket(std::move(format)),
        handle_(std::move(handle)),
        drm_info_(std::move(drm_info)),
        pkt_(std::move(pkt)) {}

  ScopedHandleAndSize handle_;
  std::unique_ptr<es_player_drm_info> drm_info_;
  media::ScopedMediaPacket pkt_;
};

}  // namespace

// static
scoped_refptr<MMPlayerAudioFormat> MMPlayerAudioFormat::Create(
    const PepperAudioStreamInfo& stream_info) {
  media_format_h fmt_handle;
  int error = media_format_create(&fmt_handle);
  if (error != MEDIA_FORMAT_ERROR_NONE) {
    LOG(ERROR) << "Error creating format error: " << error;
    return {};
  }

  scoped_refptr<MMPlayerAudioFormat> format{
      new MMPlayerAudioFormat(fmt_handle, stream_info)};

  error = media_format_set_audio_mime(
      format->GetMediaFormat(),
      ConvertToMediaFormatMimeType(stream_info.config.codec,
                                   stream_info.config.profile));
  if (error != MEDIA_FORMAT_ERROR_NONE) {
    LOG(ERROR) << "Error setting mime type: " << error;
    return {};
  }

  error = media_format_set_audio_channel(
      format->GetMediaFormat(),
      GetChannelsCount(stream_info.config.channel_layout));
  if (error != MEDIA_FORMAT_ERROR_NONE) {
    LOG(ERROR) << "Error setting channel layout: " << error;
    return {};
  }

  error = media_format_set_audio_samplerate(
      format->GetMediaFormat(), stream_info.config.samples_per_second);
  if (error != MEDIA_FORMAT_ERROR_NONE) {
    LOG(ERROR) << "Error setting sample rate: " << error;
    return {};
  }

  error =
      media_format_set_extra(format->GetMediaFormat(), &format->extra_info_);
  if (error != MEDIA_FORMAT_ERROR_NONE) {
    LOG(ERROR) << "Error setting extra data: " << error;
    return {};
  }

  return format;
}

media_format_h MMPlayerAudioFormat::GetMediaFormat() {
  return format_.get();
}

MMPlayerAudioFormat::MMPlayerAudioFormat(
    media_format_h handle,
    const PepperAudioStreamInfo& stream_info)
    : codec_extradata_{stream_info.config.extra_data}, format_{handle} {
  memset(&extra_info_, 0, sizeof(player_media_stream_audio_extra_info_s));
  extra_info_.codec_extradata = codec_extradata_.data();
  extra_info_.extradata_size = codec_extradata_.size();
  extra_info_.drm_type = ConvertToESDRMType(stream_info.drm_type);
}

MMPlayerAudioFormat::~MMPlayerAudioFormat() = default;

// static
scoped_refptr<MMPlayerVideoFormat> MMPlayerVideoFormat::Create(
    const PepperVideoStreamInfo& stream_info) {
  media_format_h fmt_handle;
  int error = media_format_create(&fmt_handle);
  if (error != MEDIA_FORMAT_ERROR_NONE) {
    LOG(ERROR) << "Error creating format error: " << error;
    return {};
  }

  scoped_refptr<MMPlayerVideoFormat> format{
      new MMPlayerVideoFormat(fmt_handle, stream_info)};

  error = media_format_set_video_mime(
      format->GetMediaFormat(),
      ConvertToMediaFormatMimeType(stream_info.config.codec,
                                   stream_info.config.profile));
  if (error != MEDIA_FORMAT_ERROR_NONE) {
    LOG(ERROR) << "Error setting mime type: " << error;
    return {};
  }

  error = media_format_set_video_width(format->GetMediaFormat(),
                                       stream_info.config.frame_size.width);
  if (error != MEDIA_FORMAT_ERROR_NONE) {
    LOG(ERROR) << "Error setting width: " << error;
    return {};
  }

  error = media_format_set_video_height(format->GetMediaFormat(),
                                        stream_info.config.frame_size.height);
  if (error != MEDIA_FORMAT_ERROR_NONE) {
    LOG(ERROR) << "Error setting height: " << error;
    return {};
  }

  error =
      media_format_set_extra(format->GetMediaFormat(), &format->extra_info_);
  if (error != MEDIA_FORMAT_ERROR_NONE) {
    LOG(ERROR) << "Error setting extra data: " << error;
    return {};
  }

  format->media_resolution_.width =
      static_cast<float>(stream_info.config.frame_size.width);
  format->media_resolution_.height =
      static_cast<float>(stream_info.config.frame_size.height);
  return format;
}

float MMPlayerVideoFormat::GetAspectRatio() const {
  return media_resolution_.width / media_resolution_.height;
}

media_format_h MMPlayerVideoFormat::GetMediaFormat() {
  return format_.get();
}

MMPlayerVideoFormat::MMPlayerVideoFormat(
    media_format_h handle,
    const PepperVideoStreamInfo& stream_info)
    : codec_extradata_{stream_info.config.extra_data}, format_{handle} {
  memset(&extra_info_, 0, sizeof(player_media_stream_video_extra_info_s));
  extra_info_.codec_extradata = codec_extradata_.data();
  extra_info_.extradata_size = codec_extradata_.size();
  extra_info_.framerate_num = stream_info.config.frame_rate.first;
  extra_info_.framerate_den = stream_info.config.frame_rate.second;
  extra_info_.max_width = stream_info.config.frame_size.width;
  extra_info_.max_height = stream_info.config.frame_size.height;
  extra_info_.hdr_mode = MEDIA_STREAM_HDR_TYPE_MAX;
  extra_info_.hdr_info = nullptr;
  extra_info_.is_framerate_changed = 0;
  extra_info_.drm_type = ConvertToESDRMType(stream_info.drm_type);
}

MMPlayerVideoFormat::~MMPlayerVideoFormat() = default;

PepperMMPlayerAdapter::PepperMMPlayerAdapter()
    : player_(nullptr),
      audio_format_(nullptr),
      video_format_(nullptr),
      audio_packets_(new MMPacketBuffer()),
      video_packets_(new MMPacketBuffer()),
      player_preparing_(PlayerPreparingState::kNone),
      buffering_started_(false),
      audio_media_callback_set_(false),
      video_media_callback_set_(false),
      packets_flush_requested_(false),
      target_state_after_seek_(PLAYER_STATE_NONE),
      es_mode_(false),
      display_mode_(PP_MEDIAPLAYERDISPLAYMODE_STRETCH) {}

PepperMMPlayerAdapter::~PepperMMPlayerAdapter() {
  Reset();
}

int32_t PepperMMPlayerAdapter::Initialize() {
  assert(player_ == nullptr);

  player_h player;

  if (player_create(&player) == PLAYER_ERROR_NONE) {
    player_ = player;
    player_set_error_cb(player_, PlayerOnErrorPrinter, 0);
    player_set_drm_error_cb(player_, PlayerOnDRMErrorPrinter, 0);
    player_set_buffering_cb(player_, OnBufferingEvent, this);
    player_set_media_stream_buffer_max_size(player_, PLAYER_STREAM_TYPE_VIDEO,
                                            kBufferMaxSize);
    player_set_media_stream_buffer_min_threshold(
        player_, PLAYER_STREAM_TYPE_VIDEO, kBufferMinThreshold);
    return PP_OK;
  }

  return PP_ERROR_FAILED;
}

int32_t PepperMMPlayerAdapter::Reset() {
  PepperPlayerAdapterBase::Reset();
  if (!player_)
    return PP_OK;

  player_unset_buffering_cb(player_);
  if (player_preparing_ == PlayerPreparingState::kPreparing) {
    OnPlayerPrepared(PP_ERROR_ABORTED);
  } else {
    StopPlayer();
    UnpreparePlayer();
  }

  ClearMediaCallbacks();
  player_unset_buffering_cb(player_);
  player_unset_drm_error_cb(player_);
  player_unset_error_cb(player_);
  player_destroy(player_);
  player_ = nullptr;
  buffering_started_ = false;
  display_mode_ = PP_MEDIAPLAYERDISPLAYMODE_STRETCH;
  return PP_OK;
}

int32_t PepperMMPlayerAdapter::PrepareES() {
  es_mode_ = true;
  target_state_after_seek_ = PLAYER_STATE_NONE;
  OnPlayerPreparing();

  if (auto buffering_listener = GetBufferingListener()) {
    // OnBufferingComplete will be called by OnInitializeDone()
    buffering_listener->OnBufferingStart();
  }

  // Simulate buffering 0% event, as ES data source does not produce such
  // events.
  OnBufferingProgress(0);

  // In 'pull mode', player_prepare_async needs to buffer some data to complete.
  // It may lead to deadlock, because player waits for data
  // and application waits for 'attach' completion.
  //
  // To overcome this problem, we finish this call immediately and notify
  // application about buffering status via buffering_listener.
  int32_t pp_error =
      ConvertToPPError(player_prepare_async(player_, OnInitializeDone, this));

  return pp_error;
}

int32_t PepperMMPlayerAdapter::PrepareURL(const std::string& url) {
  es_mode_ = false;
  target_state_after_seek_ = PLAYER_STATE_NONE;
  int error_code = player_set_uri(player_, url.c_str());

  if (error_code == PLAYER_ERROR_NONE) {
    OnPlayerPreparing();
    buffering_started_ = false;
    error_code = player_prepare(player_);
  }

  // Player doesn't emit buffering events for file:// protocol
  static const char kFileProtocol[] = "file://";
  bool is_file_protocol =
      !url.compare(0, sizeof(kFileProtocol) - 1, kFileProtocol);
  if (error_code == PP_OK && is_file_protocol) {
    OnBufferingProgress(0);
    OnBufferingProgress(100);
  }

  return ConvertToPPError(error_code);
}

int32_t PepperMMPlayerAdapter::Unprepare() {
  es_mode_ = false;
  target_state_after_seek_ = PLAYER_STATE_NONE;

  // Workaround of MM Player behavior see player_preparering_ in header.
  if (player_preparing_ == PlayerPreparingState::kPreparing) {
    Reset();
    return Initialize();
  }

  // According to MM Player API usage guidelines we need to unsed media stream
  // callbacks (media_stream_buffer_status_cb and media_stream_seek_cb)
  // before unpreparing player.
  ClearMediaCallbacks();
  return UnpreparePlayer();
}

int32_t PepperMMPlayerAdapter::Play() {
  return ConvertToPPError(player_start(player_));
}

int32_t PepperMMPlayerAdapter::Pause() {
  return ConvertToPPError(player_pause(player_));
}

void PepperMMPlayerAdapter::SeekCompleted() {
  if (es_mode_ && target_state_after_seek_ == PLAYER_STATE_PLAYING)
    player_start(player_);

  RunAndClearCallback<PepperPlayerCallbackType::SeekCompletedCallback>();
}

void PepperMMPlayerAdapter::FlushPacketsOnAppendThread() {
  audio_packets_->Flush();
  video_packets_->Flush();
  packets_flush_requested_ = false;
}

void PepperMMPlayerAdapter::FlushPacketsAndUpdateConfigs() {
  if (append_packet_runner_) {
    packets_flush_requested_ = true;
    append_packet_runner_->PostTask(
        FROM_HERE,
        base::Bind(&PepperMMPlayerAdapter::FlushPacketsOnAppendThread,
                   base::Unretained(this)));
  }

  if (audio_format_ && audio_format_->GetMediaFormat()) {
    player_set_media_stream_info(player_, PLAYER_STREAM_TYPE_AUDIO,
                                 audio_format_->GetMediaFormat());
  }

  if (video_format_ && video_format_->GetMediaFormat()) {
    player_set_media_stream_info(player_, PLAYER_STREAM_TYPE_VIDEO,
                                 video_format_->GetMediaFormat());
  }
}

// static
void PepperMMPlayerAdapter::OnSeekCompleted(void* user_data) {
  if (!user_data)
    return;

  static_cast<PepperMMPlayerAdapter*>(user_data)->SeekCompleted();
}

int32_t PepperMMPlayerAdapter::Seek(PP_TimeTicks time,
                                    const base::Closure& callback) {
  SetCallback<PepperPlayerCallbackType::SeekCompletedCallback>(callback);

  int mm_player_time = ConvertToMMPlayerTimeTicks(time);

  player_state_e mm_player_state = PLAYER_STATE_NONE;
  int mm_error_code = player_get_state(player_, &mm_player_state);
  if (mm_error_code == PLAYER_ERROR_NONE &&
      mm_player_state == PLAYER_STATE_READY) {
    mm_error_code = PLAYER_ERROR_INVALID_OPERATION;
  } else {
    if (es_mode_) {
      target_state_after_seek_ = mm_player_state;
      if (mm_player_state == PLAYER_STATE_PLAYING)
        player_pause(player_);
    }

    mm_error_code = player_set_play_position(
        player_, mm_player_time, kAccurateSeek, &OnSeekCompleted, this);
  }

  return ConvertToPPError(mm_error_code);
}

void PepperMMPlayerAdapter::SeekOnAppendThread(PP_TimeTicks) {
  if (es_mode_)
    FlushPacketsAndUpdateConfigs();
}

int32_t PepperMMPlayerAdapter::SelectTrack(
    PP_ElementaryStream_Type_Samsung stream_type,
    uint32_t track_index) {
  // SelectTrack for video streams is not supported by player
  if (stream_type == PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_VIDEO)
    return PP_ERROR_NOTSUPPORTED;

  player_stream_type_e mm_stream_type = ConvertToMMPlayerTrackType(stream_type);
  int mm_track_index = static_cast<int>(track_index);

  return ConvertToPPError(
      player_select_track(player_, mm_stream_type, mm_track_index));
}

int32_t PepperMMPlayerAdapter::SetPlaybackRate(float rate) {
  return ConvertToPPError(player_set_playback_rate(player_, rate));
}

int32_t PepperMMPlayerAdapter::SubmitEOSPacket(
    PP_ElementaryStream_Type_Samsung track_type) {
  auto format = GetMediaFormat(track_type);
  if (!format)
    return PP_ERROR_FAILED;

  return PushMediaStream(track_type, EOSPacket::Create(format));
}

int32_t PepperMMPlayerAdapter::SubmitPacket(
    const ppapi::ESPacket* packet,
    PP_ElementaryStream_Type_Samsung track_type) {
  auto format = GetMediaFormat(track_type);
  if (!format)
    return PP_ERROR_FAILED;

  return PushMediaStream(track_type, ClearPacket::Create(format, packet));
}

int32_t PepperMMPlayerAdapter::SubmitEncryptedPacket(
    const ppapi::ESPacket* packet,
    content::ScopedHandleAndSize handle,
    PP_ElementaryStream_Type_Samsung track_type) {
  auto format = GetMediaFormat(track_type);
  if (!format)
    return PP_ERROR_FAILED;

  return PushMediaStream(
      track_type, EncryptedPacket::Create(format, packet, std::move(handle)));
}

void PepperMMPlayerAdapter::Stop(
    const base::Callback<void(int32_t)>& callback) {
  if (!stop_callback_.is_null()) {
    // Private layer shouldn't allow calling Stop when Stop is already in
    // progress.
    NOTREACHED() << "Stop already in progress.";
    ExecuteAndResetCallback(PP_OK, &stop_callback_);
  }

  auto result = ConvertToPPError(player_stop(player_));
  if (result != PP_OK) {
    callback.Run(result);
    return;
  }
  // We discovered that sometimes stop is not waiting for buffering
  // completion. It causes strange, random errors in further calls. Execute
  // stop callback only when buffering finished (later, when player_preparing_
  // is set to false or immediately if player is not currently preparing).
  if (player_preparing_ == PlayerPreparingState::kPreparing)
    stop_callback_ = callback;
  else
    callback.Run(PP_OK);
}

// Player setters
int32_t PepperMMPlayerAdapter::SetApplicationID(
    const std::string& application_id) {
  return ConvertToPPError(player_set_app_id(player_, application_id.c_str()));
}

int32_t PepperMMPlayerAdapter::SetAudioStreamInfo(
    const PepperAudioStreamInfo& stream_info) {
  if (stream_info.initialization_mode != PP_STREAMINITIALIZATIONMODE_FULL)
    return PP_ERROR_NOTSUPPORTED;

  auto format = MMPlayerAudioFormat::Create(stream_info);
  if (!format)
    return PP_ERROR_FAILED;

  audio_format_ = std::move(format);
  if (!audio_packets_->IsEmpty()) {
    // Pushing configuration packet never fails, even if buffer is full.
    audio_packets_->PushBack(
        BufferedConfiguration::Create(PLAYER_STREAM_TYPE_AUDIO, audio_format_));
    return PP_OK;
  }

  return ConvertToPPError(player_set_media_stream_info(
      player_, PLAYER_STREAM_TYPE_AUDIO, audio_format_->GetMediaFormat()));
}

int32_t PepperMMPlayerAdapter::SetDisplay(void* display, bool is_windowless) {
  int mm_error_code;

  if (is_windowless) {
    mm_error_code =
        player_set_display_mode(player_, PLAYER_DISPLAY_MODE_DST_ROI);
    if (mm_error_code != PLAYER_ERROR_NONE) {
      LOG(ERROR) << "Cannot set mmplayer display mode to ROI.";
      return ConvertToPPError(mm_error_code);
    }

    mm_error_code = player_set_display_roi_area(player_, 0, 0, 1, 1);
    if (mm_error_code != PLAYER_ERROR_NONE) {
      LOG(ERROR) << "Cannot set mmplayer ROI.";
      return ConvertToPPError(mm_error_code);
    }
  }

  ecore_thread_main_loop_begin();
  mm_error_code =
      player_set_display(player_, PLAYER_DISPLAY_TYPE_OVERLAY, display);
  ecore_thread_main_loop_end();
  if (mm_error_code != PLAYER_ERROR_NONE) {
    LOG(ERROR) << "Cannot set mmplayer display type overlay.";
  }

  return ConvertToPPError(mm_error_code);
}

int32_t PepperMMPlayerAdapter::SetDisplayRect(
    const PP_Rect& viewport,
    const PP_FloatRect& crop_ratio_rect) {
  PP_Rect display_rect;
  if (video_format_ && display_mode_ == PP_MEDIAPLAYERDISPLAYMODE_LETTERBOX) {
    auto viewport_aspect_ratio = static_cast<double>(viewport.size.width) /
                                 static_cast<double>(viewport.size.height);
    auto media_resolution = video_format_->media_resolution();
    if (viewport_aspect_ratio > video_format_->GetAspectRatio()) {
      display_rect.size.width =
          GetClipForLetterboxSize(media_resolution.width, viewport.size.height,
                                  media_resolution.height);
      display_rect.size.height = viewport.size.height;
      display_rect.point.x = GetClipForLetterboxPosition(
          viewport.point.x, viewport.size.width, display_rect.size.width);
      display_rect.point.y = viewport.point.y;
    } else {
      display_rect.size.width = viewport.size.width;
      display_rect.size.height = GetClipForLetterboxSize(
          media_resolution.height, viewport.size.width, media_resolution.width);
      display_rect.point.x = viewport.point.x;
      display_rect.point.y = GetClipForLetterboxPosition(
          viewport.point.y, viewport.size.height, display_rect.size.height);
    }
  } else if (!video_format_ &&
             display_mode_ == PP_MEDIAPLAYERDISPLAYMODE_LETTERBOX) {
    LOG(WARNING) << "video_format_ not set! Can't set properly letterboxing.";
    display_rect = viewport;
  } else {
    // PP_MEDIAPLAYERDISPLAYMODE_STRETCH: make content take entire viewport.
    display_rect = viewport;
  }

  int mm_error_code = player_set_display_roi_area(
      player_, display_rect.point.x, display_rect.point.y,
      display_rect.size.width, display_rect.size.height);

  if (mm_error_code == PLAYER_ERROR_NONE) {
    mm_error_code = player_set_crop_ratio(
        player_, crop_ratio_rect.point.x, crop_ratio_rect.point.y,
        crop_ratio_rect.size.width, crop_ratio_rect.size.height);
  } else {
    LOG(WARNING) << "Failed to set display dst ROI error: " << mm_error_code;
  }

  return ConvertToPPError(mm_error_code);
}

int32_t PepperMMPlayerAdapter::SetDisplayMode(
    PP_MediaPlayerDisplayMode display_mode) {
  if (display_mode_ == display_mode)
    return PP_OK;
  display_mode_ = display_mode;
  // If display mode was changed, then it is likely that video geometry must be
  // recalculated.
  RunCallback<PepperPlayerCallbackType::DisplayRectUpdateCallback>();
  return PP_OK;
}

int32_t PepperMMPlayerAdapter::SetVr360Mode(
    PP_MediaPlayerVr360Mode vr360_mode) {
  auto mm_error_code =
      player_vr360_set_mode(player_, ConvertToPlayerVr360Mode(vr360_mode));
  if (mm_error_code != PLAYER_ERROR_NONE) {
    LOG(ERROR) << "Cannot set mmplayer Vr360 mode to " << vr360_mode
               << ", error = " << mm_error_code;
  }
  return ConvertToPPError(mm_error_code);
}

int32_t PepperMMPlayerAdapter::SetVr360Rotation(float horizontal_angle,
                                                float vertical_angle) {
  auto mm_error_code =
      player_vr360_set_rotation(player_, horizontal_angle, vertical_angle);
  if (mm_error_code != PLAYER_ERROR_NONE) {
    LOG(ERROR) << "Cannot set mmplayer Vr360 rotation to " << horizontal_angle
               << ", " << vertical_angle << ", error = " << mm_error_code;
  }
  return ConvertToPPError(mm_error_code);
}

int32_t PepperMMPlayerAdapter::SetVr360ZoomLevel(uint32_t zoom_level) {
  auto mm_error_code = player_vr360_set_zoom(player_, zoom_level);
  if (mm_error_code != PLAYER_ERROR_NONE) {
    LOG(ERROR) << "Cannot set mmplayer Vr360 zoom level to " << zoom_level
               << ", error = " << mm_error_code;
  }
  return ConvertToPPError(mm_error_code);
}

bool PepperMMPlayerAdapter::IsDisplayModeSupported(
    PP_MediaPlayerDisplayMode display_mode) {
  // We support all currently defined modes.
  return static_cast<uint32_t>(display_mode) <= PP_MEDIAPLAYERDISPLAYMODE_LAST;
}

int32_t PepperMMPlayerAdapter::SetExternalSubtitlesPath(
    const std::string& file_path,
    const std::string& encoding) {
  // Player lets only to add external subtitle path before attaching data
  // source.
  player_state_e mm_player_state = PLAYER_STATE_NONE;
  int mm_error_code = player_get_state(player_, &mm_player_state);
  if (mm_error_code != PLAYER_ERROR_NONE) {
    LOG(ERROR) << "Failed to get player state, got mm error: " << mm_error_code;
    return ConvertToPPError(mm_error_code);
  }
  if (mm_player_state != PLAYER_STATE_IDLE)
    return PP_ERROR_NOTSUPPORTED;

  int32_t error_code = ValidateSubtitleEncoding(encoding);
  // PP_ERROR_ABORTED means we should abort seting subtitle encoding
  // but still set path. For more info see ValidateSubtitleEncoding definition.
  if (error_code != PP_OK) {
    LOG(ERROR) << "Not valid subtitle encoding: " << error_code;
    return error_code;
  }

  error_code =
      ConvertToPPError(player_set_subtitle_path(player_, file_path.c_str()));
  if (error_code == PP_ERROR_FAILED) {
    LOG(ERROR) << "Subtitles cannot be set in a current Player state";
    return PP_ERROR_BADARGUMENT;
  }

  if (error_code != PP_OK)
    return error_code;

  return SetSubtitlesEncoding(encoding);
}

int32_t PepperMMPlayerAdapter::SetStreamingProperty(
    PP_StreamingProperty property,
    const std::string& data) {
  int error_code = PLAYER_ERROR_GENEREIC;

  switch (property) {
    case PP_STREAMINGPROPERTY_COOKIE:
      error_code =
          player_set_streaming_cookie(player_, data.c_str(), data.length());
      break;
    case PP_STREAMINGPROPERTY_USER_AGENT:
      error_code =
          player_set_streaming_user_agent(player_, data.c_str(), data.length());
      break;
    case PP_STREAMINGPROPERTY_ADAPTIVE_INFO:
      error_code = player_set_adaptive_streaming_info(
          player_, const_cast<char*>(data.c_str()),
          PLAYER_ADAPTIVE_INFO_URL_CUSTOM);
      break;
    case PP_STREAMINGPROPERTY_TYPE:
      error_code =
          player_set_streaming_type(player_, const_cast<char*>(data.c_str()));
      break;
    default:
      break;
  }

  return ConvertToPPError(error_code);
}

int32_t PepperMMPlayerAdapter::ValidateSubtitleEncoding(
    const std::string& encoding) {
  // Empty means UTF-8 and player does not support UTF-8 in
  // player_set_subtitle_encoding, which is default encoding in
  // the mmplayer if player_set_subtitle_encoding is not called (yes, it is
  // funny peculiar). So we should abort validating and setting encoding but
  // still set path.
  if (encoding.empty() || IsUTF8(encoding))
    return PP_OK;

  player_subtitle_encoding_e mm_encoding =
      ConvertToMMPlayerSubtitleEncoding(encoding);

  if (mm_encoding < PLAYER_SUBTITLE_ENCODING ||
      mm_encoding >= PLAYER_SUBTITLE_ENCODING_MAX) {
    return PP_ERROR_NOTSUPPORTED;
  }

  if (mm_encoding == PLAYER_SUBTITLE_ENCODING) {
    LOG(ERROR) << "Unrecognized subtitle encoding";
    return PP_ERROR_BADARGUMENT;
  }
  return PP_OK;
}

int32_t PepperMMPlayerAdapter::SetSubtitlesEncoding(
    const std::string& encoding) {
  if (encoding.empty() || IsUTF8(encoding))
    return PP_OK;

  int32_t error_code = ValidateSubtitleEncoding(encoding);

  if (error_code != PP_OK)
    return error_code;

  player_subtitle_encoding_e mm_encoding =
      ConvertToMMPlayerSubtitleEncoding(encoding);

  return ConvertToPPError(player_set_subtitle_encoding(player_, mm_encoding));
}

int32_t PepperMMPlayerAdapter::SetSubtitlesDelay(PP_TimeDelta delay) {
  int mm_delay = ConvertToMMPlayerTimeDelta(delay);

  return ConvertToPPError(
      player_set_subtitle_position_offset(player_, mm_delay));
}

int32_t PepperMMPlayerAdapter::SetVideoStreamInfo(
    const PepperVideoStreamInfo& stream_info) {
  if (stream_info.initialization_mode != PP_STREAMINITIALIZATIONMODE_FULL)
    return PP_ERROR_NOTSUPPORTED;

  bool uhd_panel_supported;
  int ret = system_info_get_value_bool(SYSTEM_INFO_KEY_UD_PANEL_SUPPORTED,
                                       &uhd_panel_supported);

  bool can_use_uhd_decoder = uhd_panel_supported;

  const auto content_width = stream_info.config.frame_size.width;
  const auto content_height = stream_info.config.frame_size.height;
  const bool resolution_over_fhd =
      (content_width > kFHDWidth) || (content_height > kFHDHeight);

  if (!uhd_panel_supported && resolution_over_fhd) {
    LOG(ERROR) << "Display doesn't support UHD resolution. "
               << "Content cannot be played.";
    return PP_ERROR_NOTSUPPORTED;
  }

#if defined(TIZEN_PEPPER_PLAYER_H264_UHD_ONLY_30FPS)
  if (stream_info.config.codec == PP_VIDEOCODEC_TYPE_SAMSUNG_H264) {
    // H.264 UHD 60 fps is not supported on specified platform.
    // For H.264 codec, if resolution is not greater than FHD
    // and framerate is over 30 fps, we should use FHD decoder.
    const auto& framerate_pair = stream_info.config.frame_rate;
    const float framerate =
        static_cast<float>(framerate_pair.first) / framerate_pair.second;
    const bool framerate_over_30fps =
        framerate > (30.0f + std::numeric_limits<float>::epsilon());

    if (resolution_over_fhd && framerate_over_30fps) {
      LOG(ERROR) << "UHD 60 fps H.264 decoder is not available on device."
                 << " Content cannot be played.";
      return PP_ERROR_NOTSUPPORTED;
    }

    if (framerate_over_30fps) {
      LOG(WARNING) << "UHD 60 fps H.264 decoder is not available on device."
                   << " Choosing FHD decoder instead.";
      can_use_uhd_decoder = false;
    }
  }
#endif

  // Needed to enable seamless representation switching. Otherwise
  // when we start from representation with resolution lower than UHD,
  // then MM Player will select H/W decoder, which doesn't support
  // seamless resolution switching.
  if (ret == SYSTEM_INFO_ERROR_NONE && uhd_panel_supported &&
      can_use_uhd_decoder)
    player_set_UHD_resolution(player_, true);

  auto format = MMPlayerVideoFormat::Create(stream_info);
  if (!format)
    return PP_ERROR_FAILED;

  video_format_ = std::move(format);
  if (!video_packets_->IsEmpty()) {
    // Pushing configuration packet never fails, even if buffer is full.
    video_packets_->PushBack(
        BufferedConfiguration::Create(PLAYER_STREAM_TYPE_VIDEO, video_format_));
    return PP_OK;
  }
  return ConvertToPPError(player_set_media_stream_info(
      player_, PLAYER_STREAM_TYPE_VIDEO, video_format_->GetMediaFormat()));
}

int32_t PepperMMPlayerAdapter::GetAudioTracksList(
    std::vector<PP_AudioTrackInfo>* track_list) {
  track_list->clear();
  int count;
  int mm_error_code =
      player_get_track_count(player_, PLAYER_STREAM_TYPE_AUDIO, &count);
  if (mm_error_code != PLAYER_ERROR_NONE)
    return ConvertToPPError(mm_error_code);

  std::vector<player_audio_track_info> info_storage(count);
  for (player_audio_track_info& track_info : info_storage)
    memset(&track_info, 0, sizeof(player_audio_track_info));

  mm_error_code = player_get_audio_info(player_, info_storage.data());
  if (mm_error_code != PLAYER_ERROR_NONE)
    return ConvertToPPError(mm_error_code);

  for (player_audio_track_info& track_info : info_storage) {
    track_list->push_back(ConvertToPPAudioTrackInfo(track_info));
  }

  return PP_OK;
}

int32_t PepperMMPlayerAdapter::GetAvailableBitrates(std::string* bitrates) {
  DCHECK(bitrates != nullptr);

  // Algorithm is quite straightforward...
  //
  // First of all, you have to request how many rates you want to get.
  // Then, you have to get actual values.
  // Finally, you have to query how many rates have been returned.

  int error_code = PLAYER_ERROR_GENEREIC;

  static uint32_t bitrates_requested = kBitratesRequestCount;
  error_code = player_set_adaptive_streaming_info(
      player_, &bitrates_requested, PLAYER_ADAPTIVE_INFO_RATE_REQUESTED);
  if (error_code != PLAYER_ERROR_NONE)
    return ConvertToPPError(error_code);

  std::array<uint32_t, kBitratesRequestCount> bitrates_array;
  // player_get_adaptive_streaming_info requires a pointer to array
  auto bitrates_data = bitrates_array.data();
  error_code = player_get_adaptive_streaming_info(
      player_, &bitrates_data, PLAYER_ADAPTIVE_INFO_AVAILABLE_BITRATES);
  if (error_code != PLAYER_ERROR_NONE)
    return ConvertToPPError(error_code);

  uint32_t count = 0;
  error_code = player_get_adaptive_streaming_info(
      player_, &count, PLAYER_ADAPTIVE_INFO_RATE_RETURNED);
  if (error_code != PLAYER_ERROR_NONE)
    return ConvertToPPError(error_code);

  for (uint32_t i = 0; i < count; ++i) {
    bitrates->append(std::to_string(bitrates_array[i]));
    if (i != (count - 1))
      bitrates->append("|");
  }

  return ConvertToPPError(error_code);
}

int32_t PepperMMPlayerAdapter::GetCurrentTime(PP_TimeTicks* time) {
  int mm_player_time_ms = 0;
  int32_t error_code =
      ConvertToPPError(player_get_play_position(player_, &mm_player_time_ms));

  *time = MilisecondsToPPTimeTicks(mm_player_time_ms);
  return error_code;
}

int32_t PepperMMPlayerAdapter::GetCurrentTrack(
    PP_ElementaryStream_Type_Samsung stream_type,
    int32_t* track_index) {
  // player_get_current_track doesn't support PLAYER_STREAM_TYPE_VIDEO
  if (stream_type == PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_VIDEO) {
    return PP_OK;
  }

  int index = 0;
  player_stream_type_e mm_stream_type = ConvertStreamType(stream_type);

  int32_t result = ConvertToPPError(
      player_get_current_track(player_, mm_stream_type, &index));

  *track_index = static_cast<int32_t>(index);
  return result;
}

int32_t PepperMMPlayerAdapter::GetDuration(PP_TimeDelta* duration) {
  int mm_duration = 0;
  int error_code = player_get_duration(player_, &mm_duration);

  if (error_code != PLAYER_ERROR_NONE)
    return PP_ERROR_FAILED;

  *duration = ConvertToPPTimeDelta(mm_duration);
  return PP_OK;
}

int32_t PepperMMPlayerAdapter::SetDuration(PP_TimeDelta /* duration */) {
  return PP_OK;
}

int32_t PepperMMPlayerAdapter::GetPlayerState(PP_MediaPlayerState* state) {
  player_state_e mm_player_state;
  int32_t error_code =
      ConvertToPPError(player_get_state(player_, &mm_player_state));

  *state = ConvertToPPState(mm_player_state);

  return error_code;
}

int32_t PepperMMPlayerAdapter::GetStreamingProperty(
    PP_StreamingProperty property,
    std::string* property_value) {
  int32_t error_code = PP_ERROR_FAILED;

  if (property == PP_STREAMINGPROPERTY_AVAILABLE_BITRATES) {
    error_code = GetAvailableBitrates(property_value);
  } else {
    int key = -1;

    switch (property) {
      case PP_STREAMINGPROPERTY_THROUGHPUT:
        key = PLAYER_ADAPTIVE_INFO_CURRENT_BANDWIDTH;
        break;
      case PP_STREAMINGPROPERTY_DURATION:
        key = PLAYER_ADAPTIVE_INFO_DURATION;
        break;
      case PP_STREAMINGPROPERTY_CURRENT_BITRATE:
        key = PLAYER_ADAPTIVE_INFO_CURRENT_BITRATE;
        break;
      default:
        break;
    }

    if (key != -1) {
      uint32_t value = 0;
      error_code = ConvertToPPError(
          player_get_adaptive_streaming_info(player_, &value, key));
      if (error_code == PP_OK)
        *property_value = std::to_string(value);
    }
  }

  return error_code;
}

int32_t PepperMMPlayerAdapter::GetTextTracksList(
    std::vector<PP_TextTrackInfo>* track_list) {
  track_list->clear();
  int count;
  int mm_error_code =
      player_get_track_count(player_, PLAYER_STREAM_TYPE_TEXT, &count);
  if (mm_error_code != PLAYER_ERROR_NONE)
    return ConvertToPPError(mm_error_code);

  std::vector<PP_TextTrackInfo> info_storage;

  for (int index = 0; index < count; ++index) {
    char* code = nullptr;
    mm_error_code = player_get_track_language_code_ex(
        player_, PLAYER_STREAM_TYPE_TEXT, index, &code);
    if (mm_error_code != PLAYER_ERROR_NONE) {
      free(code);
      break;
    }
    PP_TextTrackInfo trackInfo;
    trackInfo.index = index;
    strncpy(trackInfo.language, code, sizeof(trackInfo.language));
    free(code);
    info_storage.push_back(trackInfo);
  }

  if (mm_error_code != PLAYER_ERROR_NONE)
    return ConvertToPPError(mm_error_code);

  for (PP_TextTrackInfo& trackInfo : info_storage)
    track_list->push_back(trackInfo);

  return PP_OK;
}

int32_t PepperMMPlayerAdapter::GetVideoTracksList(
    std::vector<PP_VideoTrackInfo>* track_list) {
  track_list->clear();

  int width = 0;
  int height = 0;
  int mm_error_code = player_get_video_size(player_, &width, &height);
  if (mm_error_code != PLAYER_ERROR_NONE)
    return ConvertToPPError(mm_error_code);

  int bitrate = 0;
  mm_error_code = player_get_content_bitrate(player_, &bitrate);
  if (mm_error_code != PLAYER_ERROR_NONE)
    return ConvertToPPError(mm_error_code);

  PP_VideoTrackInfo ppInfo;
  ppInfo.index = 0;
  ppInfo.size = PP_MakeSize(width, height);
  ppInfo.bitrate = bitrate;
  track_list->push_back(ppInfo);
  return PP_OK;
}

void PepperMMPlayerAdapter::OnBufferUnderrun(
    PP_ElementaryStream_Type_Samsung stream,
    unsigned long long bytes) {  // NOLINT(runtime/int)
  if (append_packet_runner_) {
    append_packet_runner_->PostTask(
        FROM_HERE, base::Bind(&PepperMMPlayerAdapter::AppendBufferedPackets,
                              base::Unretained(this), stream,
                              // NOLINTNEXTLINE(runtime/int)
                              ClampToMax<int64_t, unsigned long long>(bytes)));
  } else if (ListenerWrapper(stream)) {
    ListenerWrapper(stream)->OnNeedData(
        // NOLINTNEXTLINE(runtime/int)
        ClampToMax<unsigned, unsigned long long>(bytes));
  }
}

void PepperMMPlayerAdapter::OnBufferOverflow(
    PP_ElementaryStream_Type_Samsung stream) {
  if (ListenerWrapper(stream))
    ListenerWrapper(stream)->OnEnoughData();
}

void PepperMMPlayerAdapter::OnInternalBufferFull(
    PP_ElementaryStream_Type_Samsung stream) {
  LOG(WARNING) << "Pepper Media Player " << ToString(stream) << " packet "
               << "buffer is full. New packets won't be accepted until backend "
               << "processes a number of packets that were already submitted.";
  OnBufferOverflow(stream);
}

int32_t PepperMMPlayerAdapter::UnpreparePlayer() {
  if (player_preparing_ != PlayerPreparingState::kPrepared)
    return PP_OK;

  player_preparing_ = PlayerPreparingState::kNone;
  return ConvertToPPError(player_unprepare(player_));
}

void PepperMMPlayerAdapter::StopPlayer() {
  player_state_e mm_player_state = PLAYER_STATE_NONE;
  int mm_error_code = player_get_state(player_, &mm_player_state);
  if (mm_error_code != PLAYER_ERROR_NONE)
    return;

  if (mm_player_state == PLAYER_STATE_PLAYING ||
      mm_player_state == PLAYER_STATE_PAUSED) {
    player_stop(player_);
  }
}

// static
void PepperMMPlayerAdapter::OnStreamBufferStatus(
    PP_ElementaryStream_Type_Samsung stream,
    player_media_stream_buffer_status_e status,
    unsigned long long bytes,  // NOLINT(runtime/int)
    void* user_data) {
  if (!user_data)
    return;

  auto* thiz = static_cast<PepperMMPlayerAdapter*>(user_data);
  switch (status) {
    case PLAYER_MEDIA_STREAM_BUFFER_OVERFLOW:
      thiz->OnBufferOverflow(stream);
      break;
    case PLAYER_MEDIA_STREAM_BUFFER_UNDERRUN:
      thiz->OnBufferUnderrun(stream, bytes);
      break;
    default:
      break;
  }
}

// static
void PepperMMPlayerAdapter::OnStreamSeekData(
    PP_ElementaryStream_Type_Samsung stream,
    unsigned long long offset,  // NOLINT(runtime/int)
    void* user_data) {
  if (!user_data)
    return;

  PP_TimeTicks time_ticks = NanosecondsToPPTimeTicks(offset);

  auto listener_wrapper =
      static_cast<PepperMMPlayerAdapter*>(user_data)->ListenerWrapper(stream);

  if (!listener_wrapper)
    return;

  listener_wrapper->OnSeekData(time_ticks);
  return;
}

// static
void PepperMMPlayerAdapter::OnAudioBufferSatus(
    player_media_stream_buffer_status_e status,
    unsigned long long bytes,  // NOLINT(runtime/int)
    void* user_data) {
  OnStreamBufferStatus(PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_AUDIO, status, bytes,
                       user_data);
}

// static
// NOLINTNEXTLINE(runtime/int)
void PepperMMPlayerAdapter::OnAudioSeekData(unsigned long long offset,
                                            void* user_data) {
  OnStreamSeekData(PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_AUDIO, offset, user_data);
}

// static
void PepperMMPlayerAdapter::OnVideoBufferStatus(
    player_media_stream_buffer_status_e status,
    unsigned long long bytes,  // NOLINT(runtime/int)
    void* user_data) {
  OnStreamBufferStatus(PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_VIDEO, status, bytes,
                       user_data);
}

// static
// NOLINTNEXTLINE(runtime/int)
void PepperMMPlayerAdapter::OnVideoSeekData(unsigned long long offset,
                                            void* user_data) {
  OnStreamSeekData(PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_VIDEO, offset, user_data);
}

void PepperMMPlayerAdapter::RegisterMediaCallbacks(
    PP_ElementaryStream_Type_Samsung type) {
  switch (type) {
    case PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_AUDIO:
      player_set_media_stream_buffer_status_cb_ex(
          player_, PLAYER_STREAM_TYPE_AUDIO, OnAudioBufferSatus, this);
      player_set_media_stream_seek_cb(player_, PLAYER_STREAM_TYPE_AUDIO,
                                      OnAudioSeekData, this);
      break;
    case PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_VIDEO:
      player_set_media_stream_buffer_status_cb_ex(
          player_, PLAYER_STREAM_TYPE_VIDEO, OnVideoBufferStatus, this);
      player_set_media_stream_seek_cb(player_, PLAYER_STREAM_TYPE_VIDEO,
                                      OnVideoSeekData, this);
      break;
    default:
      LOG(ERROR) << "Unknown stream type";
      break;
  }
}

void PepperMMPlayerAdapter::ClearMediaCallbacks() {
  if (audio_media_callback_set_) {
    player_unset_media_stream_buffer_status_cb_ex(player_,
                                                  PLAYER_STREAM_TYPE_AUDIO);
    player_unset_media_stream_seek_cb(player_, PLAYER_STREAM_TYPE_AUDIO);
    audio_media_callback_set_ = false;
  }

  if (video_media_callback_set_) {
    player_unset_media_stream_buffer_status_cb_ex(player_,
                                                  PLAYER_STREAM_TYPE_VIDEO);
    player_unset_media_stream_seek_cb(player_, PLAYER_STREAM_TYPE_VIDEO);
    video_media_callback_set_ = false;
  }
}

// static
void PepperMMPlayerAdapter::OnInitializeDone(void* user_data) {
  if (!user_data)
    return;
  auto adapter = static_cast<PepperMMPlayerAdapter*>(user_data);
  // This is ES Data Source only callback. Simulate buffering 100% event, as ES
  // data source does not produce such events.
  adapter->OnBufferingProgress(100);
}

// static
// NOLINTNEXTLINE(runtime/int)
void PepperMMPlayerAdapter::OnErrorEvent(int error_code, void* user_data) {
  if (!user_data)
    return;
  PlayerOnErrorPrinter(error_code, user_data);
  PP_MediaPlayerError error = ConvertToPPMediaPlayerError(error_code);

  auto player_adapter = static_cast<PepperMMPlayerAdapter*>(user_data);
  if (player_adapter->player_preparing_ == PlayerPreparingState::kPreparing)
    player_adapter->OnPlayerPrepared(PP_ERROR_ABORTED);

  player_adapter->RunCallback<PepperPlayerCallbackType::ErrorCallback>(error);

  auto listener = player_adapter->GetMediaEventsListener();

  if (!listener)
    return;

  listener->OnError(error);
}

int32_t PepperMMPlayerAdapter::SetErrorCallback(
    const base::Callback<void(PP_MediaPlayerError)>& callback) {
  SetCallback<PepperPlayerCallbackType::ErrorCallback>(callback);
  return ConvertToPPError(player_set_error_cb(player_, OnErrorEvent, this));
}

// static
void PepperMMPlayerAdapter::OnEventEnded(void* user_data) {
  if (!user_data)
    return;

  auto listener =
      static_cast<PepperMMPlayerAdapter*>(user_data)->GetMediaEventsListener();

  if (!listener)
    return;

  listener->OnEnded();
}

void PepperMMPlayerAdapter::RegisterToMediaEvents(bool should_register) {
  if (should_register)
    player_set_completed_cb(player_, OnEventEnded, this);
  else
    player_unset_completed_cb(player_);
}

// static
// NOLINTNEXTLINE(runtime/int)
void PepperMMPlayerAdapter::OnSubtitleEvent(unsigned long duration,
                                            void* sub,
                                            unsigned type,
                                            unsigned attri_count,
                                            MMSubAttribute** sub_attribute,
                                            void* thiz) {
  // We support only text subtitle which type is 0, type 1 is for picture
  if (type)
    return;

  if (!thiz)
    return;

  PP_TimeDelta pp_duration = MilisecondsToPPTimeTicks(duration);
  std::string subtitle_string(static_cast<const char*>(sub));

  auto listener =
      static_cast<PepperMMPlayerAdapter*>(thiz)->GetSubtitleListener();

  if (!listener || duration <= 0) {
    // duration == 0 means to hide subtitle. Currently we do not support it.
    return;
  }

  listener->OnShowSubtitle(pp_duration, std::move(subtitle_string));
}

void PepperMMPlayerAdapter::RegisterToSubtitleEvents(bool should_register) {
  if (should_register)
    player_set_subtitle_updated_cb_ex(player_, OnSubtitleEvent, this);
  else
    player_unset_subtitle_updated_cb_ex(player_);
}

void PepperMMPlayerAdapter::RegisterToBufferingEvents(
    bool /* should_register */) {
  // Always receive buffering events for internal bookkeeping. Actual need to
  // emit buffering events is controlled by the presence of BufferingListener.
}

void PepperMMPlayerAdapter::OnBufferingProgress(int percent) {
  if (player_preparing_ == PlayerPreparingState::kPreparing && percent == 100) {
    OnPlayerPrepared(PP_OK);
    // reset display rect and crop ratio if user set them before attach
    RunCallback<PepperPlayerCallbackType::DisplayRectUpdateCallback>();
  }

  auto buffering_listener = GetBufferingListener();
  if (!buffering_listener)
    return;

  if (percent == 0) {
    buffering_started_ = true;
    buffering_listener->OnBufferingStart();
  } else {
    if (!buffering_started_) {
      buffering_started_ = true;
      buffering_listener->OnBufferingStart();
    }
    if (percent == 100)
      buffering_listener->OnBufferingComplete();
    else
      buffering_listener->OnBufferingProgress(percent);
  }
}

// static
void PepperMMPlayerAdapter::OnBufferingEvent(int percent, void* thiz) {
  if (!thiz)
    return;
  auto adapter = static_cast<PepperMMPlayerAdapter*>(thiz);
  adapter->OnBufferingProgress(percent);
}

void PepperMMPlayerAdapter::OnPlayerPreparing() {
  player_preparing_ = PlayerPreparingState::kPreparing;
}

void PepperMMPlayerAdapter::OnPlayerPrepared(int32_t result) {
  player_preparing_ = (result == PP_OK ? PlayerPreparingState::kPrepared
                                       : PlayerPreparingState::kNone);
  // If stop callback is set, then it means a Stop was called. Stop callback
  // cannot be called when player is in preparing state (i.e. buffering is not
  // finished yet), because it leads to weird errors. Therefore a call to stop
  // callback is postponed until preparing finishes.
  ExecuteAndResetCallback(result, &stop_callback_);
}

scoped_refptr<MMPlayerMediaFormat> PepperMMPlayerAdapter::GetMediaFormat(
    PP_ElementaryStream_Type_Samsung stream_type) {
  switch (stream_type) {
    case PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_AUDIO:
      return audio_format_;
    case PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_VIDEO:
      return video_format_;
    default:
      return nullptr;
  }
}

MMPacketBuffer* PepperMMPlayerAdapter::GetPacketsBuffer(
    PP_ElementaryStream_Type_Samsung stream_type) {
  switch (stream_type) {
    case PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_AUDIO:
      return audio_packets_.get();
    case PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_VIDEO:
      return video_packets_.get();
    default:
      return nullptr;
  }
}

int32_t PepperMMPlayerAdapter::PushMediaStream(
    PP_ElementaryStream_Type_Samsung stream_type,
    std::unique_ptr<BufferedPacket> pkt) {
  if (!append_packet_runner_)
    append_packet_runner_ = base::MessageLoop::current()->task_runner();

  if (!pkt)
    return PP_ERROR_FAILED;

  auto* buffer = GetPacketsBuffer(stream_type);
  DCHECK(buffer != nullptr);
  if (!buffer->IsEmpty()) {
    if (!buffer->PushBack(std::move(pkt))) {
      OnInternalBufferFull(stream_type);
      return PP_ERROR_NOQUOTA;
    }
    return PP_OK;
  }

  int ret = pkt->ProcessPacket(player_);
  if (ret == PLAYER_ERROR_NONE)
    return PP_OK;

  if (ret == PLAYER_ERROR_BUFFER_SPACE) {
    if (!buffer->PushBack(std::move(pkt))) {
      OnInternalBufferFull(stream_type);
      return PP_ERROR_NOQUOTA;
    }
    return PP_OK;
  }

  LOG(WARNING) << "Got error while appending packet for stream: " << stream_type
               << " error: " << ret;
  return ConvertToPPError(ret);
}

void PepperMMPlayerAdapter::AppendBufferedPackets(
    PP_ElementaryStream_Type_Samsung stream_type,
    int64_t bytes) {
  auto* buffer = GetPacketsBuffer(stream_type);
  while (!buffer->IsEmpty() && !packets_flush_requested_) {
    auto pkt = buffer->PeekFront();
    int ret = pkt->ProcessPacket(player_);
    if (ret == PLAYER_ERROR_NONE) {
      bytes -= pkt->PacketSize();
      buffer->PopFront();
    } else {
      LOG_IF(WARNING, ret != PLAYER_ERROR_BUFFER_SPACE)
          << "Got error while appending packet for stream: " << stream_type
          << " error: " << ret;
      break;
    }
  }

  if (buffer->IsEmpty()) {
    auto* listener_wrapper = ListenerWrapper(stream_type);
    if (listener_wrapper) {
      listener_wrapper->OnNeedData(
          ClampToMax<unsigned, int64_t>(std::max<int64_t>(bytes, 0)));
    }
  }
}

}  // namespace content
