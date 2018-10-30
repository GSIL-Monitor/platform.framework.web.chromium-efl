// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_SMPLAYER_ADAPTER_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_SMPLAYER_ADAPTER_H_

#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_player_adapter_base.h"

#include "base/single_thread_task_runner.h"
#include "base/synchronization/condition_variable.h"
#include "base/synchronization/lock.h"

class ISmPlayer;

namespace content {

class PepperSMPlayerAdapter : public PepperPlayerAdapterBase {
 public:
  PepperSMPlayerAdapter();
  ~PepperSMPlayerAdapter() override;

  // Player preparing
  int32_t Initialize() override;
  int32_t Reset() override;
  int32_t PrepareES() override;
  int32_t PrepareURL(const std::string& url) override;
  int32_t Unprepare() override;

  // Player controllers
  int32_t Play() override;
  int32_t Pause() override;
  int32_t Seek(PP_TimeTicks time,
               const base::Closure& seek_completed_cb) override;
  void SeekOnAppendThread(PP_TimeTicks time) override;
  int32_t SelectTrack(PP_ElementaryStream_Type_Samsung,
                      uint32_t track_index) override;
  int32_t SetPlaybackRate(float rate) override;
  int32_t SubmitEOSPacket(PP_ElementaryStream_Type_Samsung track_type) override;
  int32_t SubmitPacket(const ppapi::ESPacket* packet,
                       PP_ElementaryStream_Type_Samsung track_type) override;
  int32_t SubmitEncryptedPacket(
      const ppapi::ESPacket* packet,
      ScopedHandleAndSize handle,
      PP_ElementaryStream_Type_Samsung track_type) override;
  void Stop(const base::Callback<void(int32_t)>&) override;

  // Player setters
  int32_t SetApplicationID(const std::string& application_id) override;
  int32_t SetAudioStreamInfo(const PepperAudioStreamInfo& stream_info) override;
  int32_t SetDisplay(void* display, bool is_windowless) override;
  int32_t SetDisplayRect(const PP_Rect& display_rect,
                         const PP_FloatRect& crop_ratio_rect) override;
  int32_t SetDisplayMode(PP_MediaPlayerDisplayMode display_mode) override;
  int32_t SetDuration(PP_TimeDelta duration) override;
  int32_t SetExternalSubtitlesPath(const std::string& file_path,
                                   const std::string& encoding) override;
  int32_t SetStreamingProperty(PP_StreamingProperty property,
                               const std::string& data) override;
  int32_t SetSubtitlesDelay(PP_TimeDelta delay) override;
  int32_t SetVideoStreamInfo(const PepperVideoStreamInfo& stream_info) override;

  /* Supported only in MMPlayer */
  int32_t SetVr360Mode(PP_MediaPlayerVr360Mode) override;
  int32_t SetVr360Rotation(float, float) override;
  int32_t SetVr360ZoomLevel(uint32_t) override;

  // Player getters
  int32_t GetAudioTracksList(
      std::vector<PP_AudioTrackInfo>* track_list) override;
  int32_t GetAvailableBitrates(std::string* bitrates) override;
  int32_t GetCurrentTime(PP_TimeTicks* time) override;
  int32_t GetCurrentTrack(PP_ElementaryStream_Type_Samsung stream_type,
                          int32_t* track_index) override;
  int32_t GetDuration(PP_TimeDelta* duration) override;
  int32_t GetPlayerState(PP_MediaPlayerState* state) override;
  int32_t GetStreamingProperty(PP_StreamingProperty property,
                               std::string* property_value) override;
  int32_t GetTextTracksList(std::vector<PP_TextTrackInfo>* track_list) override;
  int32_t GetVideoTracksList(
      std::vector<PP_VideoTrackInfo>* track_list) override;

  bool IsDisplayModeSupported(PP_MediaPlayerDisplayMode display_mode) override;

  // Callback registering
  int32_t SetErrorCallback(
      const base::Callback<void(PP_MediaPlayerError)>& callback) override;

 private:
  struct SmPlayerDeleter {
    void operator()(ISmPlayer* player);
  };

  enum class InitializationState {
    kNotInitialized,
    kSuccess,
    kFailure,
  };

  // SMPlayerAdapter can support synchronous Stop; overriden async Stop will
  // just call this.
  int32_t Stop();

  void RegisterMediaCallbacks(PP_ElementaryStream_Type_Samsung type) override;

  void RegisterToBufferingEvents(bool should_register) override;
  void RegisterToMediaEvents(bool should_register) override;
  void RegisterToSubtitleEvents(bool should_register) override;

  int32_t ValidateSubtitleEncoding(const std::string& encoding);
  int32_t SetSubtitlesEncoding(const std::string& encoding);

  void OnError(PP_MediaPlayerError error, const char* msg);
  void OnInitComplete();
  void OnInitFailed();
  void OnSeekCompleted();
  void OnSeekStartedBuffering();
  void OnShowSubtitle(void* param);

  void OnMessage(int msg_id, void* param);
  static int32_t CallOnMessage(int msg_id, void* param, void* user_data);

  static void OnAudioEnoughData(void* user_data);
  static void OnAudioNeedData(unsigned bytes, void* user_data);
  // NOLINTNEXTLINE(runtime/int)
  static bool OnAudioSeekData(unsigned long long offset, void* user_data);
  static void OnVideoEnoughData(void* user_data);
  // NOLINTNEXTLINE(runtime/int)
  static void OnVideoNeedData(unsigned bytes, void* user_data);
  // NOLINTNEXTLINE(runtime/int)
  static bool OnVideoSeekData(unsigned long long offset, void* user_data);

  void SendBufferingEvents();

  using ISmPlayerPtr = std::unique_ptr<ISmPlayer, SmPlayerDeleter>;
  ISmPlayerPtr player_;

  base::Lock initialization_lock_;
  base::ConditionVariable initialization_condition_;
  InitializationState initialization_state_;
  bool seek_in_progres_;
  PP_TimeTicks seek_time_;

  bool is_playing_;
  bool external_subs_attached_;
  PP_MediaPlayerState state_;
  scoped_refptr<base::SingleThreadTaskRunner> initialization_cb_runner_;
  base::WeakPtrFactory<PepperSMPlayerAdapter> weak_ptr_factory_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_SMPLAYER_ADAPTER_H_
