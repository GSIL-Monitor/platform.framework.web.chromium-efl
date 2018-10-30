// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/media_controls/MediaControlsResourceLoader.h"

#include "blink/public/resources/grit/media_controls_resources.h"
#include "build/build_config.h"
#include "core/style/ComputedStyle.h"
#include "platform/media/ResourceBundleHelper.h"
#include "platform/runtime_enabled_features.h"
#include "platform/wtf/text/WTFString.h"

namespace {

bool ShouldLoadAndroidCSS() {
#if defined(OS_ANDROID)
  return true;
#else
  return blink::RuntimeEnabledFeatures::MobileLayoutThemeEnabled();
#endif
}

}  // namespace

namespace blink {

MediaControlsResourceLoader::MediaControlsResourceLoader()
    : UAStyleSheetLoader() {}

MediaControlsResourceLoader::~MediaControlsResourceLoader() {}

// Official Android builds that have enable_resource_whitelist_generation
// turned on will fail to compile due to an unknown-pragmas warning. In
// Chromium this is expected, but in Blink the compiler treats warnings as
// errors and so will fail to compile.
#if defined(OS_WIN)
#pragma warning(disable : 4068)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wunknown-pragmas"
#endif
String MediaControlsResourceLoader::GetMediaControlsCSS() const {
  return ResourceBundleHelper::UncompressResourceAsString(
      RuntimeEnabledFeatures::ModernMediaControlsEnabled()
          ? IDR_UASTYLE_MODERN_MEDIA_CONTROLS_CSS
          : IDR_UASTYLE_LEGACY_MEDIA_CONTROLS_CSS);
};

String MediaControlsResourceLoader::GetMediaControlsAndroidCSS() const {
  if (RuntimeEnabledFeatures::ModernMediaControlsEnabled())
    return String();
  return ResourceBundleHelper::UncompressResourceAsString(
      IDR_UASTYLE_LEGACY_MEDIA_CONTROLS_ANDROID_CSS);
};
// Re-enable the warnings.
#if defined(OS_WIN)
#pragma warning(default : 4068)
#else
#pragma GCC diagnostic pop
#endif

#if defined(TIZEN_MULTIMEDIA_SUPPORT)
String MediaControlsResourceLoader::GetTizenMediaControlsCSS() const {
  return ResourceBundleHelper::UncompressResourceAsString(
      IDR_UASTYLE_TIZEN_MEDIA_CONTROLS_CSS);
}
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
String MediaControlsResourceLoader::GetTizenTVMediaControlsCSS() const {
  return ResourceBundleHelper::UncompressResourceAsString(
      IDR_UASTYLE_TIZEN_TV_MEDIA_CONTROLS_CSS);
}
#endif

String MediaControlsResourceLoader::GetUAStyleSheet() {
  String style_sheets = GetMediaControlsCSS();
#if defined(TIZEN_MULTIMEDIA_SUPPORT)
  style_sheets = style_sheets + GetTizenMediaControlsCSS();
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
  style_sheets = style_sheets + GetTizenTVMediaControlsCSS();
#endif

  if (ShouldLoadAndroidCSS())
    return GetMediaControlsCSS() + GetMediaControlsAndroidCSS();
  return style_sheets;
}

void MediaControlsResourceLoader::InjectMediaControlsUAStyleSheet() {
  CSSDefaultStyleSheets& default_style_sheets =
      CSSDefaultStyleSheets::Instance();
  std::unique_ptr<MediaControlsResourceLoader> loader =
      std::make_unique<MediaControlsResourceLoader>();

  if (!default_style_sheets.HasMediaControlsStyleSheetLoader())
    default_style_sheets.SetMediaControlsStyleSheetLoader(std::move(loader));
}

}  // namespace blink
