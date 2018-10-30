// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ScrollbarThemeTizen_h
#define ScrollbarThemeTizen_h

#include "platform/scroll/ScrollbarThemeAura.h"

namespace blink {

class PLATFORM_EXPORT ScrollbarThemeTizen : public ScrollbarThemeAura {
 public:
  int ScrollbarThickness(ScrollbarControlSize = kRegularScrollbar) override;
  bool UsesOverlayScrollbars() const override { return true; }

 protected:
  int MinimumThumbLength(const ScrollbarThemeClient&) override;

  void PaintScrollCorner(GraphicsContext&,
                         const DisplayItemClient&,
                         const IntRect&) override;
  void PaintTrackBackground(GraphicsContext&,
                            const Scrollbar&,
                            const IntRect&) override;
  void PaintButton(GraphicsContext&,
                   const Scrollbar&,
                   const IntRect&,
                   ScrollbarPart) override;
  void PaintThumb(GraphicsContext&, const Scrollbar&, const IntRect&) override;
  void PaintTrackPiece(GraphicsContext&,
                       const Scrollbar&,
                       const IntRect&,
                       ScrollbarPart) override {}
  IntSize ButtonSize(const ScrollbarThemeClient&) override;

 private:
  IntRect CornerRect(const ScrollbarThemeClient&) override;
  bool IsThinScrollbar(ScrollbarControlSize controlSize) {
    return controlSize == kSmallScrollbar;
  }
};

}  // namespace blink

#endif  // ScrollbarThemeTizen_h
