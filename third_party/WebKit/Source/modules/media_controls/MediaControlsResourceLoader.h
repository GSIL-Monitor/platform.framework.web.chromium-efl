// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MediaControlsResourceLoader_h
#define MediaControlsResourceLoader_h

#include "core/css/CSSDefaultStyleSheets.h"

namespace blink {

// Builds the UA stylesheet for the Media Controls based on feature flags
// and platform. There is an Android specific stylesheet that we will include
// on Android or if devtools has enable mobile emulation.
class MediaControlsResourceLoader
    : public CSSDefaultStyleSheets::UAStyleSheetLoader {
 public:
  static void InjectMediaControlsUAStyleSheet();

  String GetUAStyleSheet() override;

  MediaControlsResourceLoader();
  ~MediaControlsResourceLoader() override;

 private:
  String GetMediaControlsCSS() const;
#if defined(OS_TIZEN_TV_PRODUCT)
  String GetTizenTVMediaControlsCSS() const;
#endif
  String GetMediaControlsAndroidCSS() const;

#if defined(TIZEN_MULTIMEDIA_SUPPORT)
  String GetTizenMediaControlsCSS() const;
#endif

  DISALLOW_COPY_AND_ASSIGN(MediaControlsResourceLoader);
};

}  // namespace blink

#endif  // MediaControlsResourceLoader_h
