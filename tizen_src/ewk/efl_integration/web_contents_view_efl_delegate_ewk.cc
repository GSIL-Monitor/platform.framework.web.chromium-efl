// Copyright 2015-2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "web_contents_view_efl_delegate_ewk.h"
#include "ewk/efl_integration/private/webview_delegate_ewk.h"

#include "eweb_view.h"

WebContentsViewEflDelegateEwk::WebContentsViewEflDelegateEwk(EWebView* wv)
    : web_view_(wv) {
}

/* LCOV_EXCL_START */
void WebContentsViewEflDelegateEwk::ShowPopupMenu(
    content::RenderFrameHost* render_frame_host,
    const gfx::Rect& bounds,
    int item_height,
    double item_font_size,
    int selected_item,
    const std::vector<content::MenuItem>& items,
    bool right_aligned,
    bool allow_multiple_selection) {
  web_view_->HandlePopupMenu(items, selected_item, allow_multiple_selection,
                             bounds);
}

void WebContentsViewEflDelegateEwk::HidePopupMenu() {
  web_view_->HidePopupMenu();
}

void WebContentsViewEflDelegateEwk::CancelContextMenu(int request_id) {
  web_view_->CancelContextMenu(request_id);
}

void WebContentsViewEflDelegateEwk::HandleZoomGesture(blink::WebGestureEvent& event) {
  web_view_->HandleZoomGesture(event);
}

bool WebContentsViewEflDelegateEwk::UseKeyPadWithoutUserAction() {
  return web_view_->GetSettings()->useKeyPadWithoutUserAction();
}
/* LCOV_EXCL_STOP */

/* LCOV_EXCL_START */
void WebContentsViewEflDelegateEwk::ShowContextMenu(
    const content::ContextMenuParams& params) {
  web_view_->ShowContextMenu(params);
}

void WebContentsViewEflDelegateEwk::OrientationLock(
    blink::WebScreenOrientationLockType orientation) {

  // Conversion of WebScreenOrientationLockType to
  // list of orientations.
  // This is related to the commentary to the M42 version
  // (currently non-existent) of prototype
  // Eina_Bool (*orientation_lock)(Ewk_View_Smart_Data* sd, int orientations)
  //   InvalidOrientation = 0,
  //   OrientationPortraitPrimary = 1,
  //   OrientationLandscapePrimary = 1 << 1,
  //   OrientationPortraitSecondary = 1 << 2,
  //   OrientationLandscapeSecondary = 1 << 3,
  //   OrientationPortrait =
  //       OrientationPortraitPrimary | OrientationPortraitSecondary,
  //   OrientationLandscape =
  //       OrientationLandscapePrimary | OrientationLandscapeSecondary,
  //   OrientationNatural =
  //       OrientationPortraitPrimary | OrientationLandscapePrimary,
  //   OrientationAny = OrientationPortrait | OrientationLandscape
  int orientation_convert[9] = { 0, 1, 4, 2, 8, 15, 10, 5, 3 };

  WebViewDelegateEwk::GetInstance().RequestHandleOrientationLock(
      web_view_,
      orientation_convert[static_cast<int>(orientation)]);
}

void WebContentsViewEflDelegateEwk::OrientationUnlock() {
  WebViewDelegateEwk::GetInstance().RequestHandleOrientationUnlock(web_view_);
}

bool WebContentsViewEflDelegateEwk::ImePanelEnabled() const {
  return web_view_->GetSettings()->imePanelEnabled();
}

/* LCOV_EXCL_STOP */
