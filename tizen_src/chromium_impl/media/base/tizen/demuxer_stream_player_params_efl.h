// Copyright 2014 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_TIZEN_DEMUXER_STREAM_PLAYER_PARAMS_EFL_H_
#define MEDIA_BASE_TIZEN_DEMUXER_STREAM_PLAYER_PARAMS_EFL_H_

#include <vector>

#include "media/base/audio_decoder_config.h"
#include "media/base/decrypt_config.h"
#include "media/base/demuxer_stream.h"
#include "media/base/media_export.h"
#include "media/base/video_decoder_config.h"
#include "ui/gfx/geometry/size.h"

namespace media {

struct MEDIA_EXPORT DemuxerConfigs {
  DemuxerConfigs();
  ~DemuxerConfigs();

  AudioCodec audio_codec;
  int audio_channels;
  int audio_sampling_rate;
  int audio_bit_rate;
  bool is_audio_encrypted;
  std::vector<uint8_t> audio_extra_data;

  VideoCodec video_codec;
  gfx::Size video_size;
  bool is_video_encrypted;
  std::vector<uint8_t> video_extra_data;
#if defined(OS_TIZEN_TV_PRODUCT)
  std::string webm_hdr_info;
  int framerate_num;
  int framerate_den;
  bool is_framerate_changed;
#endif

  int duration_ms;
};

struct MEDIA_EXPORT DemuxedBufferMetaData {
  DemuxedBufferMetaData();
  ~DemuxedBufferMetaData();
#if defined(OS_TIZEN_TV_PRODUCT)
  int Release();
#endif

  int size;
  bool end_of_stream;
  base::TimeDelta timestamp;
  base::TimeDelta time_duration;
  DemuxerStream::Type type;
  DemuxerStream::Status status;
#if defined(OS_TIZEN_TV_PRODUCT)
  int tz_handle = 0;  // Handle to data decrypted in the trusted zone
#endif
};

};  // namespace media

#endif  // MEDIA_BASE_TIZEN_DEMUXER_STREAM_PLAYER_PARAMS_EFL_H_
