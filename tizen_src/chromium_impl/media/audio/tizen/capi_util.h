// Copyright (c) 2014 The Samsung Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Utils to use tizen Core API for audio output/input.
// (tizen capi-media-audio-io.)

#ifndef MEDIA_AUDIO_TIZEN_CAPI_UTIL_H_
#define MEDIA_AUDIO_TIZEN_CAPI_UTIL_H_

#include <audio_io.h>
#include <sound_manager.h>

#include "media/base/audio_parameters.h"

namespace media {

enum InternalState {
  kInError = 0,
  kIsOpened,
  kIsStarted,
  kIsStopped,
  kIsClosed
};

audio_channel_e ToCapiAudioEnum(media::ChannelLayout e);

audio_sample_type_e ToCapiBPSEnum(int bitPerSecond);

int GetAudioInputBufferSize(audio_in_h audio_in);

int GetAudioOutputBufferSize(audio_out_h audio_out,
                             int sample_rate,
                             media::ChannelLayout channel_layout,
                             int bit_per_sample);

int GetAudioOutputSampleRate(audio_out_h audio_out,
                             int sample_rate,
                             media::ChannelLayout channel_layout,
                             int bit_per_sample);

int LatencyInBytes(int latency_milli, int sample_rate, int bytes_per_frame);

int GetAudioInLatencyInBytes(audio_in_h audio_in, int bytes_per_frame);

int GetAudioOutLatencyInBytes(audio_out_h audio_out, int bytes_per_frame);

}  // namespace media

#endif  // MEDIA_AUDIO_TIZEN_CAPI_UTIL_H_
