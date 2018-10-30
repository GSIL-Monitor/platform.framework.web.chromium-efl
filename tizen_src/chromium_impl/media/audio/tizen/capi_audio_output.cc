// Copyright (c) 2014 The Samsung Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/tizen/capi_audio_output.h"

#include <audio_io.h>

#include "base/bind.h"
#include "base/single_thread_task_runner.h"
#include "base/time/default_tick_clock.h"
#include "media/audio/audio_manager_base.h"
#include "media/base/audio_parameters.h"

namespace media {

const int kBitsPerByte = 8;

// static
void CapiAudioOutputStream::AudioStreamWriteCB(audio_out_h handle,
                                               size_t nbytes,
                                               void* user_data) {
  CapiAudioOutputStream* os = static_cast<CapiAudioOutputStream*>(user_data);
  os->WriteAudioData(nbytes);
}

CapiAudioOutputStream::CapiAudioOutputStream(const AudioParameters& params,
                                             AudioManagerBase* manager)
    : params_(params),
      manager_(manager),
      audio_out_(NULL),
      volume_(1.0f),
      buffer_(NULL),
      state_(media::kIsClosed),
      source_callback_(NULL) {
  DCHECK(manager_->GetTaskRunner()->BelongsToCurrentThread());
  CHECK(params_.IsValid());

  audio_bus_ = AudioBus::Create(params_);
}

CapiAudioOutputStream::~CapiAudioOutputStream() {}

void CapiAudioOutputStream::HandleError() {
  if (source_callback_ == NULL)
    return;
  source_callback_->OnError();
}

bool CapiAudioOutputStream::Open() {
  DCHECK(manager_->GetTaskRunner()->BelongsToCurrentThread());
  DCHECK(state_ == media::kIsClosed);

  if (AUDIO_IO_ERROR_NONE !=
      audio_out_create_new(
          params_.sample_rate(), ToCapiAudioEnum(params_.channel_layout()),
          ToCapiBPSEnum(params_.bits_per_sample()), &audio_out_)) {
    LOG(ERROR) << "Fail to create audio output";
    return false;
  }

  buffer_ = (char*)malloc(sizeof(char) * params_.GetBytesPerBuffer());
  if (!buffer_) {
    LOG(ERROR) << "Memory allocation for |buffer_| Failed";
    return false;
  }

  state_ = media::kIsOpened;
  return true;
}

void CapiAudioOutputStream::Start(AudioSourceCallback* callback) {
  DCHECK(manager_->GetTaskRunner()->BelongsToCurrentThread());
  DCHECK(state_ == media::kIsOpened);
  DCHECK(callback);
  DCHECK(audio_out_);
  source_callback_ = callback;

  if (AUDIO_IO_ERROR_NONE !=
      audio_out_set_stream_cb(audio_out_, &AudioStreamWriteCB, this)) {
    LOG(ERROR) << "Fail to set audio output stream cb";
    HandleError();
    return;
  }

  if (AUDIO_IO_ERROR_NONE != audio_out_prepare(audio_out_)) {
    LOG(ERROR) << "Cannot prepare audio output";
    HandleError();
    return;
  }

  state_ = media::kIsStarted;
}

void CapiAudioOutputStream::Stop() {
  DCHECK(manager_->GetTaskRunner()->BelongsToCurrentThread());
  DCHECK(state_ == media::kIsStarted);

  if (AUDIO_IO_ERROR_NONE != audio_out_unprepare(audio_out_)) {
    LOG(WARNING) << "Cannot unprepare audio output";
  }

  if (AUDIO_IO_ERROR_NONE != audio_out_unset_stream_cb(audio_out_)) {
    LOG(WARNING) << "Cannot unset audio output cb";
  }

  state_ = media::kIsStopped;
  source_callback_ = NULL;
}

void CapiAudioOutputStream::Close() {
  DCHECK(manager_->GetTaskRunner()->BelongsToCurrentThread());

  if (state_ != media::kIsStopped)
    CapiAudioOutputStream::Stop();

  state_ = media::kIsClosed;
  audio_out_destroy(audio_out_);  // always success
  audio_out_ = NULL;
  free(buffer_);
  manager_->ReleaseOutputStream(this);
}

void CapiAudioOutputStream::SetVolume(double volume) {
  DCHECK(manager_->GetTaskRunner()->BelongsToCurrentThread());
  volume_ = static_cast<float>(volume);
}

void CapiAudioOutputStream::GetVolume(double* volume) {
  DCHECK(manager_->GetTaskRunner()->BelongsToCurrentThread());
  DCHECK(volume != NULL);
  *volume = volume_;
}

void CapiAudioOutputStream::WriteAudioData(size_t nbytes) {
  int bytes_remaining = nbytes;
  size_t bytes_to_fill = params_.GetBytesPerBuffer();

  while (bytes_remaining > 0) {
    int frames_filled = 0;
    if (source_callback_) {
      // FIXME: EWK_BRINGUP. Verify working.
      frames_filled = source_callback_->OnMoreData(
          base::TimeDelta(), base::TimeTicks::Now(), 0, audio_bus_.get());
    }

    memset(buffer_, 0, bytes_to_fill);

    // Zero any unfilled data so it plays back as silence.
    if (frames_filled < audio_bus_->frames()) {
      audio_bus_->ZeroFramesPartial(frames_filled,
                                    audio_bus_->frames() - frames_filled);
    }

    audio_bus_->Scale(volume_);
    audio_bus_->ToInterleaved(audio_bus_->frames(),
                              params_.bits_per_sample() / kBitsPerByte,
                              buffer_);

    int bytes_written = audio_out_write(audio_out_, buffer_, bytes_to_fill);
    if (bytes_written < 0) {
      if (source_callback_) {
        LOG(ERROR) << "FAILED: audio_out_write";
        source_callback_->OnError();
        continue;
      }
    }

    // value can be negative
    bytes_remaining -= bytes_to_fill;
  }
}

}  // namespace media
