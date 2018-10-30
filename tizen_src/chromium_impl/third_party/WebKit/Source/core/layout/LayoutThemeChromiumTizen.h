// Copyright 2014, 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LayoutThemeChromiumTizen_h
#define LayoutThemeChromiumTizen_h

#include "core/layout/LayoutThemeDefault.h"

namespace blink {

class LayoutThemeChromiumTizen final : public LayoutThemeDefault {
 public:
  static RefPtr<LayoutTheme> Create();

  String ExtraDefaultStyleSheet() override;
  bool DelegatesMenuListRendering() const override;
  Color PlatformTapHighlightColor() const override;
  Color PlatformFocusRingColor() const override;
  bool ThemeDrawsFocusRing(const ComputedStyle&) const override;
  void AdjustInnerSpinButtonStyle(ComputedStyle&) const override;
};

} // namespace blink

#endif // LayoutThemeChromiumTizen_h
