// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/view_messages.h"
#include "content/renderer/render_view_impl.h"
#include "third_party/WebKit/public/platform/WebRect.h"
#include "third_party/WebKit/public/web/WebView.h"
#include "ui/gfx/geometry/dip_util.h"

namespace content {

void RenderViewImpl::OnSelectFocusedLink() {
  blink::WebElement element = GetFocusedElement();
  if (element.IsNull() || element.IsEditable() ||
      element.HasHTMLTagName("select"))
    return;

  blink::WebRect box_rect = element.BoundsInViewport();
  if (box_rect.IsEmpty())
    return;

  float page_scale = webview()->PageScaleFactor();
  int focused_x = (box_rect.x + box_rect.width / 2) / page_scale;
  int focused_y = (box_rect.y + box_rect.height / 2) / page_scale;

  OnSelectClosestWord(focused_x, focused_y);
}

void RenderViewImpl::OnRequestSelectionRect() {
  Send(new ViewHostMsg_SelectionRectReceived(
      routing_id_, gfx::Rect(webview()->currentSelectionRect())));
}
}  // namespace content
