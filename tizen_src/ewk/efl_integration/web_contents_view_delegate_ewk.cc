// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "web_contents_view_delegate_ewk.h"

#include "eweb_view.h"

WebContentsViewDelegateEwk::WebContentsViewDelegateEwk(EWebView* wv)
    : web_view_(wv) {
}

// Note: WebContentsViewDelegate::ShowContextMenu is the hook called
// by chromium in response to either a long press gesture (in case of
// touch-based input event is the source) or a right button mouse click
// (in case source is mouse-based).
// For the former, EWK apps enter selection mode, whereas for the
// later, context menu is shown right way.
/* LCOV_EXCL_START */
void WebContentsViewDelegateEwk::ShowContextMenu(
    content::RenderFrameHost* render_frame_host,
    const content::ContextMenuParams& params) {
  if (params.source_type == ui::MENU_SOURCE_LONG_PRESS) {
    CHECK(web_view_->TouchEventsEnabled());
    web_view_->HandleLongPressGesture(params);
  } else {
    web_view_->ShowContextMenu(params);
  }
}
/* LCOV_EXCL_STOP */

void WebContentsViewDelegateEwk::OnSelectionRectReceived(
    const gfx::Rect& selection_rect) const {
  web_view_->OnSelectionRectReceived(selection_rect);
}
