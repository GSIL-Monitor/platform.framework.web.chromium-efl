// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/scroll/ScrollbarThemeDex.h"

#include "platform/graphics/GraphicsContext.h"
#include "platform/graphics/paint/DrawingRecorder.h"
#include "public/platform/Platform.h"
#include "public/platform/WebRect.h"
#include "public/platform/WebThemeEngine.h"

namespace blink {

static const int kThumbThickness = 15;
static const int kScrollbarMargin = 0;

ScrollbarThemeDex::ScrollbarThemeDex(Color color)
    : ScrollbarThemeOverlay(kThumbThickness,
                            kScrollbarMargin,
                            ScrollbarThemeOverlay::kAllowHitTest,
                            color) {}

bool ScrollbarThemeDex::UsesOverlayScrollbars() const {
  return false;
}

// Android does not draw scrollbars using WebThemeEngine.
// Hence ScrollbarTheme::paintScrollCorner draws corner as a button.
// Override this function to paint the scroll corner as solid color.
void ScrollbarThemeDex::PaintScrollCorner(
    GraphicsContext& context,
    const DisplayItemClient& displayItemClient,
    const IntRect& cornerRect) {
  if (cornerRect.IsEmpty())
    return;

  if (DrawingRecorder::UseCachedDrawingIfPossible(
          context, displayItemClient, DisplayItem::kScrollbarCorner))
    return;

  DrawingRecorder recorder(context, displayItemClient,
                           DisplayItem::kScrollbarCorner, cornerRect);
  context.FillRect(cornerRect, Color::kGray);
}

ScrollbarPart ScrollbarThemeDex::HitTest(const ScrollbarThemeClient& scrollbar,
                                         const IntPoint& position) {
  return ScrollbarTheme::HitTest(scrollbar, position);
}

}  // namespace blink
