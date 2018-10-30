// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SETTINGS_CLIENT_EFL_H_
#define CONTENT_SETTINGS_CLIENT_EFL_H_

#include "content/public/renderer/render_frame_observer.h"
#include "third_party/WebKit/public/platform/WebContentSettingsClient.h"

#if defined(OS_TIZEN_TV_PRODUCT)
#include "content/public/renderer/render_frame_observer_tracker.h"
#endif

namespace content {

class ContentSettingsClientEfl
    : public RenderFrameObserver,
#if defined(OS_TIZEN_TV_PRODUCT)
      public RenderFrameObserverTracker<ContentSettingsClientEfl>,
#endif
      public blink::WebContentSettingsClient {
 public:
  explicit ContentSettingsClientEfl(RenderFrame* render_view);
  ~ContentSettingsClientEfl() override;

  // content::RenderFrameObserver implementation.
  void OnDestruct() override;

  // blink::WebContentSettingsClient implementation.
  void DidNotAllowScript() override;
#if defined(OS_TIZEN_TV_PRODUCT)
  // blink::WebContentSettingsClient implementation.
  bool AllowRunningInsecureContent(bool allowed_per_settings,
                                   const blink::WebSecurityOrigin& context,
                                   const blink::WebURL& url) override;
  bool OnMessageReceived(const IPC::Message& message) override;
#endif

 private:
#if defined(OS_TIZEN_TV_PRODUCT)
  bool AllowInsecureContent(bool allowed_per_settings);
  void OnSetAllowInsecureContent(bool allow);
  ContentSettingsClientEfl* GetParentClient(RenderFrame* render_frame);
  bool GetAllowInsecureContent(const ContentSettingsClientEfl* client);
  void SetMixedContentState(ContentSettingsClientEfl* client, bool state);
  bool GetMixedContentState(const ContentSettingsClientEfl* client);

  // Insecure content may be permitted for the duration of this render view.
  ContentSettingsClientEfl* parent_client_;
  bool mixed_content_state_;
  bool allow_insecure_content_;
#endif
  DISALLOW_COPY_AND_ASSIGN(ContentSettingsClientEfl);
};

}  // namespace content

#endif  // CONTENT_SETTINGS_CLIENT_EFL_H
