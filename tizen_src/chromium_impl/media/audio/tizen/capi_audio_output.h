// Copyright (c) 2014 The Samsung Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Creates an audio output stream based on the Tizen Core API.
// Tizen Core api uses PULSE Synchronized API.

#ifndef MEDIA_AUDIO_TIZEN_CAPI_AUDIO_OUTPUT_H_
#define MEDIA_AUDIO_TIZEN_CAPI_AUDIO_OUTPUT_H_

#include <audio_io.h>
#include <sound_manager.h>

#include "base/threading/thread.h"
#include "media/audio/audio_io.h"
#include "media/audio/tizen/capi_util.h"
#include "media/base/audio_parameters.h"

namespace media {

class AudioManagerBase;

class CapiAudioOutputStream : public AudioOutputStream {
 public:
  CapiAudioOutputStream(const AudioParameters& params,
                        AudioManagerBase* manager);

  ~CapiAudioOutputStream() override;

  bool Open() override;
  void Close() override;
  void Start(AudioSourceCallback* callback) override;
  void Stop() override;
  void SetVolume(double volume) override;
  void GetVolume(double* volume) override;
  void HandleError();

 private:
  void OnAudioIOData();
  static void AudioStreamWriteCB(audio_out_h handle,
                                 size_t nbytes,
                                 void* user_data);
  void WriteAudioData(size_t nbytes);

  const AudioParameters params_;
  AudioManagerBase* manager_;
  audio_out_h audio_out_;
  double volume_;
  char* buffer_;
  media::InternalState state_;
  AudioSourceCallback* source_callback_;
  std::unique_ptr<AudioBus> audio_bus_;

  DISALLOW_COPY_AND_ASSIGN(CapiAudioOutputStream);
};

}  // namespace media

#endif  // MEDIA_AUDIO_TIZEN_CAPI_AUDIO_OUTPUT_H_
