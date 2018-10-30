// Copyright (c) 2015 Samsung Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ACCESSIBILITY_BROWSER_ACCESSIBILITY_MANAGER_EFL_H
#define CONTENT_BROWSER_ACCESSIBILITY_BROWSER_ACCESSIBILITY_MANAGER_EFL_H

#include "content/browser/accessibility/browser_accessibility_manager_auralinux.h"

namespace content {

class BrowserAccessibilityEfl;

// Manages a tree of BrowserAccessibility objects
class CONTENT_EXPORT BrowserAccessibilityManagerEfl
    : public BrowserAccessibilityManagerAuraLinux {
 public:
  BrowserAccessibilityManagerEfl(
      AtkObject* parent_object,
      const ui::AXTreeUpdate& initial_tree,
      BrowserAccessibilityDelegate* delegate,
      BrowserAccessibilityFactory* factory = new BrowserAccessibilityFactory());

  ~BrowserAccessibilityManagerEfl() override;

  void NotifyAccessibilityEvent(BrowserAccessibilityEvent::Source source,
                                ui::AXEvent event_type,
                                BrowserAccessibility* node) override;
  int32_t GetFocusedObjectID();
  BrowserAccessibilityEfl* GetFocusedObject();

  void SetHighlighted(BrowserAccessibilityAuraLinux* obj);
  bool IsHighlighted(BrowserAccessibilityAuraLinux* obj) const {
    return highlighted_obj_ == obj;
  }
  void MaybeInvalidateHighlighted(BrowserAccessibilityAuraLinux* obj);
  void InvalidateHighlighted(bool focus_root);

 private:
#if defined(TIZEN_ATK_FEATURE_VD)
  void NotifyLiveRegionChanged(BrowserAccessibilityEfl* obj);
#endif
  friend class BrowserAccessibilityManager;
  BrowserAccessibilityAuraLinux* highlighted_obj_;

  DISALLOW_COPY_AND_ASSIGN(BrowserAccessibilityManagerEfl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_ACCESSIBILITY_BROWSER_ACCESSIBILITY_MANAGER_EFL_H
