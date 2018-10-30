// Copyright (c) 2014 The Samsung Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/tizen/capi_audio_input.h"

#include <audio_io.h>
#include <sys/types.h>

#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "media/audio/tizen/audio_manager_capi.h"

namespace media {

// Number of blocks of buffers used in the |fifo_|.
const int kNumberOfBlocksBufferInFifo = 2;

CapiAudioInputStream::CapiAudioInputStream(AudioManagerCapi* audio_manager,
                                           const std::string& device_name,
                                           const AudioParameters& params)
    : audio_manager_(audio_manager),
      callback_(NULL),
      device_name_(device_name),
      params_(params),
      volume_(1.0f),
      state_(media::kIsClosed),
      audio_worker_("AudioInput"),
      device_(NULL),
#if defined(OS_TIZEN_TV_PRODUCT)
      device_list_(NULL),
      sound_device_(NULL),
      stream_info_(NULL),
      device_added_for_stream_routing_(false),
#endif
      fifo_(params.channels(),
            params.frames_per_buffer(),
            kNumberOfBlocksBufferInFifo) {
}

CapiAudioInputStream::~CapiAudioInputStream() {
  DCHECK(!device_);
}

// static
void CapiAudioInputStream::AudioStreamReadCB(audio_in_h handle,
                                             size_t nbytes,
                                             void* user_data) {
  CapiAudioInputStream* is = static_cast<CapiAudioInputStream*>(user_data);
  is->ReadAudioData();
}

bool CapiAudioInputStream::Open() {
  DCHECK(state_ == media::kIsClosed);

  int ret;

#if defined(OS_TIZEN_TV_PRODUCT)
  // TODO(Girish): Register stream focus changed callback and take action
  // accordingly.
  stream_info_ = NULL;
  ret = sound_manager_create_stream_information(
      SOUND_STREAM_TYPE_MEDIA_EXTERNAL_ONLY, NULL, NULL, &stream_info_);
  if (ret != SOUND_MANAGER_ERROR_NONE) {
    LOG(ERROR) << "sound_manager_create_stream_information failed. Err:" << ret;
    CloseInternal(false);
    return false;
  }

  ret = sound_manager_get_device_list(SOUND_DEVICE_IO_DIRECTION_IN_MASK,
                                      &device_list_);
  if (ret != SOUND_MANAGER_ERROR_NONE) {
    LOG(ERROR) << "sound_manager_get_device_list falied. Err:" << ret;
    CloseInternal(false);
    return false;
  }

  int device_id = std::stoi(device_name_, nullptr, 0);
  sound_device_h device;
  while (!sound_manager_get_next_device(device_list_, &device)) {
    int id;
    if (sound_manager_get_device_id(device, &id)) {
      LOG(ERROR) << "Failed to get device ID";
      continue;
    }
    if (device_id == id) {
      sound_device_ = device;
      break;
    }
  }

  if (sound_device_ == NULL) {
    LOG(ERROR) << "Sound device with the mentioned ID does not exist";
    CloseInternal(false);
    return false;
  }

  ret =
      sound_manager_add_device_for_stream_routing(stream_info_, sound_device_);
  if (ret != SOUND_MANAGER_ERROR_NONE) {
    LOG(ERROR) << "sound_manager_add_device_for_stream_routing failed. Err:"
               << ret;
    CloseInternal(false);
    return false;
  }

  device_added_for_stream_routing_ = true;
  ret = sound_manager_apply_stream_routing(stream_info_);
  if (ret != SOUND_MANAGER_ERROR_NONE) {
    LOG(ERROR) << "sound_manager_apply_stream_routing failed. Err:" << ret;
    CloseInternal(false);
    return false;
  }
#endif

  ret = audio_in_create(params_.sample_rate(), AUDIO_CHANNEL_STEREO,
                        AUDIO_SAMPLE_TYPE_S16_LE, &device_);
  if (ret != AUDIO_IO_ERROR_NONE) {
    LOG(ERROR) << "audio_in_create failed. Err:" << ret;
    CloseInternal(false);
    return false;
  }

#if defined(OS_TIZEN_TV_PRODUCT)
  ret = audio_in_set_sound_stream_info(device_, stream_info_);
  if (ret != SOUND_MANAGER_ERROR_NONE) {
    LOG(ERROR) << "audio_in_set_sound_stream_info failed. Err:" << ret;
    CloseInternal(false);
    return false;
  }
#endif

  state_ = media::kIsOpened;
  return true;
}

void CapiAudioInputStream::Start(AudioInputCallback* callback) {
  DCHECK(audio_manager_->GetTaskRunner()->BelongsToCurrentThread());
  DCHECK(device_);
  callback_ = callback;

  if (state_ != media::kIsOpened) {
    LOG(ERROR) << "Device is not opened";
    if (callback_)
      callback_->OnError();
    return;
  }

  int ret = audio_in_set_stream_cb(device_, &AudioStreamReadCB, this);
  if (ret != AUDIO_IO_ERROR_NONE) {
    LOG(ERROR) << "audio_in_set_stream_cb failed. Err:" << ret;
    if (callback_)
      callback_->OnError();
    return;
  }

  audio_worker_.Start();
  StartAgc();

  ret = audio_in_prepare(device_);
  if (ret != AUDIO_IO_ERROR_NONE) {
    LOG(ERROR) << "Cannot prepare audio input. Err:" << ret;
    if (callback_)
      callback_->OnError();
    return;
  }

  state_ = media::kIsStarted;
}

void CapiAudioInputStream::Stop() {
  DCHECK(device_);
  DCHECK(state_ == media::kIsStarted);

  if (AUDIO_IO_ERROR_NONE != audio_in_unprepare(device_))
    LOG(WARNING) << "Cannot unprepare audio input";

  if (AUDIO_IO_ERROR_NONE != audio_in_unset_stream_cb(device_))
    LOG(WARNING) << "Cannot unset audio input cb";

  StopAgc();
  callback_ = NULL;

  audio_worker_.Stop();

  fifo_.Clear();
  state_ = media::kIsStopped;
}

void CapiAudioInputStream::Close() {
  if (state_ != media::kIsStopped)
    CapiAudioInputStream::Stop();

  state_ = media::kIsClosed;
  CloseInternal(true);

  // Signal to the manager that we're closed and can be removed.
  // This should be the last call in the function as it deletes "this".
  audio_manager_->ReleaseInputStream(this);
}

void CapiAudioInputStream::CloseInternal(bool success) {
#if defined(OS_TIZEN_TV_PRODUCT)
  if (device_added_for_stream_routing_) {
    int ret = sound_manager_remove_device_for_stream_routing(stream_info_,
                                                             sound_device_);
    if (ret != SOUND_MANAGER_ERROR_NONE)
      LOG(ERROR)
          << "sound_manager_remove_device_for_stream_routing failed. Err:"
          << ret;

    device_added_for_stream_routing_ = false;
  }
  sound_device_ = NULL;

  if (device_list_) {
    int ret = sound_manager_free_device_list(device_list_);
    if (ret != SOUND_MANAGER_ERROR_NONE)
      LOG(ERROR) << "sound_manager_free_device_list failed. Err:" << ret;

    device_list_ = NULL;
  }

  if (stream_info_) {
    int ret = sound_manager_destroy_stream_information(stream_info_);
    if (ret != SOUND_MANAGER_ERROR_NONE)
      LOG(ERROR) << "sound_manager_destroy_stream_information failed. Err:"
                 << ret;

    stream_info_ = NULL;
  }
#endif

  if (device_) {
    int ret = audio_in_destroy(device_);
    if (ret != AUDIO_IO_ERROR_NONE)
      LOG(WARNING) << "audio_in_destroy failed. Err:" << ret;

    device_ = NULL;
  }

  if (!success && callback_)
    callback_->OnError();
}

double CapiAudioInputStream::GetMaxVolume() {
  return 1.0;
}

void CapiAudioInputStream::SetVolume(double volume) {
  double max_volume = GetMaxVolume();

  if (volume > max_volume) {
    volume = max_volume;
  } else if (volume < 0.0) {
    volume = 0.0;
  }
  volume_ = volume;
}

double CapiAudioInputStream::GetVolume() {
  return volume_;
}

bool CapiAudioInputStream::IsMuted() {
  return volume_ ? false : true;
}

void CapiAudioInputStream::OnAudioIOData(const AudioBus* audio_bus,
                                         double hardware_delay_seconds,
                                         double normalized_volume) {
  if (!callback_)
    return;

  callback_->OnData(audio_bus,
                    base::TimeTicks::Now() -
                        base::TimeDelta::FromSecondsD(hardware_delay_seconds),
                    normalized_volume);
}

void CapiAudioInputStream::ReadAudioData() {
  unsigned int bytes_read = 0;
  const void* loc_buff = NULL;

  if (AUDIO_IO_ERROR_NONE != audio_in_peek(device_, &loc_buff, &bytes_read)) {
    LOG(ERROR) << "audio_in_peek() failed";
    if (callback_)
      callback_->OnError();
    return;
  }

  int number_of_frames = bytes_read / params_.GetBytesPerFrame();
  if (number_of_frames > fifo_.GetUnfilledFrames()) {
    int increase_blocks_of_buffer =
        (number_of_frames - fifo_.GetUnfilledFrames()) /
            params_.frames_per_buffer() +
        1;
    fifo_.IncreaseCapacity(increase_blocks_of_buffer);
  }

  fifo_.Push(loc_buff, number_of_frames, params_.bits_per_sample() / 8);

  if (AUDIO_IO_ERROR_NONE != audio_in_drop(device_)) {
    LOG(ERROR) << "audio_in_drop() failed";
    if (callback_)
      callback_->OnError();
    return;
  }
  double normalized_volume = 0.0;
  GetAgcVolume(&normalized_volume);

  double hardware_delay_seconds =
      static_cast<double>(params_.GetBufferDuration().InSeconds() * 2) *
      params_.channels();

  while (fifo_.available_blocks()) {
    const AudioBus* audio_bus = fifo_.Consume();
    hardware_delay_seconds +=
        static_cast<double>(fifo_.GetAvailableFrames()) / params_.sample_rate();

    // To reduce latency run client CB from dedicated thread
    if (callback_)
      audio_worker_.message_loop()->task_runner()->PostTask(
          FROM_HERE,
          base::Bind(&CapiAudioInputStream::OnAudioIOData,
                     base::Unretained(this), audio_bus,
                     hardware_delay_seconds, normalized_volume));
  }
}

}  // namespace media
