// Copyright (c) 2014 The Samsung Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/tizen/capi_util.h"

#include <audio_io.h>

#include "base/logging.h"
#include "base/time/time.h"

namespace {

const int kDefaultSampleRate = 44100;
const int kDefaultBitsPerSec = 8;
}

namespace media {

audio_channel_e ToCapiAudioEnum(media::ChannelLayout e) {
  if (CHANNEL_LAYOUT_STEREO == e) {
    return AUDIO_CHANNEL_STEREO;
  }
  return AUDIO_CHANNEL_MONO;
}

audio_sample_type_e ToCapiBPSEnum(int bits_per_sec) {
  if (bits_per_sec == kDefaultBitsPerSec) {
    return AUDIO_SAMPLE_TYPE_U8;
  }
  return AUDIO_SAMPLE_TYPE_S16_LE;
}

int GetAudioInputBufferSize(audio_in_h audio_in) {
  int buffer_size = 0;
  bool local_audio_handle = false;
  int err = AUDIO_IO_ERROR_NONE;

  if (audio_in) {
    // |audio_in| is created as well as prepared.
    err = audio_in_get_buffer_size(audio_in, &buffer_size);
    if (AUDIO_IO_ERROR_NONE == err && buffer_size > 0) {
      return buffer_size;
    } else {
      LOG(WARNING) << "|audio_in| need to be prepared.";
    }
  } else {
    // create |audio_in| locally to get buffer size.
    err = audio_in_create(kDefaultSampleRate,
                          AUDIO_CHANNEL_STEREO,
                          AUDIO_SAMPLE_TYPE_S16_LE,
                          &audio_in);
    if (AUDIO_IO_ERROR_NONE != err) {
      LOG(ERROR) << "audio_in_create() failed, Error code " << err;
      return 0;
    }
    local_audio_handle = true;
  }

  if (AUDIO_IO_ERROR_NONE ==
          audio_in_prepare(audio_in)) {
    audio_in_get_buffer_size(audio_in, &buffer_size);
    audio_in_unprepare(audio_in);
  }

  if (local_audio_handle) {
    audio_in_destroy(audio_in);
  }
  return buffer_size;
}

int GetAudioOutputBufferSize(audio_out_h audio_out,
                             int sample_rate,
                             media::ChannelLayout channel_layout,
                             int bit_per_sample) {
  int buffer_size = 0;
  bool local_audio_handle = false;
  int err = AUDIO_IO_ERROR_NONE;

  if (audio_out) {
    // |audio_out| is created as well as prepared.
    err = audio_out_get_buffer_size(audio_out, &buffer_size);
    if (AUDIO_IO_ERROR_NONE == err && buffer_size > 0) {
      return buffer_size;
    } else {
      LOG(WARNING) << "|audio_out| need to be prepared.";
    }
  } else {
    // create |audio_out| locally to get buffer size.
    err = audio_out_create_new(sample_rate,
                               ToCapiAudioEnum(channel_layout),
                               ToCapiBPSEnum(bit_per_sample),
                               &audio_out);
    if (AUDIO_IO_ERROR_NONE != err) {
      LOG(ERROR) << "audio_out_create_new() failed, Error code " << err;
      return 0;
    }
    local_audio_handle = true;
  }

  if (AUDIO_IO_ERROR_NONE ==
          audio_out_prepare(audio_out)) {
    audio_out_get_buffer_size(audio_out, &buffer_size);
    audio_out_unprepare(audio_out);
  }

  if (local_audio_handle) {
    audio_out_destroy(audio_out);
  }
  return buffer_size;
}

int GetAudioOutputSampleRate(audio_out_h audio_out,
                             int sample_rate,
                             media::ChannelLayout channel_layout,
                             int bit_per_sample) {
  int err = AUDIO_IO_ERROR_NONE;

  // If NULL try to create audio handle with passed sample rate.
  if (audio_out == NULL) {
    err = audio_out_create_new(sample_rate,
                               ToCapiAudioEnum(channel_layout),
                               ToCapiBPSEnum(bit_per_sample),
                               &audio_out);
    if (AUDIO_IO_ERROR_NONE == err) {
      audio_out_destroy(audio_out);
      return sample_rate;
    }
    return 0;
  }

  // If not NULL get samplerate of passed audio_out handle.
  if (AUDIO_IO_ERROR_NONE !=
          audio_out_get_sample_rate(audio_out, &sample_rate)) {
    return 0;
  }

  return sample_rate;
}

int LatencyInBytes(int latency_milli, int sample_rate, int bytes_per_frame) {
  return latency_milli * sample_rate * bytes_per_frame /
      base::Time::kMillisecondsPerSecond;
}

int GetAudioInLatencyInBytes(audio_in_h audio_in, int bytes_per_frame) {
  DCHECK(audio_in != NULL);
  // FIXME: Find a way to get audio input device latency.
  static int latency_milli = 0;
  static int sample_rate = 0;

  if (sample_rate == 0 && AUDIO_IO_ERROR_NONE !=
          audio_in_get_sample_rate(audio_in, &sample_rate))
    return 0;

  return LatencyInBytes(latency_milli, sample_rate, bytes_per_frame);
}

int GetAudioOutLatencyInBytes(audio_out_h audio_out, int bytes_per_frame) {
  DCHECK(audio_out != NULL);
  // FIXME: Find a way to get audio output device latency.
  static int latency_milli = 0;
  static int sample_rate = 0;

  if (sample_rate == 0 && AUDIO_IO_ERROR_NONE !=
          audio_out_get_sample_rate(audio_out, &sample_rate))
    return 0;

  return LatencyInBytes(latency_milli, sample_rate, bytes_per_frame);
}

}  // namespace media
