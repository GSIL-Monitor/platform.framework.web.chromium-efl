// Copyright (c) 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_CPP_SAMSUNG_VIDEO_DECODER_SAMSUNG_H_
#define PPAPI_CPP_SAMSUNG_VIDEO_DECODER_SAMSUNG_H_

#include "ppapi/c/pp_stdint.h"
#include "ppapi/c/samsung/ppb_video_decoder_samsung.h"
#include "ppapi/cpp/resource.h"
#include "ppapi/cpp/video_decoder.h"

/// @file
/// This file defines the interface for extending an
/// video decoder resource within the browser.

namespace pp {

class InstanceHandle;

class VideoDecoderSamsung : public VideoDecoder {
 public:
  /// An empty constructor for an <code>VideoDecoderSamsung</code> resource.
  VideoDecoderSamsung();

  /// A constructor used to create a <code>VideoDecoderSamsung</code>
  /// and associate it with the provided <code>Instance</code>.
  /// @param[in] instance The instance with which this resource will be
  /// associated.
  explicit VideoDecoderSamsung(const InstanceHandle& instance);

  /// Constructs a <code>VideoDecoderSamsung</code> from
  /// a <code>VideoDecoder</code>.
  ///
  /// @param[in] other A reference to a <code>VideoDecoder</code>.
  explicit VideoDecoderSamsung(const VideoDecoder& other);

  /// The copy constructor for <code>VideoDecoderSamsung</code>.
  /// @param[in] other A reference to a <code>VideoDecoderSamsung</code>.
  explicit VideoDecoderSamsung(const VideoDecoderSamsung& other);

  /// Sets specified latency mode for <code>PPB_VideoDecoder</code> resource.
  /// This method must be called before calling Initialize method from
  /// <code>PPB_VideoDecoder</code> API.
  ///
  /// Setting low latency mode results in much smaller delay between input data
  /// and decoded frames. In this mode only 'I' and 'P' frames can be used.
  /// Passing 'B' frame may result in error.
  ///
  /// In low latency mode, it is possible to call GetPicture method from
  /// <code>PPB_VideoDecoder</code> API after each decode.
  ///
  /// If not specified, PP_VIDEOLATENCYMODE_NORMAL will be used.
  ///
  /// @param[in] video_decoder A <code>PP_Resource</code> identifying the video
  /// decoder.
  /// @param[in] mode A <code>PP_VideoLatencyMode</code> to return to
  /// the decoder.
  ///
  /// @return An int32_t containing a result code from
  /// <code>pp_errors.h</code>.
  int32_t SetLatencyMode(PP_VideoLatencyMode mode);
};

}  // namespace pp

#endif  // PPAPI_CPP_SAMSUNG_VIDEO_DECODER_SAMSUNG_H_
