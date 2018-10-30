// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_GPU_RTC_MM_VIDEO_DECODER_H_
#define CONTENT_RENDERER_MEDIA_GPU_RTC_MM_VIDEO_DECODER_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/synchronization/lock.h"
#include "content/common/content_export.h"
#include "third_party/webrtc/modules/video_coding/include/video_codec_interface.h"

namespace media {
class GpuVideoAcceleratorFactories;
}  // namespace media

namespace content {

class CONTENT_EXPORT RTCMMVideoDecoder : public webrtc::VideoDecoder {
 public:
  static std::unique_ptr<webrtc::VideoDecoder> Create(
      webrtc::VideoCodecType type,
      media::GpuVideoAcceleratorFactories* factories);
  static void Destroy(webrtc::VideoDecoder* decoder,
                      media::GpuVideoAcceleratorFactories* factories);

  int32_t InitDecode(const webrtc::VideoCodec* codec_settings,
                     int32_t number_of_cores) override;
  int32_t Decode(const webrtc::EncodedImage& input_image,
                 bool missing_frames,
                 const webrtc::RTPFragmentationHeader* fragmentation,
                 const webrtc::CodecSpecificInfo* codec_specific_info,
                 int64_t render_time_ms) override;
  int32_t RegisterDecodeCompleteCallback(
      webrtc::DecodedImageCallback* callback) override;
  int32_t Release() override;

  webrtc::VideoDecoderData GetDecoderData() const override;

  const char* ImplementationName() const override { return "mmdecoder"; }

  void Suspend() override;
  void Resume() override;

 private:
  RTCMMVideoDecoder();
  ~RTCMMVideoDecoder();
  webrtc::DecodedImageCallback* decode_complete_callback_ = nullptr;
  webrtc::VideoCodec codec_;

  using DecodeFunction =
      int32_t (RTCMMVideoDecoder::*)(const webrtc::EncodedImage&,
                                     bool,
                                     const webrtc::RTPFragmentationHeader*,
                                     const webrtc::CodecSpecificInfo*,
                                     int64_t);

  void ProvideSuspendResumeInterface();

  int32_t DecodeSuspended(const webrtc::EncodedImage&,
                          bool,
                          const webrtc::RTPFragmentationHeader*,
                          const webrtc::CodecSpecificInfo*,
                          int64_t);
  int32_t DecodeResuming(const webrtc::EncodedImage& input_image,
                         bool missingFrames,
                         const webrtc::RTPFragmentationHeader* fragmentation,
                         const webrtc::CodecSpecificInfo* codecSpecificInfo,
                         int64_t renderTimeMs);
  int32_t DecodeResumed(
      const webrtc::EncodedImage& inputImage,
      bool missingFrames,
      const webrtc::RTPFragmentationHeader* fragmentation,
      const webrtc::CodecSpecificInfo* codecSpecificInfo = NULL,
      int64_t renderTimeMs = -1);

  base::Lock suspend_resume_lock_;
  bool trigger_key_frame_after_resume_{false};

  std::shared_ptr<webrtc::SuspendingDecoder> sr_decoder_;

  DecodeFunction decode_function_{nullptr};

  DISALLOW_COPY_AND_ASSIGN(RTCMMVideoDecoder);
};
}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_GPU_RTC_MM_VIDEO_DECODER_H_
