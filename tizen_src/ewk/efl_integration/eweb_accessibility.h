// Copyright 2015-17 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWEB_ACCESSIBILITY_H
#define EWEB_ACCESSIBILITY_H

#include <memory>
#include "eweb_accessibility_object.h"

class EWebView;

// pre-conditions:
// |ecore_main_loop| SHOULD be running before using this class.
//
// This class initializes and owns all accessibility objects.
class EWebAccessibility {
 public:
  EWebAccessibility(EWebView*);
  ~EWebAccessibility();

  void NotifyAccessibilityStatus(bool);
  AtkObject* RootObject();
  void OnFocusOut();

 private:
  void AddPlug();
  void RemovePlug();

  std::unique_ptr<EWebAccessibilityObject> accessibility_object_;
  EWebView* ewebview_;
};

#endif  // EWEB_ACCESSIBILITY_H
