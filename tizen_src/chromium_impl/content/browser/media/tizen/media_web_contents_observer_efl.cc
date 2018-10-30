// Copyright 2014 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/tizen/media_web_contents_observer_efl.h"

#include "content/browser/media/tizen/browser_media_player_manager_efl.h"
#include "content/browser/renderer_host/render_process_host_impl.h"
#include "content/common/media/media_player_messages_efl.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "ipc/ipc_message_macros.h"

namespace content {

MediaWebContentsObserver* CreateMediaWebContentsObserver(
    WebContents* web_contents) {
  return new MediaWebContentsObserver(web_contents);
}

MediaWebContentsObserver::MediaWebContentsObserver(WebContents* web_contents)
    : WebContentsObserver(web_contents), last_render_frame_host_(nullptr) {}

void MediaWebContentsObserver::MaybeUpdateAudibleState() {
  // NOTIMPLEMENTED();
}

void MediaWebContentsObserver::RenderFrameDeleted(
    RenderFrameHost* render_frame_host) {
  GetMediaPlayerManager()->CleanUpBelongToRenderFrameHost(render_frame_host);
}

bool MediaWebContentsObserver::OnMessageReceived(
    const IPC::Message& msg,
    RenderFrameHost* render_frame_host) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP_WITH_PARAM(MediaWebContentsObserver, msg,
                                   render_frame_host)
  IPC_MESSAGE_FORWARD(MediaPlayerEflHostMsg_Init, GetMediaPlayerManager(),
                      BrowserMediaPlayerManagerEfl::OnInitialize)
  IPC_MESSAGE_FORWARD(MediaPlayerEflHostMsg_DeInit, GetMediaPlayerManager(),
                      BrowserMediaPlayerManagerEfl::OnDestroy)
  IPC_MESSAGE_FORWARD(MediaPlayerEflHostMsg_Play, GetMediaPlayerManager(),
                      BrowserMediaPlayerManagerEfl::OnPlay)
  IPC_MESSAGE_FORWARD(MediaPlayerEflHostMsg_Pause, GetMediaPlayerManager(),
                      BrowserMediaPlayerManagerEfl::OnPause)
  IPC_MESSAGE_FORWARD(MediaPlayerEflHostMsg_SetVolume, GetMediaPlayerManager(),
                      BrowserMediaPlayerManagerEfl::OnSetVolume)
  IPC_MESSAGE_FORWARD(MediaPlayerEflHostMsg_SetRate, GetMediaPlayerManager(),
                      BrowserMediaPlayerManagerEfl::OnSetRate)
  IPC_MESSAGE_FORWARD(MediaPlayerEflHostMsg_Seek, GetMediaPlayerManager(),
                      BrowserMediaPlayerManagerEfl::OnSeek)
  IPC_MESSAGE_FORWARD(MediaPlayerEflHostMsg_EnteredFullscreen,
                      GetMediaPlayerManager(),
                      BrowserMediaPlayerManagerEfl::OnEnteredFullscreen)
  IPC_MESSAGE_FORWARD(MediaPlayerEflHostMsg_ExitedFullscreen,
                      GetMediaPlayerManager(),
                      BrowserMediaPlayerManagerEfl::OnExitedFullscreen)
#if defined(TIZEN_VIDEO_HOLE)
  IPC_MESSAGE_FORWARD(MediaPlayerEflHostMsg_SetGeometry,
                      GetMediaPlayerManager(),
                      BrowserMediaPlayerManagerEfl::OnSetGeometry)
#endif

