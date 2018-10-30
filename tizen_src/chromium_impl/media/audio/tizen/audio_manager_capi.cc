// Copyright (c) 2014 The Samsung Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/tizen/audio_manager_capi.h"

#include "base/command_line.h"
#include "base/debug/stack_trace.h"
#include "base/environment.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/nix/xdg_util.h"
#include "base/stl_util.h"
#include "media/audio/pulse/pulse_output.h"
#include "media/audio/tizen/capi_audio_input.h"
#include "media/audio/tizen/capi_audio_output.h"
#include "media/audio/tizen/capi_util.h"
#include "media/base/audio_parameters.h"
#include "media/base/channel_layout.h"
#include "media/base/limits.h"

#if defined(TIZEN_PEPPER_EXTENSIONS) && defined(OS_TIZEN_TV_PRODUCT)
#include "media/audio/tizen/mm_player_audio_output.h"
#endif

namespace {

const int kMaxOutputStreams = 50;
const int kDefaultSampleRate = 44100;
const int kDefaultOutputBufferSize = 512;
const int kDefaultInputBufferSize = 1024;
const int kBitsPerSample = 16;

}  // namespace

namespace media {

std::unique_ptr<AudioManager> CreateAudioManager(
    std::unique_ptr<AudioThread> audio_thread,
    AudioLogFactory* audio_log_factory) {
  return std::make_unique<AudioManagerCapi>(std::move(audio_thread),
                                            audio_log_factory);
}

AudioManagerCapi::AudioManagerCapi(std::unique_ptr<AudioThread> audio_thread,
                                   AudioLogFactory* audio_log_factory)
    : AudioManagerBase(std::move(audio_thread), audio_log_factory),
      native_input_sample_rate_(kDefaultSampleRate) {
  SetMaxOutputStreamsAllowed(kMaxOutputStreams);
}

AudioManagerCapi::~AudioManagerCapi() {
  Shutdown();
}

bool AudioManagerCapi::HasAudioOutputDevices() {
  // FIXME: No proper implementation for GetAudioOutputDeviceNames() is present.
  return true;
}

bool AudioManagerCapi::HasAudioInputDevices() {
  AudioDeviceNames devices;
  GetAudioInputDeviceNames(&devices);
  return !devices.empty();
}

void AudioManagerCapi::GetAudioDeviceNames(
    bool input,
    media::AudioDeviceNames* device_names) const {
  DCHECK(device_names != NULL);
  DCHECK(device_names->empty());

#if defined(OS_TIZEN_TV_PRODUCT)
  int mask;
  if (input)
    mask = SOUND_DEVICE_IO_DIRECTION_IN_MASK;
  else
    mask = SOUND_DEVICE_IO_DIRECTION_OUT_MASK;

  sound_device_list_h list;
  int ret = sound_manager_get_device_list(mask, &list);
  if (ret != SOUND_MANAGER_ERROR_NONE) {
    LOG(ERROR) << "Failed to get device list. Err:" << ret
               << ". Creating only the default device";
    device_names->push_front(AudioDeviceName::CreateDefault());
    return;
  }

  sound_device_h device;
  while (!sound_manager_get_next_device(list, &device)) {
    int id;
    char* name;
    int get_status = sound_manager_get_device_id(device, &id);
    if (get_status != SOUND_MANAGER_ERROR_NONE) {
      LOG(ERROR) << "Failed to get device ID. Err:" << get_status;
      continue;
    }

    get_status = sound_manager_get_device_name(device, &name);
    if (get_status != SOUND_MANAGER_ERROR_NONE) {
      LOG(ERROR) << "Failed to get device name. Err:" << get_status;
      continue;
    }

    LOG(INFO) << "Device - ID:" << id << ". Name:" << name;
    device_names->push_front(AudioDeviceName(name, std::to_string(id)));
  }

  ret = sound_manager_free_device_list(list);
  if (ret != SOUND_MANAGER_ERROR_NONE)
    LOG(INFO) << "Failed to free device list. Err:" << ret;
#else
  device_names->push_front(AudioDeviceName::CreateDefault());
#endif
}

void AudioManagerCapi::GetAudioInputDeviceNames(
    AudioDeviceNames* device_names) {
  GetAudioDeviceNames(true, device_names);
}

void AudioManagerCapi::GetAudioOutputDeviceNames(
    AudioDeviceNames* device_names) {
  GetAudioDeviceNames(false, device_names);
}

AudioParameters AudioManagerCapi::GetInputStreamParameters(
    const std::string& device_id) {
  int buffer_size = media::GetAudioInputBufferSize(NULL);

  if (buffer_size <= 0)
    buffer_size = kDefaultInputBufferSize;

  return AudioParameters(AudioParameters::AUDIO_PCM_LOW_LATENCY,
                         CHANNEL_LAYOUT_STEREO, kDefaultSampleRate,
                         kBitsPerSample, buffer_size);
}

AudioOutputStream* AudioManagerCapi::MakeAudioOutputStream(
    const AudioParameters& params,
    const std::string& device_id,
    const LogCallback& log_callback) {
  DCHECK(GetTaskRunner()->BelongsToCurrentThread());
  return MakeOutputStream(params, device_id);
}

AudioInputStream* AudioManagerCapi::MakeAudioInputStream(
    const AudioParameters& params,
    const std::string& device_id,
    const LogCallback& log_callback) {
  DCHECK(GetTaskRunner()->BelongsToCurrentThread());
  return MakeInputStream(params, device_id);
}

void AudioManagerCapi::ReleaseOutputStream(AudioOutputStream* stream) {
  DCHECK(GetTaskRunner()->BelongsToCurrentThread());
  AudioManagerBase::ReleaseOutputStream(stream);
}

void AudioManagerCapi::ReleaseInputStream(AudioInputStream* stream) {
  DCHECK(GetTaskRunner()->BelongsToCurrentThread());
  AudioManagerBase::ReleaseInputStream(stream);
}

const char* AudioManagerCapi::GetName() {
  return "Tizen";
}

// FIXME: Use the audio format in all below APIs.
AudioOutputStream* AudioManagerCapi::MakeLinearOutputStream(
    const AudioParameters& params,
    const LogCallback& log_callback) {
  DCHECK(GetTaskRunner()->BelongsToCurrentThread());
  return MakeOutputStream(params, std::string());
}

AudioOutputStream* AudioManagerCapi::MakeLowLatencyOutputStream(
    const AudioParameters& params,
    const std::string& device_id,
    const LogCallback& log_callback) {
  DCHECK(GetTaskRunner()->BelongsToCurrentThread());
  return MakeOutputStream(params, device_id);
}

AudioOutputStream* AudioManagerCapi::MakeBitstreamOutputStream(
    const AudioParameters& params,
    const std::string& device_id,
    const LogCallback& log_callback) {
  DCHECK(GetTaskRunner()->BelongsToCurrentThread());
  return MakeOutputStream(params, device_id);
}

#if defined(TIZEN_PEPPER_EXTENSIONS)
AudioOutputStream* AudioManagerCapi::MakeLowLatencyOutputStreamPPAPI(
    const AudioParameters& params,
    const std::string& device_id,
    const LogCallback& log_callback) {
  DCHECK_EQ(AudioParameters::AUDIO_PCM_LOW_LATENCY_PPAPI, params.format());
  return MakeOutputStream(params, device_id);
}
#endif

AudioInputStream* AudioManagerCapi::MakeLinearInputStream(
    const AudioParameters& params,
    const std::string& device_id,
    const LogCallback& log_callback) {
  DCHECK(GetTaskRunner()->BelongsToCurrentThread());
  return MakeInputStream(params, device_id);
}

AudioInputStream* AudioManagerCapi::MakeLowLatencyInputStream(
    const AudioParameters& params,
    const std::string& device_id,
    const LogCallback& log_callback) {
  DCHECK(GetTaskRunner()->BelongsToCurrentThread());
  return MakeInputStream(params, device_id);
}

void AudioManagerCapi::ShutdownOnAudioThread() {
  AudioManagerBase::ShutdownOnAudioThread();
}

AudioParameters AudioManagerCapi::GetPreferredOutputStreamParameters(
    const std::string& output_device_id,
    const AudioParameters& input_params) {
  ChannelLayout channel_layout = CHANNEL_LAYOUT_STEREO;
  int frame_per_buffer = kDefaultOutputBufferSize;
  int bits_per_sample = kBitsPerSample;
  int sample_rate = kDefaultSampleRate;

  if (input_params.IsValid()) {
    bits_per_sample = input_params.bits_per_sample();
    channel_layout = input_params.channel_layout();
    frame_per_buffer = input_params.frames_per_buffer();
    sample_rate = input_params.sample_rate();
  } else {
    if (input_params.sample_rate() >= media::limits::kMinSampleRate &&
        input_params.sample_rate() <= media::limits::kMaxSampleRate) {
      LOG(WARNING) << "but sample_rate is OK. Use it for fail-safe routine";
      sample_rate = input_params.sample_rate();
    }
    if (input_params.channel_layout() > CHANNEL_LAYOUT_UNSUPPORTED &&
        input_params.channel_layout() < CHANNEL_LAYOUT_MAX) {
      LOG(WARNING) << "but channel_layout is OK. Use it for fail-safe routine";
      channel_layout = input_params.channel_layout();
    }
    if (input_params.bits_per_sample() > 0 &&
        input_params.bits_per_sample() <= media::limits::kMaxBitsPerSample) {
      LOG(WARNING) << "but bits_per_sample is OK. Use it for fail-safe routine";
      bits_per_sample = input_params.bits_per_sample();
    }

    sample_rate = media::GetAudioOutputSampleRate(
        NULL, sample_rate, channel_layout, bits_per_sample);
    if (!sample_rate)
      sample_rate = kDefaultSampleRate;
  }

  int capi_buffer_size = media::GetAudioOutputBufferSize(
      NULL, sample_rate, channel_layout, bits_per_sample);

  if (capi_buffer_size) {
    if (input_params.GetBytesPerFrame() != 0) {
      frame_per_buffer = capi_buffer_size / input_params.GetBytesPerFrame();
    } else {
      int channel_count = channel_layout == CHANNEL_LAYOUT_STEREO ? 2 : 1;
      frame_per_buffer =
          capi_buffer_size / (bits_per_sample / 8 * channel_count);
    }
  }

  if (channel_layout > CHANNEL_LAYOUT_STEREO) {
    // Tizen only support STEREO and MONO.
    channel_layout = CHANNEL_LAYOUT_STEREO;
    LOG(WARNING) << "channel_layout is greater than CHANNEL_LAYOUT_STEREO";
  }

  return AudioParameters(AudioParameters::AUDIO_PCM_LOW_LATENCY, channel_layout,
                         sample_rate, bits_per_sample, frame_per_buffer);
}

AudioOutputStream* AudioManagerCapi::MakeOutputStream(
    const AudioParameters& params,
    const std::string& input_device_id) {
#if defined(TIZEN_PEPPER_EXTENSIONS) && defined(OS_TIZEN_TV_PRODUCT)
  if (params.format() == AudioParameters::AUDIO_PCM_LOW_LATENCY_PPAPI)
    return new MMPlayerAudioOutputStream(params, this);
#endif
  return new CapiAudioOutputStream(params, this);
}

AudioInputStream* AudioManagerCapi::MakeInputStream(
    const AudioParameters& params,
    const std::string& device_id) {
  return new CapiAudioInputStream(this, device_id, params);
}

}  // namespace media
