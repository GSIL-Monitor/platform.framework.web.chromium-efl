// Copyright (c) 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "scroll_client_observer.h"

#include "common/render_messages_ewk.h"
#include "common/web_contents_utils.h"
#include "eweb_view.h"
#include "ipc/ipc_message.h"

using web_contents_utils::WebViewFromWebContents;

DEFINE_WEB_CONTENTS_USER_DATA_KEY(ScrollClientObserver);

ScrollClientObserver::ScrollClientObserver(content::WebContents* webContents)
    : content::WebContentsObserver(webContents) {}

bool ScrollClientObserver::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(ScrollClientObserver, message)
  IPC_MESSAGE_HANDLER(EwkHostMsg_DidEdgeScrollBy, OnDidEdgeScrollBy)
  IPC_MESSAGE_HANDLER(EwkViewHostMsg_ScrollbarThumbPartFocusChanged,
                      OnScrollbarThumbPartFocusChanged)
  IPC_MESSAGE_HANDLER(EwkHostMsg_RunArrowScroll, OnRunArrowScroll)
  IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void ScrollClientObserver::OnDidEdgeScrollBy(gfx::Point offset, bool handled) {
  EWebView* web_view = WebViewFromWebContents(web_contents());
  if (web_view)
    web_view->InvokeEdgeScrollByCallback(offset, handled);
}

void ScrollClientObserver::OnScrollbarThumbPartFocusChanged(int orientation,
                                                            bool focused) {
  EWebView* web_view = WebViewFromWebContents(web_contents());
  if (web_view) {
    web_view->InvokeScrollbarThumbFocusChangedCallback(
        static_cast<Ewk_Scrollbar_Orientation>(orientation), focused);
  }
}

void ScrollClientObserver::OnRunArrowScroll() {
  EWebView* web_view = WebViewFromWebContents(web_contents());
  if (web_view)
    web_view->InvokeRunArrowScrollCallback();
}