  IPC_MESSAGE_FORWARD(MediaPlayerEflHostMsg_Suspend, GetMediaPlayerManager(),
                      BrowserMediaPlayerManagerEfl::OnSuspend)
  IPC_MESSAGE_FORWARD(MediaPlayerEflHostMsg_Resume, GetMediaPlayerManager(),
                      BrowserMediaPlayerManagerEfl::OnResume)
  IPC_MESSAGE_FORWARD(MediaPlayerEflHostMsg_Activate, GetMediaPlayerManager(),
                      BrowserMediaPlayerManagerEfl::OnActivate)
  IPC_MESSAGE_FORWARD(MediaPlayerEflHostMsg_Deactivate, GetMediaPlayerManager(),
                      BrowserMediaPlayerManagerEfl::OnDeactivate)
#if defined(OS_TIZEN_TV_PRODUCT)
  IPC_MESSAGE_FORWARD(MediaPlayerEflHostMsg_SetDecryptorHandle,
                      GetMediaPlayerManager(),
                      BrowserMediaPlayerManagerEfl::OnSetDecryptorHandle)
  IPC_MESSAGE_FORWARD(MediaPlayerEflHostMsg_DeInitSync, GetMediaPlayerManager(),
                      BrowserMediaPlayerManagerEfl::OnDestroySync)
  IPC_MESSAGE_FORWARD(MediaPlayerEflHostMsg_SetActiveTextTrack,
                      GetMediaPlayerManager(),
                      BrowserMediaPlayerManagerEfl::OnSetActiveTextTrack)
  IPC_MESSAGE_FORWARD(MediaPlayerEflHostMsg_SetActiveAudioTrack,
                      GetMediaPlayerManager(),
                      BrowserMediaPlayerManagerEfl::OnSetActiveAudioTrack)
  IPC_MESSAGE_FORWARD(MediaPlayerEflHostMsg_SetActiveVideoTrack,
                      GetMediaPlayerManager(),
                      BrowserMediaPlayerManagerEfl::OnSetActiveVideoTrack)
  IPC_MESSAGE_FORWARD(MediaPlayerEflHostMsg_NotifySubtitlePlay,
                      GetMediaPlayerManager(),
                      BrowserMediaPlayerManagerEfl::OnNotifySubtitlePlay)
  IPC_MESSAGE_FORWARD(MediaPlayerEflHostMsg_RegisterTimeline,
                      GetMediaPlayerManager(),
                      BrowserMediaPlayerManagerEfl::OnRegisterTimeline)
  IPC_MESSAGE_FORWARD(MediaPlayerEflHostMsg_UnRegisterTimeline,
                      GetMediaPlayerManager(),
                      BrowserMediaPlayerManagerEfl::OnUnRegisterTimeline)
  IPC_MESSAGE_FORWARD(MediaPlayerEflHostMsg_GetSpeed, GetMediaPlayerManager(),
                      BrowserMediaPlayerManagerEfl::OnGetSpeed)
  IPC_MESSAGE_FORWARD(MediaPlayerEflHostMsg_GetMrsUrl, GetMediaPlayerManager(),
                      BrowserMediaPlayerManagerEfl::OnGetMrsUrl)
  IPC_MESSAGE_FORWARD(MediaPlayerEflHostMsg_GetContentId,
                      GetMediaPlayerManager(),
                      BrowserMediaPlayerManagerEfl::OnGetContentId)
  IPC_MESSAGE_FORWARD(MediaPlayerEflHostMsg_GetTimelinePositions,
                      GetMediaPlayerManager(),
                      BrowserMediaPlayerManagerEfl::OnGetTimelinePositions)
  IPC_MESSAGE_FORWARD(MediaPlayerEflHostMsg_SyncTimeline,
                      GetMediaPlayerManager(),
                      BrowserMediaPlayerManagerEfl::OnSyncTimeline)
  IPC_MESSAGE_FORWARD(MediaPlayerEflHostMsg_SetWallClock,
                      GetMediaPlayerManager(),
                      BrowserMediaPlayerManagerEfl::OnSetWallClock)
  IPC_MESSAGE_FORWARD(MediaPlayerEflHostMsg_GetDroppedFrameCount,
                      GetMediaPlayerManager(),
                      BrowserMediaPlayerManagerEfl::OnGetDroppedFrameCount)
  IPC_MESSAGE_FORWARD(MediaPlayerEflHostMsg_GetDecodedFrameCount,
                      GetMediaPlayerManager(),
                      BrowserMediaPlayerManagerEfl::OnGetDecodedFrameCount)
  IPC_MESSAGE_FORWARD(MediaPlayerEflHostMsg_NotifyElementRemove,
                      GetMediaPlayerManager(),
                      BrowserMediaPlayerManagerEfl::OnElementRemove)
  IPC_MESSAGE_FORWARD(MediaPlayerEflHostMsg_SetParentalRatingResult,
                      GetMediaPlayerManager(),
                      BrowserMediaPlayerManagerEfl::OnSetParentalRatingResult)
  IPC_MESSAGE_FORWARD(FrameHostMsg_CheckHLSSupport, GetMediaPlayerManager(),
                      BrowserMediaPlayerManagerEfl::OnCheckHLS)
  IPC_MESSAGE_FORWARD(MediaPlayerEflHostMsg_ControlMediaPacket,
                      GetMediaPlayerManager(),
                      BrowserMediaPlayerManagerEfl::OnControlMediaPacket)
#endif
  IPC_MESSAGE_FORWARD(MediaPlayerEflHostMsg_GetStartDate,
                      GetMediaPlayerManager(),
                      BrowserMediaPlayerManagerEfl::OnGetStartDate)
  IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  // IPC_MESSAGE_FORWARD and other macros don't work with sync messages
  // with param.
  if (!handled) {
    last_render_frame_host_ = render_frame_host;
    handled = true;
    IPC_BEGIN_MESSAGE_MAP(MediaWebContentsObserver, msg)
    IPC_MESSAGE_HANDLER(MediaPlayerEflHostMsg_InitSync,
                        MediaWebContentsObserver::OnInitSync)
    IPC_MESSAGE_UNHANDLED(handled = false)
    IPC_END_MESSAGE_MAP()
  }
  return handled;
}

void MediaWebContentsObserver::OnInitSync(
    int player_id,
    MediaPlayerHostMsg_Initialize_Type type,
    const GURL& url,
    const std::string& mime_type,
    int demuxer_client_id,
    bool* success) {
  if (!last_render_frame_host_) {
    *success = false;
    return;
  }
  MediaPlayerInitConfig config{type, url, mime_type, demuxer_client_id, false};
  GetMediaPlayerManager()->OnInitializeSync(last_render_frame_host_, player_id,
                                            config, success);
  last_render_frame_host_ = nullptr;
}

BrowserMediaPlayerManagerEfl*
MediaWebContentsObserver::GetMediaPlayerManager() {
  return BrowserMediaPlayerManagerEfl::GetInstance();
}

}  // namespace content
