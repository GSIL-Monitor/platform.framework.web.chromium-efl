// Copyright 2016 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/tizen/sound_focus_manager.h"

#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_task_runner_handle.h"
#include "browser/tizen_extensible_host.h"
#include "content/browser/media/tizen/browser_media_player_manager_efl.h"
#include "media/base/tizen/media_player_util_efl.h"
#include "tizen/system_info.h"

namespace {
template <typename C, typename E>
void RemoveFromQueue(C& c, const E& e) {
  c.erase(std::remove(c.begin(), c.end(), e), c.end());
}

template <typename C, typename E>
bool IsExistInQueue(C& c, const E& e) {
  return std::find(c.begin(), c.end(), e) != c.end();
}

bool IsSoundFocusFeatureEnabled() {
  if (IsMobileProfile() || IsWearableProfile())
    return true;

  return false;
}

}  // namespace

namespace media {

static bool ShouldBePlayingForSoundFocusAcquire(
    sound_stream_focus_change_reason_e reason_for_change) {
  switch (reason_for_change) {
    // Add event list here to resume play.
    case SOUND_STREAM_FOCUS_CHANGED_BY_RINGTONE:
    case SOUND_STREAM_FOCUS_CHANGED_BY_VOIP:
    case SOUND_STREAM_FOCUS_CHANGED_BY_CALL:
    case SOUND_STREAM_FOCUS_CHANGED_BY_ALARM:
    case SOUND_STREAM_FOCUS_CHANGED_BY_NOTIFICATION:
      return true;
    default:
      return false;
  }

  return false;
}

// TODO: Can remove this method if we don't intend to ignore any case.
static bool CanIgnoreThisFocusChanging(
    sound_stream_focus_change_reason_e reason_for_change) {
  return false;
}

static bool GetSoundMode() {
  return TizenExtensibleHost::GetInstance()->GetExtensibleAPI("sound,mode");
}

SoundFocusManager* SoundFocusManager::GetInstance() {
  return base::Singleton<SoundFocusManager>::get();
}

SoundFocusManager::SoundFocusManager()
    : stream_info_(NULL),
      focus_state_watch_cb_id_(-1),
      device_changed_cb_id_(-1),
      task_runner_(base::ThreadTaskRunnerHandle::Get()),
      weak_factory_(this) {
  extra_info_ = base::StringPrintf("web_media-%d", getpid());
  InitializeStreamInfo();
}

SoundFocusManager::~SoundFocusManager() {
  weak_factory_.InvalidateWeakPtrs();
  sound_focus_clients_.clear();
  ReleaseSoundFocus();
  DestroyStreamInfo();
}

void SoundFocusManager::SoundFocusChangedCb(
    sound_stream_info_h stream_h,
    sound_stream_focus_mask_e /*focus_mask*/,
    sound_stream_focus_state_e focus_state,
    sound_stream_focus_change_reason_e reason_for_change,
    int sound_behavior,
    const char* additional_info,
    void* user_data) {
  SoundFocusManager* soundFocusManager =
      static_cast<SoundFocusManager*>(user_data);
  if (!soundFocusManager ||
      soundFocusManager->extra_info_.compare(additional_info) == 0)
    return;

  if (stream_h == NULL || stream_h != soundFocusManager->stream_info_) {
    LOG(ERROR) << "[SFM] -  stream info is null or not matched";
    return;
  }

  soundFocusManager->task_runner_->PostTask(
      FROM_HERE, base::Bind(&SoundFocusManager::SoundFocusChanged,
                            soundFocusManager->weak_factory_.GetWeakPtr(),
                            stream_h, focus_state, reason_for_change,
                            !(sound_behavior & SOUND_BEHAVIOR_NO_RESUME)));
}

void SoundFocusManager::SoundFocusWatchCb(
    int /*id*/,
    sound_stream_focus_mask_e /*focus_mask*/,
    sound_stream_focus_state_e focus_state,
    sound_stream_focus_change_reason_e reason_for_change,
    const char* additional_info,
    void* user_data) {
  if (GetSoundMode() == true) {
    return;
  }

  SoundFocusManager* soundFocusManager =
      static_cast<SoundFocusManager*>(user_data);
  if (!soundFocusManager ||
      soundFocusManager->extra_info_.compare(additional_info) == 0)
    return;

  soundFocusManager->task_runner_->PostTask(
      FROM_HERE, base::Bind(&SoundFocusManager::SoundFocusWatch,
                            soundFocusManager->weak_factory_.GetWeakPtr(),
                            focus_state, reason_for_change));
}

void SoundFocusManager::DeviceChangedCb(sound_device_h device,
                                        bool is_connected,
                                        void* user_data) {
  SoundFocusManager* soundFocusManager =
      static_cast<SoundFocusManager*>(user_data);
  if (!soundFocusManager || is_connected)
    return;

  soundFocusManager->task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&SoundFocusManager::SoundFocusReleased,
                 soundFocusManager->weak_factory_.GetWeakPtr(), false));
}

