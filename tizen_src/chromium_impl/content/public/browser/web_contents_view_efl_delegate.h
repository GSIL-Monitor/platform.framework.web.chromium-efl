// Copyright 2015-2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_WEB_CONTENTS_VIEW_EFL_DELEGATE_H_
#define CONTENT_PUBLIC_BROWSER_WEB_CONTENTS_VIEW_EFL_DELEGATE_H_

#include <vector>

#include "content/common/content_export.h"
#include "content/public/common/context_menu_params.h"
#include "content/public/common/menu_item.h"
#include "third_party/WebKit/public/platform/WebGestureEvent.h"
#include "third_party/WebKit/public/platform/WebInputEvent.h"
#include "third_party/WebKit/public/platform/modules/screen_orientation/WebScreenOrientationLockType.h"
#include "ui/gfx/geometry/rect.h"

namespace content {

class RenderFrameHost;

class WebContentsViewEflDelegate {
 public:
  WebContentsViewEflDelegate() {};
  virtual ~WebContentsViewEflDelegate() {};

  virtual void ShowPopupMenu(
      RenderFrameHost* render_frame_host,
      const gfx::Rect& bounds,
      int item_height,
      double item_font_size,
      int selected_item,
      const std::vector<MenuItem>& items,
      bool right_aligned,
      bool allow_multiple_selection) = 0;
  virtual void HidePopupMenu() = 0;

  virtual void CancelContextMenu(int request_id) = 0;

  virtual void HandleZoomGesture(blink::WebGestureEvent& event) = 0;

  virtual bool UseKeyPadWithoutUserAction() = 0;

  virtual void ShowContextMenu(const ContextMenuParams& params) = 0;

  virtual void OrientationLock(blink::WebScreenOrientationLockType) = 0;
  virtual void OrientationUnlock() = 0;
  virtual bool ImePanelEnabled() const = 0;
};

} // namespace content

#endif // CONTENT_PUBLIC_BROWSER_WEB_CONTENTS_VIEW_EFL_DELEGATE_H_
