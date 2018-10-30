// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_AUDIO_TIZEN_MM_PLAYER_AUDIO_OUTPUT_H_
#define MEDIA_AUDIO_TIZEN_MM_PLAYER_AUDIO_OUTPUT_H_

#include <player.h>
#include <player_internal.h>
#include <player_product.h>

#include <atomic>

#include "base/threading/thread.h"
#include "media/audio/audio_io.h"
#include "media/base/audio_parameters.h"

namespace media {

class AudioManagerBase;

class MMPlayerAudioOutputStream : public AudioOutputStream {
 public:
  MMPlayerAudioOutputStream(const AudioParameters& params,
                            AudioManagerBase* manager);

  ~MMPlayerAudioOutputStream() override;

  bool Open() override;
  void Close() override;
  void Start(AudioSourceCallback* callback) override;
  void Stop() override;
  void SetVolume(double volume) override;
  void GetVolume(double* volume) override;
  void HandleError();

 private:
  static void OnBufferStatusCB(
      player_media_stream_buffer_status_e status,
      unsigned long long nbytes,  // NOLINT(runtime/int)
      void* user_data);
  void OnBufferStatus(player_media_stream_buffer_status_e status,
                      unsigned long long nbytes);  // NOLINT(runtime/int)

  void PushPacketsRoutine();

  enum State { kInError = 0, kIsOpened, kIsStarted, kIsStopped, kIsClosed };

  const AudioParameters params_;
  AudioManagerBase* manager_;
  player_h player_handle_;
  std::atomic<double> volume_;
  std::vector<uint8_t> buffer_;
  State state_;
  AudioSourceCallback* source_callback_;
  std::unique_ptr<AudioBus> audio_bus_;
  base::Thread push_packets_thread_;
  std::atomic<bool> push_packets_;

  DISALLOW_COPY_AND_ASSIGN(MMPlayerAudioOutputStream);
};

}  // namespace media

#endif  // MEDIA_AUDIO_TIZEN_MM_PLAYER_AUDIO_OUTPUT_H_