void SoundFocusManager::SoundFocusChanged(
    sound_stream_info_h stream_h,
    sound_stream_focus_state_e focus_state,
    sound_stream_focus_change_reason_e reason_for_change,
    bool should_resume) {
  LOG(INFO) << "[SFM] " << GetString(reason_for_change);

  // In this callback, point of view is "web_media".
  // Point of view is different with |SoundFocusWatch|.
  // So "web_media" focus is released , pause all media
  // and when it acquired focus, resume paused media.
  if (focus_state == SOUND_STREAM_FOCUS_STATE_RELEASED) {
    LOG(INFO) << "[SFM] - SOUND_STREAM_FOCUS_STATE_RELEASED";

    //
    // Audio Policy
    //
    // For focus releasing during play :
    // 1. by alarm or notification during play, nothing will happen.
    // 2. by media player for web media, nothing will happen.
    // 3. by ringtone, voip, or call during play, all media players for
    //    web_media which are on playing will be paused. They will be
    //    resumed to play when it reacquires the focus.
    // 4. by all others, all media players for web_meida on playing will be
    //    paused. They will NOT be resumed to play even after it reacquires
    //    the focus.
    //

    if (CanIgnoreThisFocusChanging(reason_for_change)) {
      LOG(INFO) << "[SFM] - Ignore this change : "
                << GetString(reason_for_change);
      return;
    }

    if (should_resume) {
      LOG(INFO) << "[SFM] - sound_manager_set_focus_reacquisition : true for "
                << GetString(reason_for_change);
      sound_manager_set_focus_reacquisition(stream_h, true);
      SoundFocusReleased(true);
    } else {
      LOG(INFO) << "[SFM] - sound_manager_set_focus_reacquisition : false for "
                << GetString(reason_for_change);
      sound_manager_set_focus_reacquisition(stream_h, false);
      SoundFocusReleased(false);
    }
  } else {
    LOG(INFO) << "[SFM] - SOUND_STREAM_FOCUS_STATE_ACQUIRED";

    if (should_resume) {
      SoundFocusAcquired();
    } else {
      LOG(INFO) << "[SFM] - No need resume playing for "
                << GetString(reason_for_change);
    }
  }
}

void SoundFocusManager::SoundFocusWatch(
    sound_stream_focus_state_e focus_state,
    sound_stream_focus_change_reason_e reason_for_change) {
  LOG(INFO) << "[SFM] " << GetString(reason_for_change);

  // In this callback, point of view is other app, not "web_media".
  // Point of view is different with |SoundFocusChanged|.
  // So when other app acquired focus, all media in "web_media" should be
  // paused.
  if (focus_state == SOUND_STREAM_FOCUS_STATE_ACQUIRED) {
    LOG(INFO) << "[SFM] - SOUND_STREAM_FOCUS_STATE_ACQUIRED";

    if (CanIgnoreThisFocusChanging(reason_for_change)) {
      LOG(INFO) << "[SFM] - Ignore this change "
                << GetString(reason_for_change);
      return;
    }

    SoundFocusReleased(ShouldBePlayingForSoundFocusAcquire(reason_for_change));
  } else {
    LOG(INFO) << "[SFM] - SOUND_STREAM_FOCUS_STATE_RELEASED";

    if (ShouldBePlayingForSoundFocusAcquire(reason_for_change)) {
      SoundFocusAcquired();
    } else {
      LOG(INFO) << "[SFM] - No need resume playing for "
                << GetString(reason_for_change);
    }
  }
}

void SoundFocusManager::InitializeStreamInfo() {
  int error = sound_manager_create_stream_information(
      SOUND_STREAM_TYPE_MEDIA, SoundFocusManager::SoundFocusChangedCb, this,
      &stream_info_);
  if (error != SOUND_MANAGER_ERROR_NONE) {
    LOG(ERROR) << "[SFM] - create_stream_information fail : " << error;
    return;
  }

  sound_stream_focus_mask_e focus_mask = SOUND_STREAM_FOCUS_FOR_PLAYBACK;
  error = sound_manager_add_focus_state_watch_cb(
      focus_mask, SoundFocusManager::SoundFocusWatchCb, this,
      &focus_state_watch_cb_id_);
  if (error != SOUND_MANAGER_ERROR_NONE) {
    LOG(ERROR) << "[SFM] - add_focus_state_watch_cb : " << error;
    return;
  }
  error = sound_manager_add_device_connection_changed_cb(
      SOUND_DEVICE_TYPE_EXTERNAL_MASK, SoundFocusManager::DeviceChangedCb, this,
      &device_changed_cb_id_);
  if (error != SOUND_MANAGER_ERROR_NONE) {
    LOG(ERROR) << "[SFM] - add_device_connection_changed_cb: " << error;
    return;
  }
}

