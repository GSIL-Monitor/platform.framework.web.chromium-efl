// Copyright 2014, 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "LayoutThemeChromiumTizen.h"

#include "TizenUserAgentStyleSheets.h"
#include "core/style/ComputedStyle.h"
#include "platform/LayoutTestSupport.h"
#include "public/platform/Platform.h"
#include "public/platform/WebThemeEngine.h"
#include "tizen/system_info.h"

namespace blink {

static const RGBA32 kDefaultTapHighlightColorTizen = 0x2eee6e1a; // Light orange
static const RGBA32 kDefaultFocusOutlineColorTizen = 0xff00dae5; // (255,0,218,229)
static const RGBA32 kFocusOutlineColorTizenTv = 0xff0077f6; // (255,0,119,246)

RefPtr<LayoutTheme> LayoutThemeChromiumTizen::Create() {
  return WTF::AdoptRef(new LayoutThemeChromiumTizen());
}

LayoutTheme& LayoutTheme::NativeTheme() {
  DEFINE_STATIC_REF(LayoutTheme, layout_theme,
      LayoutThemeChromiumTizen::Create());
  return *layout_theme;
}

String LayoutThemeChromiumTizen::ExtraDefaultStyleSheet() {
  return LayoutThemeDefault::ExtraDefaultStyleSheet() +
#if defined(OS_TIZEN_TV_PRODUCT)
      String(themeChromiumTizenTVCss, sizeof(themeChromiumTizenTVCss));
#else
      String(themeChromiumTizenCss, sizeof(themeChromiumTizenCss));
#endif
}

bool LayoutThemeChromiumTizen::DelegatesMenuListRendering() const {
  if (IsTvProfile())
    return false;

  return true;
}

Color LayoutThemeChromiumTizen::PlatformTapHighlightColor() const {
  return kDefaultTapHighlightColorTizen;
}

Color LayoutThemeChromiumTizen::PlatformFocusRingColor() const {
  if (IsTvProfile())
    return kFocusOutlineColorTizenTv;

  return kDefaultFocusOutlineColorTizen;
}

bool LayoutThemeChromiumTizen::ThemeDrawsFocusRing(
    const ComputedStyle& style) const {
  if (LayoutTestSupport::IsRunningLayoutTest()) {
    // Don't use focus rings for buttons when mocking controls.
    return style.Appearance() == kButtonPart ||
           style.Appearance() == kPushButtonPart ||
           style.Appearance() == kSquareButtonPart;
  }

  // Partially draw focus-outline on some elements for compatibility.
  if (style.Appearance() == kTextFieldPart ||
      style.Appearance() == kTextAreaPart ||
      style.Appearance() == kMenulistPart ||
      style.Appearance() == kMenulistButtonPart ||
      style.Appearance() == kMenulistTextPart ||
      style.Appearance() == kMenulistTextFieldPart ||
      style.Appearance() == kListboxPart ||
      style.Appearance() == kSquareButtonPart) {
    return false;
  }

  // Except above cases, chromium-efl do not draw focus-outline by defalut.
  return true;
}

void LayoutThemeChromiumTizen::AdjustInnerSpinButtonStyle(
    ComputedStyle& style) const {
  if (!IsMobileProfile() && !IsWearableProfile()) {
    LayoutThemeDefault::AdjustInnerSpinButtonStyle(style);
    return;
  }

  if (LayoutTestSupport::IsRunningLayoutTest()) {
    IntSize size = Platform::Current()->ThemeEngine()->
        GetSize(WebThemeEngine::kPartInnerSpinButton);

    style.SetWidth(Length(size.Width(), kFixed));
    style.SetMinWidth(Length(size.Width(), kFixed));
  }
}

} // namespace blink
