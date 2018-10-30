// Copyright (c) 2014 The Samsung Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_AUDIO_TIZEN_CAPI_AUDIO_INPUT_H_
#define MEDIA_AUDIO_TIZEN_CAPI_AUDIO_INPUT_H_

#include <audio_io.h>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/threading/thread.h"
#include "base/time/time.h"
#include "media/audio/agc_audio_stream.h"
#include "media/audio/audio_device_name.h"
#include "media/audio/audio_io.h"
#include "media/audio/tizen/capi_util.h"
#include "media/base/audio_block_fifo.h"
#include "media/base/audio_parameters.h"

namespace media {

class AudioManagerCapi;

class CapiAudioInputStream final : public AgcAudioStream<AudioInputStream> {
 public:
  CapiAudioInputStream(AudioManagerCapi* audio_manager,
                       const std::string& device_name,
                       const AudioParameters& params);

  ~CapiAudioInputStream() override;

  bool Open() override;
  void Start(AudioInputCallback* callback) override;
  void Stop() override;
  void Close() override;
  double GetMaxVolume() override;
  void SetVolume(double volume) override;
  double GetVolume() override;
  bool IsMuted();
  void OnAudioIOData(const AudioBus* audio_bus,
                     double hardware_delay_seconds,
                     double normalized_volume);

 private:
  static void AudioStreamReadCB(audio_in_h handle,
                                size_t nbytes,
                                void* user_data);
  void ReadAudioData();
  void CloseInternal(bool success);

  AudioManagerCapi* audio_manager_;
  AudioInputCallback* callback_;
  std::string device_name_;
  AudioParameters params_;
  double volume_;
  media::InternalState state_;
  base::Thread audio_worker_;
  audio_in_h device_;

#if defined(OS_TIZEN_TV_PRODUCT)
  sound_device_list_h device_list_;
  sound_device_h sound_device_;
  sound_stream_info_h stream_info_;
  bool device_added_for_stream_routing_;
#endif

  // Holds the data from the OS.
  AudioBlockFifo fifo_;

  DISALLOW_COPY_AND_ASSIGN(CapiAudioInputStream);
};

}  // namespace media

#endif  // MEDIA_AUDIO_TIZEN_CAPI_AUDIO_INPUT_H_
