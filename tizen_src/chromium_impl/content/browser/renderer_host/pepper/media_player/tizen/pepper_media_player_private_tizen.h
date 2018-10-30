// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_MEDIA_PLAYER_PRIVATE_TIZEN_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_MEDIA_PLAYER_PRIVATE_TIZEN_H_

#include <memory>
#include <string>
#include <vector>

#include <Evas.h>

#include "base/single_thread_task_runner.h"
#include "base/timer/timer.h"
#include "content/browser/renderer_host/pepper/media_player/pepper_media_data_source_private.h"
#include "content/browser/renderer_host/pepper/media_player/pepper_media_player_private.h"
#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_player_adapter_interface.h"
#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_tizen_drm_manager.h"
#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_tizen_player_dispatcher.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/samsung/pp_media_player_samsung.h"

namespace base {
class Location;
}  // namespace base

namespace content {

class PepperMediaDataSourcePrivate;

class PepperMediaPlayerPrivateTizen : public PepperMediaPlayerPrivate {
 public:
  static scoped_refptr<PepperMediaPlayerPrivateTizen> Create(
      std::unique_ptr<PepperPlayerAdapterInterface> player);

  // PepperMediaPlayerPrivate
  void SetBufferingListener(std::unique_ptr<BufferingListenerPrivate>) override;
  void SetDRMListener(std::unique_ptr<DRMListenerPrivate>) override;
  void SetMediaEventsListener(
      std::unique_ptr<MediaEventsListenerPrivate>) override;
  void SetSubtitleListener(std::unique_ptr<SubtitleListenerPrivate>) override;

  void AttachDataSource(const scoped_refptr<PepperMediaDataSourcePrivate>&,
                        const base::Callback<void(int32_t)>& callback) override;
  void DetachDataSource();

  void Play(const base::Callback<void(int32_t)>& callback) override;
  void Pause(const base::Callback<void(int32_t)>& callback) override;
  void Stop(const base::Callback<void(int32_t)>& callback) override;
  void Seek(PP_TimeTicks,
            const base::Callback<void(int32_t)>& callback) override;

  void SetPlaybackRate(double rate,
                       const base::Callback<void(int32_t)>& callback) override;
  void GetDuration(
      const base::Callback<void(int32_t, PP_TimeDelta)>& callback) override;
  void GetCurrentTime(
      const base::Callback<void(int32_t, PP_TimeTicks)>& callback) override;

  void GetPlayerState(const base::Callback<void(int32_t, PP_MediaPlayerState)>&
                          callback) override;

  void GetVideoTracksList(const base::Callback<
                          void(int32_t, const std::vector<PP_VideoTrackInfo>&)>&
                              callback) override;
  void GetCurrentVideoTrackInfo(
      const base::Callback<void(int32_t, const PP_VideoTrackInfo&)>& callback)
      override;

  void GetAudioTracksList(const base::Callback<
                          void(int32_t, const std::vector<PP_AudioTrackInfo>&)>&
                              callback) override;
  void GetCurrentAudioTrackInfo(
      const base::Callback<void(int32_t, const PP_AudioTrackInfo&)>& callback)
      override;

  void GetTextTracksList(
      const base::Callback<void(int32_t, const std::vector<PP_TextTrackInfo>&)>&
          callback) override;
  void GetCurrentTextTrackInfo(
      const base::Callback<void(int32_t, const PP_TextTrackInfo&)>& callback)
      override;

  void SelectTrack(PP_ElementaryStream_Type_Samsung,
                   uint32_t track_index,
                   const base::Callback<void(int32_t)>& callback) override;

  void AddExternalSubtitles(
      const std::string& file_path,
      const std::string& encoding,
      const base::Callback<void(int32_t, const PP_TextTrackInfo&)>& callback)
      override;
  void SetSubtitlesDelay(
      PP_TimeDelta delay,
      const base::Callback<void(int32_t)>& callback) override;

  void SetDisplayRect(const PP_Rect& display_rect,
                      const PP_FloatRect& crop_ratio_rect,
                      const base::Callback<void(int32_t)>& callback) override;
  void SetDisplayMode(PP_MediaPlayerDisplayMode display_mode,
                      const base::Callback<void(int32_t)>& callback) override;
  void SetVr360Mode(PP_MediaPlayerVr360Mode vr360_mode,
                    const base::Callback<void(int32_t)>& callback) override;
  void SetVr360Rotation(float horizontal_angle,
                        float vertical_angle,
                        const base::Callback<void(int32_t)>& callback) override;
  void SetVr360ZoomLevel(
      uint32_t,
      const base::Callback<void(int32_t)>& callback) override;

  void SetDRMSpecificData(
      PP_MediaPlayerDRMType,
      PP_MediaPlayerDRMOperation,
      const void*,
      size_t,
      const base::Callback<void(int32_t)>& callback) override;

