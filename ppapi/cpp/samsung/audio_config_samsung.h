// Copyright (c) 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_CPP_SAMSUNG_AUDIO_CONFIG_SAMSUNG_H_
#define PPAPI_CPP_SAMSUNG_AUDIO_CONFIG_SAMSUNG_H_

#include "ppapi/c/pp_stdint.h"
#include "ppapi/c/samsung/ppb_audio_config_samsung.h"
#include "ppapi/cpp/audio_config.h"
#include "ppapi/cpp/resource.h"

/// @file
/// This file defines the interface for extending an
/// audio configuration resource within the browser.

namespace pp {

class InstanceHandle;

class AudioConfigSamsung : public AudioConfig {
 public:
  /// An empty constructor for an <code>AudioConfigSamsung</code> resource.
  AudioConfigSamsung();

  /// A constructor that creates an audio config based on the given sample rate
  /// and frame count.
  ///
  /// For details check AudioConfig class.
  ///
  /// @param[in] instance The instance associated with this resource.
  ///
  /// @param[in] sample_rate A <code>PP_AudioSampleRate</code> which is either
  /// <code>PP_AUDIOSAMPLERATE_44100</code> or
  /// <code>PP_AUDIOSAMPLERATE_48000</code>.
  ///
  /// @param[in] sample_frame_count A uint32_t frame count returned from the
  /// <code>RecommendSampleFrameCount</code> function.
  AudioConfigSamsung(const InstanceHandle& instance,
                     PP_AudioSampleRate sample_rate,
                     uint32_t sample_frame_count);

  /// Sets specified audio mode for the given audio config.
  ///
  /// @param[in] audio_mode Selected audio mode.
  ///
  /// @return An int32_t containing a result code from
  /// <code>pp_errors.h</code>.
  int32_t SetAudioMode(PP_AudioMode audio_mode);
};

}  // namespace pp

#endif  // PPAPI_CPP_SAMSUNG_AUDIO_CONFIG_SAMSUNG_H_
