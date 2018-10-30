/*
 * Copyright (C) 2003, 2006, 2007, 2008, 2009, 2011, 2012 Apple Inc. All rights
 * reserved.
 *           (C) 2006 Graham Dennis (graham.dennis@gmail.com)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef Settings_h
#define Settings_h

#include <memory>
#include "bindings/core/v8/V8CacheOptions.h"
#include "bindings/core/v8/V8CacheStrategiesForCacheStorage.h"
#include "core/CoreExport.h"
#include "core/SettingsMacros.h"
#include "core/dom/events/AddEventListenerOptionsDefaults.h"
#include "core/editing/EditingBehaviorTypes.h"
#include "core/editing/SelectionStrategy.h"
#include "core/frame/SettingsDelegate.h"
#include "core/html/media/AutoplayPolicy.h"
#include "core/html/track/TextTrackKindUserPreference.h"
#include "core/loader/FrameLoaderTypes.h"
#include "platform/Timer.h"
#include "platform/fonts/GenericFontFamilySettings.h"
#include "platform/geometry/IntSize.h"
#include "platform/graphics/HighContrastSettings.h"
#include "platform/graphics/ImageAnimationPolicy.h"
#include "platform/weborigin/KURL.h"
#include "public/platform/PointerProperties.h"
#include "public/platform/WebDisplayMode.h"
#include "public/platform/WebViewportStyle.h"

namespace blink {

class CORE_EXPORT Settings {
  WTF_MAKE_NONCOPYABLE(Settings);
  USING_FAST_MALLOC(Settings);

 public:
  static std::unique_ptr<Settings> Create();

  GenericFontFamilySettings& GetGenericFontFamilySettings() {
    return generic_font_family_settings_;
  }
  void NotifyGenericFontFamilyChange() {
    Invalidate(SettingsDelegate::kFontFamilyChange);
  }

  void SetTextAutosizingEnabled(bool);
  bool TextAutosizingEnabled() const { return text_autosizing_enabled_; }

  // Only set by Layout Tests, and only used if textAutosizingEnabled() returns
  // true.
  void SetTextAutosizingWindowSizeOverride(const IntSize&);
  const IntSize& TextAutosizingWindowSizeOverride() const {
    return text_autosizing_window_size_override_;
  }

  SETTINGS_GETTERS_AND_SETTERS

  // FIXME: This does not belong here.
  static void SetMockScrollbarsEnabled(bool flag);
  static bool MockScrollbarsEnabled();

#if defined(TIZEN_VD_NATIVE_SCROLLBARS)
  static void SetUseNativeScrollbars(bool);
  static bool UseNativeScrollbars();
#endif

#if defined(USE_EFL)
  void SetUsesEncodingDetector(bool encoding_detector) {
    uses_encoding_detector = encoding_detector;
  }
  bool UsesEncodingDetector() const { return uses_encoding_detector; }
  void SetTizenVersion(unsigned major, unsigned minor, unsigned release);
  bool TizenCompatibilityModeEnabled() const;
#endif
#if defined(OS_TIZEN)
  void SetHwKeyboardConnected(bool connected) {
    hw_keyboard_connected_ = connected;
  }
  bool HwKeyboardConnected() const { return hw_keyboard_connected_; }
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
  void SetMediaPlaybackNotificationEnabled(bool enabled) {
    media_playback_notification_enabled_ = enabled;
  }
  bool MediaPlaybackNotificationEnabled() const {
    return media_playback_notification_enabled_;
  }
  void SetMediaSubtitleNotificationEnabled(bool enabled) {
    media_subtitle_notification_enabled_ = enabled;
  }
  bool MediaSubtitleNotificationEnabled() const {
    return media_subtitle_notification_enabled_;
  }
  void SetGpuVendorKant2Used(bool used) { gpu_vendor_kant2_used_ = used; }
  bool GetGpuVendorKant2Used() const { return gpu_vendor_kant2_used_; }
#endif

  void SetDelegate(SettingsDelegate*);

 private:
  Settings();

  void Invalidate(SettingsDelegate::ChangeType);

  SettingsDelegate* delegate_;

  GenericFontFamilySettings generic_font_family_settings_;
  IntSize text_autosizing_window_size_override_;
  bool text_autosizing_enabled_ : 1;

#if defined(USE_EFL)
  bool uses_encoding_detector : 1;
  struct {
    unsigned major;
    unsigned minor;
    unsigned release;
    bool TizenCompatibilityModeEnabled() const {
      return (major && major < 3);
    }
  } tizen_compatibility_settings_;
#endif
#if defined(OS_TIZEN)
  bool hw_keyboard_connected_ : 1;
#endif
#if defined(TIZEN_ATK_SUPPORT)
  bool atk_enabled_ : 1;
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
  bool media_playback_notification_enabled_ : 1;
  bool media_subtitle_notification_enabled_ : 1;
  bool gpu_vendor_kant2_used_ : 1;
#endif

  SETTINGS_MEMBER_VARIABLES
};

}  // namespace blink

#endif  // Settings_h
