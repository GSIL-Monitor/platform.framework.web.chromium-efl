// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEB_CONTENTS_VIEW_EFL_DELEGATE_EWK
#define WEB_CONTENTS_VIEW_EFL_DELEGATE_EWK

#include "content/public/browser/web_contents_view_delegate.h"

#include "content/public/common/menu_item.h"

class EWebView;

namespace content {
class RenderFrameHost;
}

class WebContentsViewDelegateEwk
    : public content::WebContentsViewDelegate {
 public:
  WebContentsViewDelegateEwk(EWebView*);

  void ShowContextMenu(
      content::RenderFrameHost* render_frame_host,
      const content::ContextMenuParams& params) override;

  void OnSelectionRectReceived(const gfx::Rect& selection_rect) const override;

 private:
  EWebView* web_view_;
};

#endif // WEB_CONTENTS_VIEW_EFL_DELEGATE_EWK

