// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_MEDIA_PLAYER_HOST_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_MEDIA_PLAYER_HOST_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "cc/layers/video_layer.h"
#include "content/browser/renderer_host/pepper/media_player/buffering_listener_private.h"
#include "content/browser/renderer_host/pepper/media_player/drm_listener_private.h"
#include "content/browser/renderer_host/pepper/media_player/media_events_listener_private.h"
#include "content/browser/renderer_host/pepper/media_player/subtitle_listener_private.h"
#include "media/blink/video_frame_compositor.h"
#include "ppapi/c/pp_rect.h"
#include "ppapi/c/samsung/pp_media_player_samsung.h"
#include "ppapi/host/resource_host.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "url/gurl.h"

namespace base {
class ScopedTempDir;
}

namespace content {

class PepperMediaPlayerPrivate;
class BrowserPpapiHost;

class PepperMediaPlayerHost : public ppapi::host::ResourceHost {
 public:
  class SubtitlesDownloader {
   public:
    static std::unique_ptr<SubtitlesDownloader> Create(
        const GURL& url,
        const base::Callback<base::ScopedTempDir*(void)>& temp_dir_callback,
        const base::Callback<void(int32_t, const base::FilePath&)>& callback);

    SubtitlesDownloader(
        const base::Callback<void(int32_t, const base::FilePath&)>& callback);
    virtual ~SubtitlesDownloader();

   protected:
    void PostDownloadCompleted(int32_t result, const base::FilePath& file_path);

   private:
    static void DownloadCompleted(
        const base::Callback<void(int32_t, const base::FilePath&)>& callback,
        int32_t result,
        const base::FilePath& file_path);

    base::Callback<void(int32_t, const base::FilePath&)> callback_;

    DISALLOW_COPY_AND_ASSIGN(SubtitlesDownloader);
  };

  PepperMediaPlayerHost(BrowserPpapiHost*,
                        PP_Instance,
                        PP_Resource,
                        PP_MediaPlayerMode);
  ~PepperMediaPlayerHost() override;

  bool IsMediaPlayerHost() override;
  void UpdatePluginView(const PP_Rect& plugin_rect, const PP_Rect& clip_rect);

 protected:
  // ppapi::host::ResourceHost override.
  int32_t OnResourceMessageReceived(
      const IPC::Message& msg,
      ppapi::host::HostMessageContext* context) override;

 private:
  PP_Rect GetViewportRect();
  bool UpdateVideoPosition(const base::Callback<void(int32_t)>& callback);
  int32_t OnSetMediaEventsListener(ppapi::host::HostMessageContext*,
                                   bool is_set);
  int32_t OnSetSubtitleListener(ppapi::host::HostMessageContext*, bool is_set);
  int32_t OnSetBufferingListener(ppapi::host::HostMessageContext*, bool is_set);
  int32_t OnSetDRMListener(ppapi::host::HostMessageContext*, bool is_set);
  int32_t OnAttachDataSource(ppapi::host::HostMessageContext*,
                             PP_Resource data_source);
  int32_t OnPlay(ppapi::host::HostMessageContext*);
  int32_t OnPause(ppapi::host::HostMessageContext*);
  int32_t OnStop(ppapi::host::HostMessageContext*);
  int32_t OnSeek(ppapi::host::HostMessageContext*, PP_TimeTicks);
  int32_t OnSetPlaybackRate(ppapi::host::HostMessageContext*, double);
  int32_t OnGetDuration(ppapi::host::HostMessageContext*);
  int32_t OnGetCurrentTime(ppapi::host::HostMessageContext*);
  int32_t OnGetPlayerState(ppapi::host::HostMessageContext*);
  int32_t OnGetCurrentVideoTrackInfo(ppapi::host::HostMessageContext*);
  int32_t OnGetVideoTracksList(ppapi::host::HostMessageContext*);
  int32_t OnGetCurrentAudioTrackInfo(ppapi::host::HostMessageContext*);
  int32_t OnGetAudioTracksList(ppapi::host::HostMessageContext*);
  int32_t OnGetCurrentTextTrackInfo(ppapi::host::HostMessageContext*);
  int32_t OnGetTextTracksList(ppapi::host::HostMessageContext*);
  int32_t OnSelectTrack(ppapi::host::HostMessageContext*,
                        PP_ElementaryStream_Type_Samsung,
                        uint32_t trackIndex);
  int32_t OnAddExternalSubtitles(ppapi::host::HostMessageContext*,
                                 const std::string& url,
                                 const std::string& encoding);
  int32_t OnSetSubtitlesDelay(ppapi::host::HostMessageContext*, PP_TimeDelta);
  int32_t OnSetDisplayRect(ppapi::host::HostMessageContext*, const PP_Rect&);
  int32_t OnSetDisplayMode(ppapi::host::HostMessageContext*,
                           PP_MediaPlayerDisplayMode);
  int32_t OnSetVr360Mode(ppapi::host::HostMessageContext*,
                         PP_MediaPlayerVr360Mode);
  int32_t OnSetVr360Rotation(ppapi::host::HostMessageContext*, float, float);
  int32_t OnSetVr360ZoomLevel(ppapi::host::HostMessageContext*, uint32_t);
  int32_t OnSetDRMSpecificData(ppapi::host::HostMessageContext*,
                               PP_MediaPlayerDRMType,
                               PP_MediaPlayerDRMOperation,
                               const std::string& data);

  void OnSubtitlesDownloaded(ppapi::host::ReplyMessageContext reply_context,
                             const std::string& encoding,
                             int32_t download_result,
                             const base::FilePath& path);

  base::ScopedTempDir* GetTempDir();

  std::unique_ptr<SubtitlesDownloader> downloader_;
  std::unique_ptr<base::ScopedTempDir> temp_dir_;

  // plugin_rect_ is plugin rect relative to the page. When scrolled
  // out of screen plugin_rect_ x and y may go negative.
  PP_Rect plugin_rect_;
  // video_position_ is rect of media player video relative to the plugin_rect_
  PP_Rect video_position_;
  // clip_rect is visible part of plugin relative to the plugin_rect_
  PP_Rect clip_rect_;
  // Notifies if initial plugin view data was sent from the renderer
  bool plugin_rect_initialized_;
  base::Callback<void(int32_t)> display_rect_cb_;

  BrowserPpapiHost* browser_ppapi_host_;
  scoped_refptr<PepperMediaPlayerPrivate> private_;
  base::WeakPtrFactory<PepperMediaPlayerHost> factory_;

  DISALLOW_COPY_AND_ASSIGN(PepperMediaPlayerHost);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_MEDIA_PLAYER_HOST_H_
