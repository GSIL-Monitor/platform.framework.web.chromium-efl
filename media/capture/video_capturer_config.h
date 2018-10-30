#ifndef MEDIA_CAPTURE_VIDEO_CAPTURER_CONFIG_H_
#define MEDIA_CAPTURE_VIDEO_CAPTURER_CONFIG_H_

#include <ostream>

#include "base/memory/ref_counted.h"
#include "media/base/video_decoder_config.h"
#include "third_party/webrtc/api/video_codecs/video_decoder.h"

namespace media {

enum class TizenDecoderType { ENCODED, TEXTURE, UNKNOWN };

inline const char* TizenDecoderTypeToSting(TizenDecoderType type) {
  switch (type) {
    case TizenDecoderType::ENCODED:
      return "MM";
    case TizenDecoderType::TEXTURE:
      return "TEXTURE";
    default:
      return "UNKNOWN";
  }
}

class SuspendingDecoder : public base::RefCountedThreadSafe<SuspendingDecoder> {
 public:
  explicit SuspendingDecoder(
      std::shared_ptr<webrtc::SuspendingDecoder> sr_decoder)
      : decoder_(std::move(sr_decoder)) {}
  void Suspend() const { decoder_->Suspend(); }
  void Resume() const { decoder_->Resume(); }

 private:
  std::shared_ptr<webrtc::SuspendingDecoder> decoder_;
};

class MediaStreamVideoDecoderData
    : public base::RefCountedThreadSafe<MediaStreamVideoDecoderData> {
 public:
  MediaStreamVideoDecoderData(TizenDecoderType type_,
                              const VideoDecoderConfig& config_)
      : type(type_), config(config_) {}
  TizenDecoderType type;
  VideoDecoderConfig config;
};

inline std::ostream& operator<<(std::ostream& os, TizenDecoderType type) {
  return os << TizenDecoderTypeToSting(type);
}

inline std::ostream& operator<<(std::ostream& os,
                                const MediaStreamVideoDecoderData& data) {
  return os << "Type: " << data.type << " codec: " << data.config.codec();
}
}  // namespace media

#endif  // MEDIA_CAPTURE_VIDEO_CAPTURER_CONFIG_H_
