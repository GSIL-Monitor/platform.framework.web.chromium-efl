// Copyright (c) 2015-17 Samsung Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ACCESSIBILITY_BROWSER_ACCESSIBILITY_EFL_H_
#define CONTENT_BROWSER_ACCESSIBILITY_BROWSER_ACCESSIBILITY_EFL_H_

#include "content/browser/accessibility/browser_accessibility.h"
#include "content/browser/accessibility/browser_accessibility_auralinux.h"
#include "content/common/content_export.h"
#include "ui/gfx/geometry/rect.h"

#include <atk/atk.h>

namespace gfx {
class Point;
}

namespace content {

// Represents an object in the accessibility tree.
class CONTENT_EXPORT BrowserAccessibilityEfl
    : public BrowserAccessibilityAuraLinux {
 public:
  BrowserAccessibilityEfl();

  bool is_highlightable() const { return highlightable_; }
  gfx::Rect GetScaledGlobalBoundsRect() const;
  BrowserAccessibility* GetChildAtPoint(const gfx::Point& point) const;
  void RecalculateHighlightable(bool recalculate_ancestors);
  bool IsChecked() const;
  bool IsCheckable() const;
  bool IsColorWell() const;
  gchar* GetObjectText() const;
  bool IsStaticText() const;
  bool IsTextObjectType() const;
  void SetInterfaceFromObject(GType, int);
  void SetInterfaceMaskFromObject(int&);
#if defined(TIZEN_ATK_FEATURE_VD)
  std::string GetSupplementaryText() const;
  std::string GetSubTreeSpeechContent() const;
  std::string GetSpeechContent();
  bool HasSpeechContent();
#endif

 private:
  bool IsSection() const;
  std::string GetObjectValue() const;
  bool IsAccessible() const;
  void RecalculateHighlightableSingle();
  bool IsUnknown() const;
  bool IsInlineTextBox() const;
  bool IsFocusable() const;
  bool HasTextValue() const;
  bool HasText() const;
  bool MoreThanOneKnownChild() const;
  bool IsDocument() const;
#if defined(TIZEN_ATK_FEATURE_VD)
  bool HasOnlyTextChildren() const;
  bool HasOnlyTextAndImageChildren() const;
  std::string GetChildrenText() const;
#endif

  bool highlightable_;
};

CONTENT_EXPORT const BrowserAccessibilityEfl* ToBrowserAccessibilityEfl(
    const BrowserAccessibilityAuraLinux* obj);

CONTENT_EXPORT BrowserAccessibilityEfl* ToBrowserAccessibilityEfl(
    BrowserAccessibilityAuraLinux* obj);

CONTENT_EXPORT const BrowserAccessibilityEfl* ToBrowserAccessibilityEfl(
    const BrowserAccessibility* obj);

CONTENT_EXPORT BrowserAccessibilityEfl* ToBrowserAccessibilityEfl(
    BrowserAccessibility* obj);

}  // namespace content

#endif  // CONTENT_BROWSER_ACCESSIBILITY_BROWSER_ACCESSIBILITY_EFL_H_