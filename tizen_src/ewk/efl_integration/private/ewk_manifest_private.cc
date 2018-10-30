// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ewk_manifest_private.h"

#include "third_party/WebKit/public/platform/WebDisplayMode.h"
#include "third_party/WebKit/public/platform/modules/screen_orientation/WebScreenOrientationLockType.h"

_Ewk_View_Request_Manifest::_Ewk_View_Request_Manifest(
    const content::Manifest& manifest) {
  if (!manifest.short_name.is_null())
    short_name = base::UTF16ToUTF8(manifest.short_name.string());

  if (!manifest.name.is_null())
    name = base::UTF16ToUTF8(manifest.name.string());

  start_url = manifest.start_url.possibly_invalid_spec();

  if (!manifest.spp_sender_id.is_null())
    spp_sender_id = base::UTF16ToUTF8(manifest.spp_sender_id.string());

  int orientation = static_cast<int>(manifest.orientation);
  orientation_type = static_cast<Ewk_View_Orientation_Type>(orientation);

  int display = static_cast<int>(manifest.display);
  web_display_mode = static_cast<Ewk_View_Web_Display_Mode>(display);

  // Set the theme color based on the manifest value.
  theme_color = manifest.theme_color;

  // Set the background color based on the manifest value.
  background_color = manifest.background_color;

  // If any icons are specified in the manifest, they take precedence over any
  // we picked up from the web_app stuff.
  for (const auto& icon : manifest.icons) {
    Icon ewk_icon;
    ewk_icon.src = icon.src.possibly_invalid_spec();
    ewk_icon.type = base::UTF16ToUTF8(icon.type);

    for (const auto& size : icon.sizes) {
      _Ewk_View_Request_Manifest::Size size_pair;
      size_pair.width = size.width();
      size_pair.height = size.height();
      ewk_icon.sizes.push_back(size_pair);
    }
    icons.push_back(ewk_icon);
  }
}

_Ewk_View_Request_Manifest::~_Ewk_View_Request_Manifest() {}

_Ewk_View_Request_Manifest::Icon::Icon() {}
_Ewk_View_Request_Manifest::Icon::~Icon() {}

static_assert(int(blink::kWebDisplayModeUndefined) ==
                  int(WebDisplayModeUndefined),
              "mismatching enums: WebDisplayModeUndefined");
static_assert(int(blink::kWebDisplayModeBrowser) == int(WebDisplayModeBrowser),
              "mismatching enums: WebDisplayModeBrowser");
static_assert(int(blink::kWebDisplayModeMinimalUi) ==
                  int(WebDisplayModeMinimalUi),
              "mismatching enums: WebDisplayModeMinimalUi");
static_assert(int(blink::kWebDisplayModeStandalone) ==
                  int(WebDisplayModeStandalone),
              "mismatching enums: WebDisplayModeStandalone");
static_assert(int(blink::kWebDisplayModeFullscreen) ==
                  int(WebDisplayModeFullscreen),
              "mismatching enums: WebDisplayModeFullscreen");
static_assert(int(blink::kWebDisplayModeLast) == int(WebDisplayModeLast),
              "mismatching enums: WebDisplayModeLast");

static_assert(int(blink::kWebScreenOrientationLockDefault) ==
                  int(WebScreenOrientationLockDefault),
              "mismatching enums: WebScreenOrientationLockDefault");
static_assert(int(blink::kWebScreenOrientationLockPortraitPrimary) ==
                  int(WebScreenOrientationLockPortraitPrimary),
              "mismatching enums: WebScreenOrientationLockPortraitPrimary");
static_assert(int(blink::kWebScreenOrientationLockPortraitSecondary) ==
                  int(WebScreenOrientationLockPortraitSecondary),
              "mismatching enums: WebScreenOrientationLockPortraitSecondary");
static_assert(int(blink::kWebScreenOrientationLockLandscapePrimary) ==
                  int(WebScreenOrientationLockLandscapePrimary),
              "mismatching enums: WebScreenOrientationLockLandscapePrimary");
static_assert(int(blink::kWebScreenOrientationLockLandscapeSecondary) ==
                  int(WebScreenOrientationLockLandscapeSecondary),
              "mismatching enums: WebScreenOrientationLockLandscapeSecondary");
static_assert(int(blink::kWebScreenOrientationLockAny) ==
                  int(WebScreenOrientationLockAny),
              "mismatching enums: WebScreenOrientationLockAny");
static_assert(int(blink::kWebScreenOrientationLockLandscape) ==
                  int(WebScreenOrientationLockLandscape),
              "mismatching enums: WebScreenOrientationLockLandscape");
static_assert(int(blink::kWebScreenOrientationLockPortrait) ==
                  int(WebScreenOrientationLockPortrait),
              "mismatching enums: WebScreenOrientationLockPortrait");
static_assert(int(blink::kWebScreenOrientationLockNatural) ==
                  int(WebScreenOrientationLockNatural),
              "mismatching enums: WebScreenOrientationLockNatural");