  // Listeners
  void OnLicenseRequest(const std::vector<uint8_t>& request);

  void OnErrorEvent(PP_MediaPlayerError error_code);
  void OnSeekCompleted();
  void OnDRMEvent(int event_type, void* msg_data);

 private:
  friend class base::RefCountedThreadSafe<PepperMediaPlayerPrivateTizen>;
  explicit PepperMediaPlayerPrivateTizen(
      std::unique_ptr<PepperPlayerAdapterInterface> player);
  ~PepperMediaPlayerPrivateTizen() override;

  // Aborts all pending callbacks.
  void AbortCallbacks(int32_t reason);

  // Invalidates player's dispatcher when it can't be reused in further calls.
  // This can happen eg. when muse-server has crashed
  void InvalidatePlayer();

  void StartCurrentTimeNotifier();
  void StopCurrentTimeNotifier();

  // Executes and resets a given callback. Logs on_cb_empty_message if
  // callback is not set, or skips printing if it's nullptr.
  // Please note callbacks must only be executed on the main thread.
  void ExecuteCallback(int32_t result,
                       base::Callback<void(int32_t)>* callback,
                       const char* on_cb_empty_message = nullptr);
  // dispatcher call helpers

  // Returns false if there is no need to fail (i.e. data source is attached)
  // or runs a callback with an error and then returns true.
  template <typename Functor, typename... Args>
  bool FailIfNotAttached(const base::Location& from_here,
                         const Functor& callback,
                         const Args&... args) {
    if (data_source_)
      return false;

    base::MessageLoop::current()->task_runner()->PostTask(
        from_here,
        base::Bind(callback, static_cast<int32_t>(PP_ERROR_FAILED), args...));
    return true;
  }

  // Player operations

  // UpdateDisplayRect variant meant to be called as a result of PPAPI
  // operations.
  void UpdateDisplayRect(const base::Callback<void(int32_t)>&);
  // UpdateDisplayRect  variant meant to be called by a platform player.
  void UpdateDisplayRect();

  void OnDataSourceAttached(int32_t result);

  void SeekOnPlayerThread(PP_TimeTicks, PepperPlayerAdapterInterface*);
  void SeekOnAppendThread(PP_TimeTicks, PepperPlayerAdapterInterface*);

  void GetAudioTracksListOnPlayerThread(
      const base::Callback<void(int32_t,
                                const std::vector<PP_AudioTrackInfo>&)>&,
      PepperPlayerAdapterInterface*);
  void GetCurrentAudioTrackInfoOnPlayerThread(
      const base::Callback<void(int32_t, const PP_AudioTrackInfo&)>&,
      PepperPlayerAdapterInterface*);
  void GetTextTracksListOnPlayerThread(
      const base::Callback<void(int32_t,
                                const std::vector<PP_TextTrackInfo>&)>&,
      PepperPlayerAdapterInterface*);
  void GetCurrentTextTrackInfoOnPlayerThread(
      const base::Callback<void(int32_t, const PP_TextTrackInfo&)>&,
      PepperPlayerAdapterInterface*);
  void InitializeDisplayOnPlayerThread(PepperPlayerAdapterInterface* player);
  void SetDisplayCropRatioOnPlayerThread(
      const PP_FloatRect&,
      const base::Callback<void(int32_t)>& callback,
      PepperPlayerAdapterInterface*);

  int32_t SetExternalSubtitlePathOnPlayerThread(const std::string& path,
                                                const std::string& encoding,
                                                PepperPlayerAdapterInterface*,
                                                PP_TextTrackInfo*);

  void OnTimeNeedsUpdate();

  void OnStoppedReply(int32_t result);

  std::unique_ptr<PepperTizenPlayerDispatcher> dispatcher_;
  scoped_refptr<PepperMediaDataSourcePrivate> data_source_;

  std::unique_ptr<DRMListenerPrivate> drm_listener_;

  base::RepeatingTimer time_update_timer_;

  base::Callback<void(int32_t)> attach_callback_;
  base::Callback<void(int32_t)> stop_callback_;
  base::Callback<void(int32_t)> seek_callback_;
  scoped_refptr<base::SingleThreadTaskRunner> main_cb_runner_;

  PP_Rect display_rect_;
  PP_FloatRect crop_ratio_rect_;
  PP_MediaPlayerDisplayMode display_mode_;

  PP_TimeTicks current_time_;

  base::WeakPtrFactory<PepperMediaPlayerPrivateTizen> weak_ptr_factory_;

  PepperTizenDRMManager drm_manager_;

  Evas_Object* video_window_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_MEDIA_PLAYER_PRIVATE_TIZEN_H_
