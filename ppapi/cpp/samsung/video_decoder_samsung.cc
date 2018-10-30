// Copyright (c) 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/cpp/samsung/video_decoder_samsung.h"

#include "ppapi/c/pp_errors.h"
#include "ppapi/cpp/instance_handle.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/module_impl.h"

namespace pp {

namespace {

template <>
const char* interface_name<PPB_VideoDecoder_Samsung_1_0>() {
  return PPB_VIDEODECODER_SAMSUNG_INTERFACE_1_0;
}

}  // namespace

VideoDecoderSamsung::VideoDecoderSamsung() : VideoDecoder() {}

VideoDecoderSamsung::VideoDecoderSamsung(const VideoDecoderSamsung& other)
    : VideoDecoder(other) {}

VideoDecoderSamsung::VideoDecoderSamsung(const VideoDecoder& other)
    : VideoDecoder(other) {}

VideoDecoderSamsung::VideoDecoderSamsung(const InstanceHandle& instance)
    : VideoDecoder(instance) {}

int32_t VideoDecoderSamsung::SetLatencyMode(PP_VideoLatencyMode mode) {
  if (has_interface<PPB_VideoDecoder_Samsung_1_0>()) {
    return get_interface<PPB_VideoDecoder_Samsung_1_0>()->SetLatencyMode(
        pp_resource(), mode);
  }
  return PP_ERROR_NOINTERFACE;
}

}  // namespace pp
