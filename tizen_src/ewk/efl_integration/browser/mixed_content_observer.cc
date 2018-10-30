// Copyright (c) 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mixed_content_observer.h"

#include "common/render_messages_ewk.h"
#include "common/web_contents_utils.h"
#include "eweb_view.h"
#include "ipc/ipc_message.h"

using web_contents_utils::WebViewFromWebContents;

DEFINE_WEB_CONTENTS_USER_DATA_KEY(MixedContentObserver);

MixedContentObserver::MixedContentObserver(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {}

bool MixedContentObserver::OnMessageReceived(
    const IPC::Message& message,
    content::RenderFrameHost* render_frame_host) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(MixedContentObserver, message)
  IPC_MESSAGE_HANDLER(EwkHostMsg_DidBlockInsecureContent,
                      OnDidBlockInsecureContent)
  IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void MixedContentObserver::OnDidBlockInsecureContent() {
  EWebView* web_view = WebViewFromWebContents(web_contents());
  if (web_view) {
    web_view->SmartCallback<EWebViewCallbacks::DidBlockInsecureContent>()
        .call();
  }
}

bool MixedContentObserver::MixedContentReply(bool allow) {
  if (allow) {
    web_contents()->SendToAllFrames(
        new EwkMsg_SetAllowInsecureContent(MSG_ROUTING_NONE, true));
  }
  return true;
}
