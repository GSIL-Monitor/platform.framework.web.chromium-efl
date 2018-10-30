// Copyright 2016 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_TIZEN_SOUND_FOCUS_MANAGER_H_
#define MEDIA_BASE_TIZEN_SOUND_FOCUS_MANAGER_H_

#include <sound_manager.h>
#include <deque>

#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "media/base/media_export.h"

namespace media {

class MEDIA_EXPORT SoundFocusClient {
 public:
  virtual void OnFocusAcquired() = 0;
  virtual void OnFocusReleased(bool resume_playing) = 0;

 protected:
  virtual ~SoundFocusClient(){};
};

class MEDIA_EXPORT SoundFocusManager {
 public:
  static SoundFocusManager* GetInstance();

  void RegisterClient(SoundFocusClient* client);
  void DeregisterClient(SoundFocusClient* client);
  bool IsOnCall();

  const sound_stream_info_h GetSoundStreamInfo() { return stream_info_; }

 private:
  friend struct base::DefaultSingletonTraits<SoundFocusManager>;
  SoundFocusManager();
  ~SoundFocusManager();

  // callback functions.
  static void SoundFocusChangedCb(
      sound_stream_info_h stream_h,
      sound_stream_focus_mask_e focus_mask,
      sound_stream_focus_state_e focus_state,
      sound_stream_focus_change_reason_e reason_for_change,
      int sound_behavior,
      const char* additional_info,
      void* user_data);
  static void SoundFocusWatchCb(
      int id,
      sound_stream_focus_mask_e focus_mask,
      sound_stream_focus_state_e focus_state,
      sound_stream_focus_change_reason_e reason_for_change,
      const char* extra_info,
      void* user_data);
  static void DeviceChangedCb(sound_device_h device,
                              bool is_connected,
                              void* user_data);

  void SoundFocusChanged(sound_stream_info_h stream_h,
                         sound_stream_focus_state_e focus_state,
                         sound_stream_focus_change_reason_e reason_for_change,
                         bool should_resume);
  void SoundFocusWatch(sound_stream_focus_state_e focus_state,
                       sound_stream_focus_change_reason_e reason_for_change);

  void InitializeStreamInfo();
  void DestroyStreamInfo();
  void AcquireSoundFocus();
  void ReleaseSoundFocus();
  void SoundFocusAcquired();
  void SoundFocusReleased(bool resume_playing);
  bool IsWebMediaAcquiredFocus();

  sound_stream_info_h stream_info_;
  int focus_state_watch_cb_id_;
  int device_changed_cb_id_;
  std::deque<SoundFocusClient*> sound_focus_clients_;
  const scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  // Information about acquired playback focus along with Process ID
  std::string extra_info_;

  // NOTE: Weak pointers must be invalidated before all other member variables.
  base::WeakPtrFactory<SoundFocusManager> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(SoundFocusManager);
};

}  // namespace media

#endif  // MEDIA_BASE_TIZEN_SOUND_FOCUS_MANAGER_H_
