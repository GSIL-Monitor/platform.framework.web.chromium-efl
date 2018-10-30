// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/tizen/mm_player_audio_output.h"

#include "base/bind.h"
#include "media/audio/audio_manager_base.h"

namespace media {

namespace {

media_format_mimetype_e BitsToMediaFormatMimetype(int bits_per_sample) {
  switch (bits_per_sample) {
    case 16:
      return MEDIA_FORMAT_PCM_S16LE;
    case 24:
      return MEDIA_FORMAT_PCM_S24LE;
    case 32:
      return MEDIA_FORMAT_PCM_S32LE;
    default:
      NOTREACHED() << "Unsupported bits per sample: " << bits_per_sample;
      return MEDIA_FORMAT_MAX;
  }
}

}  // namespace

MMPlayerAudioOutputStream::MMPlayerAudioOutputStream(
    const AudioParameters& params,
    AudioManagerBase* manager)
    : params_(params),
      manager_(manager),
      player_handle_(nullptr),
      volume_(1.0f),
      state_(kIsClosed),
      source_callback_(nullptr),
      push_packets_thread_("MMPlayerPushPacketsThread"),
      push_packets_(false) {
  DCHECK(manager_->GetTaskRunner()->BelongsToCurrentThread());
  CHECK(params_.IsValid());
  audio_bus_ = AudioBus::Create(params_);
}

MMPlayerAudioOutputStream::~MMPlayerAudioOutputStream() {}

void MMPlayerAudioOutputStream::HandleError() {
  if (source_callback_ == nullptr)
    return;
  source_callback_->OnError();
}

// static
void MMPlayerAudioOutputStream::OnBufferStatusCB(
    player_media_stream_buffer_status_e status,
    unsigned long long nbytes,  // NOLINT(runtime/int)
    void* user_data) {
  MMPlayerAudioOutputStream* os =
      static_cast<MMPlayerAudioOutputStream*>(user_data);
  os->OnBufferStatus(status, nbytes);
}

bool MMPlayerAudioOutputStream::Open() {
  DCHECK(manager_->GetTaskRunner()->BelongsToCurrentThread());
  DCHECK(state_ == kIsClosed);
  if (state_ != kIsClosed)
    return false;

  if (player_create(&player_handle_) != PLAYER_ERROR_NONE) {
    LOG(ERROR) << "player_create failed";
    return false;
  }

  buffer_.resize(params_.GetBytesPerBuffer());
  state_ = kIsOpened;
  return true;
}

void MMPlayerAudioOutputStream::Start(AudioSourceCallback* callback) {
  DCHECK(manager_->GetTaskRunner()->BelongsToCurrentThread());
  DCHECK(callback);
  DCHECK(player_handle_);

  source_callback_ = callback;
  push_packets_thread_.Start();

  if (state_ != kIsOpened && state_ != kIsStopped) {
    HandleError();
    return;
  }

  if (player_set_uri(player_handle_, "buff://PCM") != PLAYER_ERROR_NONE) {
    LOG(ERROR) << "player_set_uri failed";
    HandleError();
    return;
  }

  media_format_h audio_format = nullptr;
  if (media_format_create(&audio_format) != MEDIA_FORMAT_ERROR_NONE) {
    LOG(ERROR) << "media_format_create failed";
    HandleError();
    return;
  }

  DCHECK(audio_format);
  media_format_set_audio_mime(
      audio_format, BitsToMediaFormatMimetype(params_.bits_per_sample()));
  media_format_set_audio_channel(audio_format, params_.channels());
  media_format_set_audio_samplerate(audio_format, params_.sample_rate());

  if (player_set_media_stream_info(player_handle_, PLAYER_STREAM_TYPE_AUDIO,
                                   audio_format) != PLAYER_ERROR_NONE) {
    LOG(ERROR) << "player_set_media_stream_info failed";
    media_format_unref(audio_format);
    HandleError();
    return;
  }

  media_format_unref(audio_format);

  if (player_set_media_stream_buffer_max_size(
          player_handle_, PLAYER_STREAM_TYPE_AUDIO,
          params_.GetBytesPerBuffer()) != PLAYER_ERROR_NONE) {
    LOG(ERROR) << "player_set_media_stream_buffer_max_size failed";
    HandleError();
    return;
  }

  static const int kMinThresholdPercent = 50;
  if (player_set_media_stream_buffer_min_threshold(
          player_handle_, PLAYER_STREAM_TYPE_AUDIO, kMinThresholdPercent)) {
    LOG(ERROR) << "player_set_media_stream_buffer_min_threshold failed";
    HandleError();
    return;
  }

  if (player_set_media_stream_buffer_status_cb_ex(
          player_handle_, PLAYER_STREAM_TYPE_AUDIO,
          MMPlayerAudioOutputStream::OnBufferStatusCB,
          this) != PLAYER_ERROR_NONE) {
    LOG(ERROR) << "player_set_media_stream_buffer_status_cb_ex failed";
    return;
  }

  if (player_prepare(player_handle_) != PLAYER_ERROR_NONE) {
    LOG(ERROR) << "player_prepare failed";
    HandleError();
    return;
  }

  if (player_start(player_handle_) != PLAYER_ERROR_NONE) {
    LOG(ERROR) << "player_start failed";
    HandleError();
    return;
  }
  state_ = kIsStarted;
}

void MMPlayerAudioOutputStream::Stop() {
  DCHECK(manager_->GetTaskRunner()->BelongsToCurrentThread());
  DCHECK(state_ == kIsStarted);

  source_callback_ = nullptr;
  if (state_ != kIsStarted)
    return;

  if (player_stop(player_handle_) != PLAYER_ERROR_NONE) {
    LOG(ERROR) << "player_stop failed";
    return;
  }

  if (player_unset_media_stream_buffer_status_cb_ex(
          player_handle_, PLAYER_STREAM_TYPE_AUDIO) != PLAYER_ERROR_NONE) {
    LOG(WARNING) << "player_unset_media_stream_buffer_status_cb_ex failed";
    return;
  }

  push_packets_ = false;
  push_packets_thread_.Stop();

  if (player_unprepare(player_handle_) != PLAYER_ERROR_NONE) {
    LOG(WARNING) << "Cannot unprepare audio output";
    return;
  }

  state_ = kIsStopped;
}

void MMPlayerAudioOutputStream::Close() {
  DCHECK(manager_->GetTaskRunner()->BelongsToCurrentThread());

  if (state_ != kIsStopped)
    MMPlayerAudioOutputStream::Stop();

  state_ = kIsClosed;

  player_destroy(player_handle_);
  player_handle_ = nullptr;
  manager_->ReleaseOutputStream(this);
}

void MMPlayerAudioOutputStream::SetVolume(double volume) {
  DCHECK(manager_->GetTaskRunner()->BelongsToCurrentThread());
  volume_ = volume;
}

void MMPlayerAudioOutputStream::GetVolume(double* volume) {
  DCHECK(manager_->GetTaskRunner()->BelongsToCurrentThread());
  DCHECK(volume != nullptr);
  *volume = volume_;
}

void MMPlayerAudioOutputStream::OnBufferStatus(
    player_media_stream_buffer_status_e status,
    unsigned long long nbytes) {  // NOLINT(runtime/int)
  if (status == PLAYER_MEDIA_STREAM_BUFFER_UNDERRUN) {
    if (push_packets_ == false) {
      push_packets_ = true;
      push_packets_thread_.task_runner().get()->PostTask(
          FROM_HERE, base::Bind(&MMPlayerAudioOutputStream::PushPacketsRoutine,
                                base::Unretained(this)));
    }
  } else if (status == PLAYER_MEDIA_STREAM_BUFFER_OVERFLOW) {
    push_packets_ = false;
  }
}

void MMPlayerAudioOutputStream::PushPacketsRoutine() {
  size_t bytes_to_fill = params_.GetBytesPerBuffer();
  while (push_packets_) {
    int frames_filled = 0;
    if (source_callback_) {
      frames_filled = source_callback_->OnMoreData(
          base::TimeDelta(), base::TimeTicks::Now(), 0, audio_bus_.get());
    }

    memset(buffer_.data(), 0, bytes_to_fill);

    // Zero any unfilled data so it plays back as silence.
    if (frames_filled < audio_bus_->frames()) {
      audio_bus_->ZeroFramesPartial(frames_filled,
                                    audio_bus_->frames() - frames_filled);
    }

    audio_bus_->Scale(volume_);
    audio_bus_->ToInterleaved(audio_bus_->frames(),
                              params_.bits_per_sample() / 8, buffer_.data());

    int error_code = player_push_buffer(
        player_handle_, reinterpret_cast<unsigned char*>(buffer_.data()),
        bytes_to_fill);
    if (error_code != PLAYER_ERROR_NONE) {
      LOG(ERROR) << "player_push_buffer failed";
      HandleError();
    }
  }
}

}  // namespace media
