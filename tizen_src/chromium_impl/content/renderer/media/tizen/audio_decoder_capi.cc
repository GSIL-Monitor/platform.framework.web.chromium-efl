// Copyright 2015 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/audio_decoder.h"

#include <player_internal.h>

#include "base/bind.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread.h"
#include "third_party/WebKit/public/platform/WebAudioBus.h"

namespace content {

base::Lock shutdownLock;
bool platformIsTerminated = false;

void PlatformIsShutDown() {
  base::AutoLock auto_lock(shutdownLock);
  platformIsTerminated = true;
}

const int kChannelCount = 2;
const int kSampleRate = 44100;

struct DecodedSamples {
  typedef std::vector<float> Packet;
  typedef std::list<Packet> ChannelData;
  ChannelData data_[kChannelCount];

  void push(int channel, float* decoded_data, int decoded_size) {
    auto& channel_data = data_[channel];
    channel_data.emplace_back(decoded_data, decoded_data + decoded_size);
  }

  int get_size() const {
    auto& channel_data = data_[0];
    int size = 0;
    for (auto& packet : channel_data)
      size += packet.size();
    return size;
  }
};

class AudioDecoderIO {
 public:
  AudioDecoderIO(const char* data,
                 size_t data_size,
                 blink::WebAudioBus* destination_bus);
  virtual ~AudioDecoderIO();

  bool Start();
  void OnPlayerEos();
  void OnPlayerError(int error_code, char const* from);

  static void PlayerDecodedBufferReadyCb(player_audio_raw_data_s* audio_data,
                                         void* data);
  static void PlayerEosCb(void* data);
  static void PlayerErrorCb(int error_code, void* data);

  blink::WebAudioBus* destination_bus_;

 private:
  void DestroyPlayerHandle();

  // Memory that will hold the encoded audio data. This is
  // used by |MediaCodec| for decoding.
  const char* data_;
  size_t size_;
  player_h player_handle_;
  bool is_success_;
  bool is_prepared_;

  base::WaitableEvent event_;
  DecodedSamples decoded_samples_;
  base::Thread decoder_thread_;

  DISALLOW_COPY_AND_ASSIGN(AudioDecoderIO);
};

AudioDecoderIO::AudioDecoderIO(const char* data,
                               size_t data_size,
                               blink::WebAudioBus* destination_bus)
    : destination_bus_(destination_bus),
      data_(data),
      size_(data_size),
      player_handle_(nullptr),
      is_success_(false),
      is_prepared_(false),
      event_(base::WaitableEvent::ResetPolicy::MANUAL,
             base::WaitableEvent::InitialState::NOT_SIGNALED),
      decoder_thread_("CAPIAudioDecoder") {}

AudioDecoderIO::~AudioDecoderIO() {
  DestroyPlayerHandle();
  if (decoder_thread_.IsRunning())
    decoder_thread_.Stop();
}

void AudioDecoderIO::DestroyPlayerHandle() {
  if (player_handle_) {
    player_unset_completed_cb(player_handle_);
    player_unset_error_cb(player_handle_);
    if (is_prepared_) {
      player_unprepare(player_handle_);
      is_prepared_ = false;
    }
    player_destroy(player_handle_);
    player_handle_ = nullptr;
  }
}

void AudioDecoderIO::PlayerDecodedBufferReadyCb(
    player_audio_raw_data_s* audio_data,
    void* data) {
  float* src_data = static_cast<float*>(audio_data->data);
  int src_size = audio_data->size / sizeof(float);
  if (src_size <= 0 || !src_data) {
    LOG(ERROR) << "Decoded buffer (size=" << audio_data->size << ") is invalid";
    return;
  }

  AudioDecoderIO* self = static_cast<AudioDecoderIO*>(data);
  if (!self)
    return;

  int channel = audio_data->channel_mask - 1;
  self->decoded_samples_.push(channel, src_data, src_size);
}

void AudioDecoderIO::PlayerEosCb(void* data) {
  AudioDecoderIO* self = static_cast<AudioDecoderIO*>(data);
  if (!self)
    return;
  self->decoder_thread_.task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&AudioDecoderIO::OnPlayerEos, base::Unretained(self)));
}

void AudioDecoderIO::PlayerErrorCb(int error_code, void* data) {
  AudioDecoderIO* self = static_cast<AudioDecoderIO*>(data);
  if (!self)
    return;

  self->decoder_thread_.task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&AudioDecoderIO::OnPlayerError, base::Unretained(self),
                 error_code, "PlayerErrorCb"));
}

bool AudioDecoderIO::Start() {
  CHECK(player_handle_ == NULL);
  decoder_thread_.Start();
  int error = player_create(&player_handle_);
  if (error != PLAYER_ERROR_NONE) {
    OnPlayerError(error, "player_create()");
    return false;
  }

  player_set_completed_cb(player_handle_, &AudioDecoderIO::PlayerEosCb, this);
  player_set_error_cb(player_handle_, &AudioDecoderIO::PlayerErrorCb, this);
  player_set_pcm_extraction_mode(
      player_handle_, false, &AudioDecoderIO::PlayerDecodedBufferReadyCb, this);
  player_set_pcm_spec(player_handle_, "F32LE", kSampleRate, kChannelCount);

  error = player_set_memory_buffer(player_handle_, data_, size_);
  if (error != PLAYER_ERROR_NONE) {
    DestroyPlayerHandle();
    OnPlayerError(error, "player_set_memory_buffer()");
    return false;
  }

  error = player_prepare(player_handle_);
  if (error != PLAYER_ERROR_NONE) {
    DestroyPlayerHandle();
    OnPlayerError(error, "player_prepare()");
    return false;
  }
  is_prepared_ = true;

  error = player_start(player_handle_);
  if (error != PLAYER_ERROR_NONE) {
    DestroyPlayerHandle();
    OnPlayerError(error, "player_start()");
    return false;
  }
  event_.Wait();
  return is_success_;
}

void AudioDecoderIO::OnPlayerEos() {
  size_t number_of_frames = decoded_samples_.get_size();
  if (number_of_frames == 0) {
    event_.Signal();
    return;
  }

  {
    base::AutoLock auto_lock(shutdownLock);
    if (platformIsTerminated) {
      event_.Signal();
      return;
    }
  }

  destination_bus_->Initialize(kChannelCount, number_of_frames, kSampleRate);

  for (size_t channel = 0; channel < kChannelCount; channel++) {
    auto& channel_data = decoded_samples_.data_[channel];
    float* destination = destination_bus_->ChannelData(channel);
    for (auto& packet : channel_data) {
      memcpy(destination, packet.data(), packet.size() * sizeof(float));
      destination += packet.size();
    }
    channel_data.clear();
  }
  LOG(INFO) << " number_of_frames : " << number_of_frames;
  is_success_ = true;
  event_.Signal();
}

void AudioDecoderIO::OnPlayerError(int err, char const* from) {
  LOG(ERROR) << "Stoping decoding due to error " << err << " from " << from;
  event_.Signal();
}

// Decode in-memory audio file data.
bool DecodeAudioFileData(blink::WebAudioBus* destination_bus,
                         const char* data,
                         size_t data_size) {
  {
    base::AutoLock auto_lock(shutdownLock);
    if (platformIsTerminated)
      return false;
  }

  AudioDecoderIO audio_decoder(data, data_size, destination_bus);
  if (!audio_decoder.Start())
    return false;

  return true;
}

}  // namespace content