void SoundFocusManager::DestroyStreamInfo() {
  if (!stream_info_)
    return;

  int error =
      sound_manager_remove_focus_state_watch_cb(focus_state_watch_cb_id_);
  if (error != SOUND_MANAGER_ERROR_NONE) {
    LOG(ERROR) << "[SFM] - remove_focus_state_watch_cb fail : " << error;
  }

  error = sound_manager_destroy_stream_information(stream_info_);
  if (error != SOUND_MANAGER_ERROR_NONE) {
    LOG(ERROR) << "[SFM] - destroy_stream_information fail :" << error;
  }

  error =
      sound_manager_remove_device_connection_changed_cb(device_changed_cb_id_);
  if (error != SOUND_MANAGER_ERROR_NONE) {
    LOG(ERROR) << "[SFM] - remove_device_connection_changed_cb : " << error;
  }
}

void SoundFocusManager::AcquireSoundFocus() {
  if (!stream_info_ || (GetSoundMode() == false) || IsWebMediaAcquiredFocus())
    return;

  // Note : Acquiring focus during call is restricted by Audio policy.
  // Call is most top priority stream type of sound manager.
  // If call |sound_manager_acquire_focus| during call, it retuns fail always
  // and never get focus changed callback.
  if (IsOnCall())
    return;

  int error = sound_manager_acquire_focus(
      stream_info_, SOUND_STREAM_FOCUS_FOR_PLAYBACK, SOUND_BEHAVIOR_NO_RESUME,
      extra_info_.c_str());
  if (error != SOUND_MANAGER_ERROR_NONE) {
    LOG(ERROR) << "[SFM] - sound_manager_acquire_focus fail : " << error;
  }

  sound_manager_set_focus_reacquisition(stream_info_, true);
}

void SoundFocusManager::ReleaseSoundFocus() {
  if (!stream_info_ || (GetSoundMode() == false) || !IsWebMediaAcquiredFocus())
    return;

  int error =
      sound_manager_release_focus(stream_info_, SOUND_STREAM_FOCUS_FOR_PLAYBACK,
                                  SOUND_BEHAVIOR_NONE, extra_info_.c_str());
  if (error != SOUND_MANAGER_ERROR_NONE) {
    LOG(ERROR) << "[SFM] - sound_manager_release_focus fail : " << error;
  }
}

void SoundFocusManager::SoundFocusAcquired() {
  for (auto client : sound_focus_clients_) {
    client->OnFocusAcquired();
  }
}

void SoundFocusManager::SoundFocusReleased(bool resume_playing) {
  for (auto client : sound_focus_clients_) {
    client->OnFocusReleased(resume_playing);
  }
}

bool SoundFocusManager::IsWebMediaAcquiredFocus() {
  bool acquired = false;
  sound_stream_focus_state_e state_for_playback;
  sound_stream_focus_state_e state_for_recording;

  int ret = sound_manager_get_focus_state(stream_info_, &state_for_playback,
                                          &state_for_recording);
  if (SOUND_MANAGER_ERROR_NONE == ret) {
    if (state_for_playback == SOUND_STREAM_FOCUS_STATE_ACQUIRED ||
        state_for_recording == SOUND_STREAM_FOCUS_STATE_ACQUIRED)
      acquired = true;
  }

  return acquired;
}

void SoundFocusManager::RegisterClient(SoundFocusClient* client) {
  if (!IsSoundFocusFeatureEnabled())
    return;

  LOG(INFO) << "[SFM] - Register Client : " << client;
  if (!IsExistInQueue(sound_focus_clients_, client))
    sound_focus_clients_.push_back(client);
  AcquireSoundFocus();
}

void SoundFocusManager::DeregisterClient(SoundFocusClient* client) {
  if (!IsSoundFocusFeatureEnabled())
    return;

  LOG(INFO) << "[SFM] - Deregister Client : " << client;
  RemoveFromQueue(sound_focus_clients_, client);

  if (IsWebMediaAcquiredFocus() && sound_focus_clients_.empty()) {
    ReleaseSoundFocus();
  }
}

bool SoundFocusManager::IsOnCall() {
  sound_stream_focus_change_reason_e acquiredBy;
  int flags = 0;
  char* additional_info = NULL;
  int ret = sound_manager_get_current_playback_focus(&acquiredBy, &flags,
                                                     &additional_info);
  if (SOUND_MANAGER_ERROR_NONE != ret) {
    LOG(INFO) << "sound_manager_get_current_playback_focus error : " << ret;
    return false;
  }

  free(additional_info);
  LOG(INFO) << "[SFM] Sound Focus acquired by : " << GetString(acquiredBy);
  switch (acquiredBy) {
    case SOUND_STREAM_FOCUS_CHANGED_BY_RINGTONE:
    case SOUND_STREAM_FOCUS_CHANGED_BY_VOIP:
    case SOUND_STREAM_FOCUS_CHANGED_BY_CALL:
      return true;
    default:
      return false;
  }
}

}  // namespace media
