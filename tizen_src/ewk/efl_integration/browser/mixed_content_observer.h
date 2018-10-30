// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TIZEN_SRC_EWK_EFL_INTEGRATION_MIXED_CONTENT_OBSERVER_H
#define TIZEN_SRC_EWK_EFL_INTEGRATION_MIXED_CONTENT_OBSERVER_H

#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class WebContents;
class RenderFrameHost;
}  // namespace content

class MixedContentObserver
    : public content::WebContentsUserData<MixedContentObserver>,
      public content::WebContentsObserver {
 public:
  ~MixedContentObserver() override {}
  bool MixedContentReply(bool allow);

  // content::WebContentsObserver implementation.
  bool OnMessageReceived(const IPC::Message& message,
                         content::RenderFrameHost* render_frame_host) override;

 private:
  explicit MixedContentObserver(content::WebContents* web_contents);
  friend class content::WebContentsUserData<MixedContentObserver>;

  void OnDidBlockInsecureContent();

  DISALLOW_COPY_AND_ASSIGN(MixedContentObserver);
};

#endif  // TIZEN_SRC_EWK_EFL_INTEGRATION_MIXED_CONTENT_OBSERVER_H
