// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/gpu/rtc_mm_video_decoder.h"

#include <vector>

#include "base/bind.h"
#include "base/logging.h"
#include "media/video/gpu_video_accelerator_factories.h"
#include "third_party/webrtc/api/video/video_frame_buffer.h"
#include "third_party/webrtc/common_video/include/video_frame_buffer.h"

namespace content {

std::unique_ptr<webrtc::VideoDecoder> RTCMMVideoDecoder::Create(
    webrtc::VideoCodecType,
    media::GpuVideoAcceleratorFactories*) {
  return std::unique_ptr<webrtc::VideoDecoder>(new RTCMMVideoDecoder());
}

int32_t RTCMMVideoDecoder::InitDecode(const webrtc::VideoCodec* codec_settings,
                                      int32_t /*number_of_cores*/) {
  memcpy(&codec_, codec_settings, sizeof(webrtc::VideoCodec));
  DLOG(INFO) << "Codec type: " << codec_.codecType << " size: (" << codec_.width
             << " x " << codec_.height << ")";
  return WEBRTC_VIDEO_CODEC_OK;
}

// static
void RTCMMVideoDecoder::Destroy(
    webrtc::VideoDecoder* decoder,
    media::GpuVideoAcceleratorFactories* factories) {
  factories->GetTaskRunner()->DeleteSoon(FROM_HERE, decoder);
}

RTCMMVideoDecoder::RTCMMVideoDecoder() {
  decode_function_ = &RTCMMVideoDecoder::DecodeResumed;
}

int32_t RTCMMVideoDecoder::Decode(
    const webrtc::EncodedImage& inputImage,
    bool missingFrames,
    const webrtc::RTPFragmentationHeader* fragmentation,
    const webrtc::CodecSpecificInfo* codecSpecificInfo,
    int64_t renderTimeMs) {
  return (this->*decode_function_)(inputImage, missingFrames, fragmentation,
                                   codecSpecificInfo, renderTimeMs);
}

int32_t RTCMMVideoDecoder::DecodeSuspended(
    const webrtc::EncodedImage&,
    bool,
    const webrtc::RTPFragmentationHeader*,
    const webrtc::CodecSpecificInfo*,
    int64_t) {
  return WEBRTC_VIDEO_CODEC_FALLBACK_SOFTWARE;
}

int32_t RTCMMVideoDecoder::DecodeResuming(
    const webrtc::EncodedImage& inputImage,
    bool missingFrames,
    const webrtc::RTPFragmentationHeader* fragmentation,
    const webrtc::CodecSpecificInfo* codecSpecificInfo,
    int64_t renderTimeMs) {
  if (inputImage._frameType == webrtc::kVideoFrameKey) {
    base::AutoLock auto_lock(suspend_resume_lock_);
    decode_complete_callback_->OnDecoderType(GetDecoderData());
    if (trigger_key_frame_after_resume_) {
      trigger_key_frame_after_resume_ = false;
    }
    decode_function_ = &RTCMMVideoDecoder::DecodeResumed;
    return (this->*decode_function_)(inputImage, missingFrames, fragmentation,
                                     codecSpecificInfo, renderTimeMs);
  }
  // After resume is called, in order to switch to hardware decoding, decoder
  // needs to get video key frame. In order to do it, decode function has to
  // return an error. RTCMMVideoDecoder returns new error, so it can
  // propagate itself to upper layers, but also trigger decoding current
  // frame on software decoder to avoid glitches
  if (trigger_key_frame_after_resume_) {
    trigger_key_frame_after_resume_ = false;
    return WEBRTC_VIDEO_CODEC_SUSPEND_RESUME_TRANSITION;
  }
  return WEBRTC_VIDEO_CODEC_FALLBACK_SOFTWARE;
}

int32_t RTCMMVideoDecoder::DecodeResumed(
    const webrtc::EncodedImage& input_image,
    bool /*missing_frames*/,
    const webrtc::RTPFragmentationHeader* /*fragmentation*/,
    const webrtc::CodecSpecificInfo* /*codec_specific_info*/,
    int64_t render_time_ms) {
  DCHECK(decode_complete_callback_);

  auto width = input_image._encodedWidth ?: codec_.width;
  auto height = input_image._encodedHeight ?: codec_.height;

  rtc::scoped_refptr<webrtc::PlanarYuvBuffer> buffer =
      new rtc::RefCountedObject<webrtc::WrappedEncodedBuffer>(
          width, height, input_image._buffer, input_image._length);
  webrtc::VideoFrame frame(buffer, input_image._timeStamp, render_time_ms,
                           input_image.rotation_);

  decode_complete_callback_->Decoded(frame);
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t RTCMMVideoDecoder::RegisterDecodeCompleteCallback(
    webrtc::DecodedImageCallback* callback) {
  decode_complete_callback_ = callback;

  sr_decoder_ = std::shared_ptr<webrtc::SuspendingDecoder>(
      new webrtc::SuspendingDecoder());
  sr_decoder_->RegisterDecoder(this);

  decode_complete_callback_->OnSuspendingDecoder(sr_decoder_);
  decode_complete_callback_->OnDecoderType(GetDecoderData());
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t RTCMMVideoDecoder::Release() {
  return WEBRTC_VIDEO_CODEC_OK;
}

webrtc::VideoDecoderData RTCMMVideoDecoder::GetDecoderData() const {
  return {webrtc::VideoDecoderType::ENCODED, codec_};
}

void RTCMMVideoDecoder::Suspend() {
  base::AutoLock auto_lock(suspend_resume_lock_);
  decode_function_ = &RTCMMVideoDecoder::DecodeSuspended;
  trigger_key_frame_after_resume_ = true;
}

void RTCMMVideoDecoder::Resume() {
  base::AutoLock auto_lock(suspend_resume_lock_);
  decode_function_ = &RTCMMVideoDecoder::DecodeResuming;
}

RTCMMVideoDecoder::~RTCMMVideoDecoder() {
  if (sr_decoder_)
    sr_decoder_->UnregisterDecoder();
}

}  // namespace content
