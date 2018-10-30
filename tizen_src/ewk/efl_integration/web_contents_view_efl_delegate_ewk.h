// Copyright 2015-2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEB_CONTENTS_VIEW_EFL_DELEGATE_EWK
#define WEB_CONTENTS_VIEW_EFL_DELEGATE_EWK

#include "content/public/browser/web_contents_view_efl_delegate.h"

#include "content/public/common/menu_item.h"

class EWebView;

namespace content {
class RenderFrameHost;
}

class WebContentsViewEflDelegateEwk
    : public content::WebContentsViewEflDelegate {
 public:
  WebContentsViewEflDelegateEwk(EWebView*);

  /* LCOV_EXCL_START */
  void ShowPopupMenu(
      content::RenderFrameHost* render_frame_host,
      const gfx::Rect& bounds,
      int item_height,
      double item_font_size,
      int selected_item,
      const std::vector<content::MenuItem>& items,
      bool right_aligned,
      bool allow_multiple_selection) override;
  void HidePopupMenu() override;

  void CancelContextMenu(int request_id) override;

  void HandleZoomGesture(blink::WebGestureEvent& event) override;

  bool UseKeyPadWithoutUserAction() override;

  void ShowContextMenu(const content::ContextMenuParams& params) override;

  virtual void OrientationLock(blink::WebScreenOrientationLockType) override;
  virtual void OrientationUnlock() override;
  bool ImePanelEnabled() const override;
  /* LCOV_EXCL_STOP */

 private:
  EWebView* web_view_;
};

#endif // WEB_CONTENTS_VIEW_EFL_DELEGATE_EWK

