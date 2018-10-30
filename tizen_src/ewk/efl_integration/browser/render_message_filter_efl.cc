// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/render_message_filter_efl.h"

#include "common/render_messages_ewk.h"
#include "common/web_contents_utils.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "eweb_view.h"
#include "ipc_message_start_ewk.h"
#include "net/url_request/url_request_context_getter.h"

using web_contents_utils::WebContentsFromViewID;
using web_contents_utils::WebViewFromWebContents;
using content::BrowserThread;

const uint32_t kFilteredMessageClasses[] = {
    EwkMsgStart, ChromeMsgStart,
};

RenderMessageFilterEfl::RenderMessageFilterEfl(int render_process_id)
    : BrowserMessageFilter(kFilteredMessageClasses,
                           arraysize(kFilteredMessageClasses)),
      render_process_id_(render_process_id) {}

RenderMessageFilterEfl::~RenderMessageFilterEfl() {}

void RenderMessageFilterEfl::OverrideThreadForMessage(
    const IPC::Message& message,
    content::BrowserThread::ID* thread) {
  switch (message.type()) {
    case EwkHostMsg_DecideNavigationPolicy::ID:
      *thread = content::BrowserThread::UI;
      break;
  }
}

bool RenderMessageFilterEfl::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(RenderMessageFilterEfl, message)
  IPC_MESSAGE_HANDLER(EwkHostMsg_DecideNavigationPolicy,
                      OnDecideNavigationPolicy)
  IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void RenderMessageFilterEfl::OnDecideNavigationPolicy(
    NavigationPolicyParams params,
    bool* handled) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  if (content::WebContents* web_contents =
          WebContentsFromViewID(render_process_id_, params.render_view_id)) {
    WebViewFromWebContents(web_contents)
        ->InvokePolicyNavigationCallback(params, handled);
  }
}
