// Copyright (c) 2014 The Samsung Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_AUDIO_TIZEN_AUDIO_MANAGER_CAPI_H_
#define MEDIA_AUDIO_TIZEN_AUDIO_MANAGER_CAPI_H_

#include <string>

#include "base/macros.h"
#include "base/synchronization/lock.h"
#include "base/synchronization/waitable_event.h"
#include "media/audio/audio_manager_base.h"

namespace media {

class MuteableAudioOutputStream;

class MEDIA_EXPORT AudioManagerCapi : public AudioManagerBase {
 public:
  explicit AudioManagerCapi(std::unique_ptr<AudioThread> audio_thread,
                            AudioLogFactory* audio_log_factory);
  ~AudioManagerCapi() override;

  // Implementation of AudioManager.
  bool HasAudioOutputDevices() override;
  bool HasAudioInputDevices() override;
  void GetAudioInputDeviceNames(AudioDeviceNames* device_names) override;
  void GetAudioOutputDeviceNames(AudioDeviceNames* device_names) override;
  AudioParameters GetInputStreamParameters(
      const std::string& device_id) override;

  AudioOutputStream* MakeAudioOutputStream(
      const AudioParameters& params,
      const std::string& device_id,
      const LogCallback& log_callback) override;
  AudioInputStream* MakeAudioInputStream(
      const AudioParameters& params,
      const std::string& device_id,
      const LogCallback& log_callback) override;
  void ReleaseOutputStream(AudioOutputStream* stream) override;
  void ReleaseInputStream(AudioInputStream* stream) override;
  const char* GetName() override;

  // Implementation of AudioManagerBase.
  AudioOutputStream* MakeLinearOutputStream(
      const AudioParameters& params,
      const LogCallback& log_callback) override;
  AudioOutputStream* MakeLowLatencyOutputStream(
      const AudioParameters& params,
      const std::string& device_id,
      const LogCallback& log_callback) override;
  AudioOutputStream* MakeBitstreamOutputStream(
      const AudioParameters& params,
      const std::string& device_id,
      const LogCallback& log_callback) override;
  AudioInputStream* MakeLinearInputStream(
      const AudioParameters& params,
      const std::string& device_id,
      const LogCallback& log_callback) override;
  AudioInputStream* MakeLowLatencyInputStream(
      const AudioParameters& params,
      const std::string& device_id,
      const LogCallback& log_callback) override;

#if defined(TIZEN_PEPPER_EXTENSIONS)
  AudioOutputStream* MakeLowLatencyOutputStreamPPAPI(
      const AudioParameters& params,
      const std::string& device_id,
      const LogCallback& log_callback) override;
#endif

 protected:
  void ShutdownOnAudioThread() override;
  AudioParameters GetPreferredOutputStreamParameters(
      const std::string& output_device_id,
      const AudioParameters& input_params) override;

 private:
  void GetAudioDeviceNames(bool input,
                           media::AudioDeviceNames* device_names) const;

  // Called by MakeLinearOutputStream and MakeLowLatencyOutputStream.
  AudioOutputStream* MakeOutputStream(const AudioParameters& params,
                                      const std::string& device_id);

  // Called by MakeLinearInputStream and MakeLowLatencyInputStream.
  AudioInputStream* MakeInputStream(const AudioParameters& params,
                                    const std::string& device_id);

  int native_input_sample_rate_;

  DISALLOW_COPY_AND_ASSIGN(AudioManagerCapi);
};

}  // namespace media

#endif  // MEDIA_AUDIO_TIZEN_AUDIO_MANAGER_CAPI_H_
