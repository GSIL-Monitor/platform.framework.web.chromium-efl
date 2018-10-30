// Copyright 2015-17 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWEB_ACCESSIBILITY_UTIL_H
#define EWEB_ACCESSIBILITY_UTIL_H

#include "base/memory/singleton.h"

class EWebView;

// This singleton class initializes atk-bridge and
// registers an implementation of AtkUtil class.
// Global class that every accessible application
// needs to registers once.
class EWebAccessibilityUtil {
 public:
  static EWebAccessibilityUtil* GetInstance();
  void ToggleBrowserAccessibility(bool mode, EWebView* ewebview);
#if defined(TIZEN_ATK_FEATURE_VD)
  bool IsDeactivatedByApp() const { return deactivated_by_app_; }
  void Deactivate(bool deactivated);
#endif

 private:
  EWebAccessibilityUtil();
  ~EWebAccessibilityUtil();

  void NotifyAccessibilityStatus(bool mode);
  void InitAtkBridgeAdaptor();
  void CleanAtkBridgeAdaptor();

  bool atk_bridge_initialized_;
  friend struct base::DefaultSingletonTraits<EWebAccessibilityUtil>;
#if defined(TIZEN_ATK_FEATURE_VD)
  bool deactivated_by_app_;
#endif
};

#endif  // EWEB_ACCESSIBILITY_UTIL_H
