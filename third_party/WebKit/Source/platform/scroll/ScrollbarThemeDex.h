// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ScrollbarThemeDex_h
#define ScrollbarThemeDex_h

#include "platform/scroll/ScrollbarThemeOverlay.h"

namespace blink {

// This scrollbar theme is used to get a combination of non-overlay scrollbar
// and solid color scrollbar for Samsung Dex.
class PLATFORM_EXPORT ScrollbarThemeDex : public ScrollbarThemeOverlay {
 public:
  ScrollbarThemeDex(Color);
  ~ScrollbarThemeDex() override {}

  bool UsesOverlayScrollbars() const override;
  void PaintScrollCorner(GraphicsContext&,
                         const DisplayItemClient&,
                         const IntRect& cornerRect) override;
  ScrollbarPart HitTest(const ScrollbarThemeClient&, const IntPoint&) override;
};

}  // namespace blink

#endif
