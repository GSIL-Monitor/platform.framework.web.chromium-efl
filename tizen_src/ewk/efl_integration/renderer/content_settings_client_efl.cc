// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/content_settings_client_efl.h"

#include "common/render_messages_ewk.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"

#if defined(OS_TIZEN_TV_PRODUCT)
#include "common/application_type.h"
#include "content/public/renderer/render_view.h"
#endif

namespace content {

/* LCOV_EXCL_START */
ContentSettingsClientEfl::ContentSettingsClientEfl(RenderFrame* render_frame)
#if !defined(OS_TIZEN_TV_PRODUCT)
    : RenderFrameObserver(render_frame) {
#else
    : RenderFrameObserver(render_frame),
      RenderFrameObserverTracker<ContentSettingsClientEfl>(render_frame),
      mixed_content_state_(false),
      allow_insecure_content_(false) {
#endif
  render_frame->GetWebFrame()->SetContentSettingsClient(this);

#if defined(OS_TIZEN_TV_PRODUCT)
  // get allow_insecure_content_ from parent frame
  parent_client_ = GetParentClient(render_frame);
  GetAllowInsecureContent(parent_client_);
  GetMixedContentState(parent_client_);
#endif
}

ContentSettingsClientEfl::~ContentSettingsClientEfl() {}

void ContentSettingsClientEfl::DidNotAllowScript() {
  Send(new EwkHostMsg_DidNotAllowScript(routing_id()));
}

void ContentSettingsClientEfl::OnDestruct() {
  delete this;
}
/* LCOV_EXCL_STOP */

#if defined(OS_TIZEN_TV_PRODUCT)
bool ContentSettingsClientEfl::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(ContentSettingsClientEfl, message)
  IPC_MESSAGE_HANDLER(EwkMsg_SetAllowInsecureContent, OnSetAllowInsecureContent)
  IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

bool ContentSettingsClientEfl::AllowInsecureContent(bool allowed_per_settings) {
  if (allowed_per_settings)
    return true;

  if (GetMixedContentState(parent_client_))
    return allow_insecure_content_;
  Send(new EwkHostMsg_DidBlockInsecureContent(routing_id()));
  SetMixedContentState(parent_client_, true);
  return false;
}

bool ContentSettingsClientEfl::AllowRunningInsecureContent(
    bool allowed_per_settings,
    const blink::WebSecurityOrigin& origin,
    const blink::WebURL& resource_url) {
  if (IsWebBrowser()) {
    // For floating video window feature
    std::string str_url = resource_url.GetString().Utf8().data();
    if (!str_url.compare(0, 16, "http://127.0.0.1") ||
        !str_url.compare(0, 16, "http://localhost"))
      return true;
  }
  return AllowInsecureContent(allowed_per_settings);
}

inline ContentSettingsClientEfl* ContentSettingsClientEfl::GetParentClient(
    RenderFrame* render_frame) {
  RenderFrame* main_frame = NULL;
  ContentSettingsClientEfl* parent_client = NULL;
  if (render_frame && render_frame->GetRenderView())
    main_frame = render_frame->GetRenderView()->GetMainRenderFrame();
  if (main_frame)
    parent_client = ContentSettingsClientEfl::Get(main_frame);

  return parent_client;
}

void ContentSettingsClientEfl::OnSetAllowInsecureContent(bool allow) {
  allow_insecure_content_ = allow;
}

inline bool ContentSettingsClientEfl::GetAllowInsecureContent(
    const ContentSettingsClientEfl* client) {
  if (client)
    allow_insecure_content_ = client->allow_insecure_content_;

  return allow_insecure_content_;
}

void ContentSettingsClientEfl::SetMixedContentState(
    ContentSettingsClientEfl* client,
    bool state) {
  if (client)
    client->mixed_content_state_ = state;
  mixed_content_state_ = state;
}

inline bool ContentSettingsClientEfl::GetMixedContentState(
    const ContentSettingsClientEfl* client) {
  if (client)
    mixed_content_state_ = client->mixed_content_state_;

  return mixed_content_state_;
}

#endif

}  // namespace content
