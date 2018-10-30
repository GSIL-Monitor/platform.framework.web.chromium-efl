// Copyright 2014 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_TIZEN_MEDIA_WEB_CONTENTS_OBSERVER_EFL_H_
#define CONTENT_BROWSER_MEDIA_TIZEN_MEDIA_WEB_CONTENTS_OBSERVER_EFL_H_

#include <unordered_map>

#include "base/compiler_specific.h"
#include "content/common/content_export.h"
#include "content/common/media/media_player_messages_enums_efl.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {

class BrowserMediaPlayerManagerEfl;

// This class manages all RenderFrame based media related managers at the
// browser side. It receives IPC messages from media RenderFrameObservers and
// forwards them to the corresponding managers. The managers are responsible
// for sending IPCs back to the RenderFrameObservers at the render side.
class CONTENT_EXPORT MediaWebContentsObserver : public WebContentsObserver {
 public:
  explicit MediaWebContentsObserver(WebContents* web_contents);

  // Called by WebContentsImpl when the audible state may have changed.
  void MaybeUpdateAudibleState();

  // WebContentsObserver implementations.
  virtual void RenderFrameDeleted(RenderFrameHost* render_frame_host) override;
  virtual bool OnMessageReceived(const IPC::Message& message,
                                 RenderFrameHost* render_frame_host) override;

  // Gets the media player manager associated with |render_frame_host|. Creates
  // a new one if it doesn't exist. The caller doesn't own the returned pointer.
  BrowserMediaPlayerManagerEfl* GetMediaPlayerManager();

 private:
  void OnInitSync(int player_id,
                  MediaPlayerHostMsg_Initialize_Type type,
                  const GURL& url,
                  const std::string& mime_type,
                  int demuxer_client_id,
                  bool* success);

  RenderFrameHost* last_render_frame_host_;
  DISALLOW_COPY_AND_ASSIGN(MediaWebContentsObserver);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_TIZEN_MEDIA_WEB_CONTENTS_OBSERVER_EFL_H_
