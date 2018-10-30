// Copyright (c) 2015 Samsung Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/accessibility/browser_accessibility_efl.h"

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "content/browser/accessibility/browser_accessibility_manager.h"
#include "tizen/system_info.h"
#include "ui/accessibility/ax_role_properties.h"
#include "ui/display/device_display_info_efl.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/dip_util.h"
#include "ui/gfx/geometry/point.h"

namespace {
enum AtkInterfaces {
  ATK_ACTION_INTERFACE,
  ATK_COMPONENT_INTERFACE,
  ATK_DOCUMENT_INTERFACE,
  ATK_EDITABLE_TEXT_INTERFACE,
  ATK_HYPERLINK_INTERFACE,
  ATK_HYPERTEXT_INTERFACE,
  ATK_IMAGE_INTERFACE,
  ATK_SELECTION_INTERFACE,
  ATK_TABLE_INTERFACE,
  ATK_TEXT_INTERFACE,
  ATK_VALUE_INTERFACE,
};
}

namespace content {

static BrowserAccessibilityAuraLinux* ToBrowserAccessibilityAuraLinux(
    BrowserAccessibilityAtk* atk_object) {
  if (!atk_object)
    return nullptr;

  return atk_object->m_object;
}

//
// AtkText interface
//

static BrowserAccessibilityAuraLinux* ToBrowserAccessibilityAuraLinux(
    AtkText* atk_text) {
  if (!IS_BROWSER_ACCESSIBILITY(atk_text))
    return nullptr;

  return ToBrowserAccessibilityAuraLinux(BROWSER_ACCESSIBILITY(atk_text));
}

static gchar* BrowserAccessibilityGetText(AtkText* atk_text,
                                          gint start_offset,
                                          gint end_offset) {
  g_return_val_if_fail(ATK_IS_TEXT(atk_text), nullptr);

  BrowserAccessibilityAuraLinux* obj =
      ToBrowserAccessibilityAuraLinux(atk_text);
  if (!obj)
    return nullptr;

  return ToBrowserAccessibilityEfl(obj)->GetObjectText();
}

static void TextInterfaceInit(AtkTextIface* iface) {
  iface->get_text = BrowserAccessibilityGetText;
}

static const GInterfaceInfo TextInfo = {
    reinterpret_cast<GInterfaceInitFunc>(TextInterfaceInit), 0, 0};

// static
BrowserAccessibility* BrowserAccessibility::Create() {
  return new BrowserAccessibilityEfl();
}

const BrowserAccessibilityEfl* ToBrowserAccessibilityEfl(
    const BrowserAccessibilityAuraLinux* obj) {
  DCHECK(!obj || obj->IsNative());
  return static_cast<const BrowserAccessibilityEfl*>(obj);
}

BrowserAccessibilityEfl* ToBrowserAccessibilityEfl(
    BrowserAccessibilityAuraLinux* obj) {
  DCHECK(!obj || obj->IsNative());
  return static_cast<BrowserAccessibilityEfl*>(obj);
}

const BrowserAccessibilityEfl* ToBrowserAccessibilityEfl(
    const BrowserAccessibility* obj) {
  DCHECK(!obj || obj->IsNative());
  return static_cast<const BrowserAccessibilityEfl*>(
      ToBrowserAccessibilityAuraLinux(obj));
}

BrowserAccessibilityEfl* ToBrowserAccessibilityEfl(BrowserAccessibility* obj) {
  DCHECK(!obj || obj->IsNative());
  return static_cast<BrowserAccessibilityEfl*>(
      ToBrowserAccessibilityAuraLinux(obj));
}

BrowserAccessibilityEfl::BrowserAccessibilityEfl() : highlightable_(false) {}

bool BrowserAccessibilityEfl::IsCheckable() const {
  return HasIntAttribute(ui::AX_ATTR_CHECKED_STATE);
}

bool BrowserAccessibilityEfl::IsColorWell() const {
  return GetRole() == ui::AX_ROLE_COLOR_WELL;
}

bool BrowserAccessibilityEfl::IsChecked() const {
  return GetIntAttribute(ui::AX_ATTR_CHECKED_STATE) ==
         ui::AX_CHECKED_STATE_TRUE;
}

bool BrowserAccessibilityEfl::IsStaticText() const {
  return GetRole() == ui::AX_ROLE_STATIC_TEXT;
}

bool BrowserAccessibilityEfl::IsTextObjectType() const {
  int32_t role = GetRole();
  switch (role) {
    case ui::AX_ROLE_HEADING:
    case ui::AX_ROLE_LINK:
    case ui::AX_ROLE_PARAGRAPH:
    case ui::AX_ROLE_POP_UP_BUTTON:
    case ui::AX_ROLE_STATIC_TEXT:
    case ui::AX_ROLE_TEXT_FIELD:
    case ui::AX_ROLE_INLINE_TEXT_BOX:
      return true;
    default:
      return false;
  }
  return false;
}

bool BrowserAccessibilityEfl::IsSection() const {
  switch (GetRole()) {
    case ui::AX_ROLE_GROUP:
    case ui::AX_ROLE_GENERIC_CONTAINER:
      return true;
    default:
      return false;
  }
}

void BrowserAccessibilityEfl::SetInterfaceFromObject(GType type,
                                                     int interface_mask) {
  if (interface_mask & (1 << ATK_TEXT_INTERFACE))
    g_type_add_interface_static(type, ATK_TYPE_TEXT, &TextInfo);
}

void BrowserAccessibilityEfl::SetInterfaceMaskFromObject(int& interface_mask) {
  // AX_ATTR_NAME attribute value which contains link or heading contents will
  // be read by screen-reader anyway as it is returned from get_name atk
  // interface.
  auto role = GetRole();
  if (role == ui::AX_ROLE_HEADING || role == ui::AX_ROLE_LINK)
    return;

  if (IsTextObjectType() || IsColorWell() || IsSection())
    interface_mask |= 1 << ATK_TEXT_INTERFACE;

  if (role == ui::AX_ROLE_TEXT_FIELD)
    interface_mask |= 1 << ATK_EDITABLE_TEXT_INTERFACE;
}

gchar* BrowserAccessibilityEfl::GetObjectText() const {
  // For color inputs, return string with attributes
  if (IsColorWell()) {
    int color = GetIntAttribute(ui::AX_ATTR_COLOR_VALUE);
    int red = (color >> 16) & 0xFF;
    int green = (color >> 8) & 0xFF;
    int blue = color & 0xFF;
    return g_strdup_printf("rgb %7.5f %7.5f %7.5f 1", red / 255., green / 255.,
                           blue / 255.);
  }

#if defined(TIZEN_ATK_FEATURE_VD)
  if (IsTvProfile())
    return g_strdup(GetSupplementaryText().c_str());
#endif

  // Always prefer value first
  std::string text = GetStringAttribute(ui::AX_ATTR_VALUE);
  if (!text.empty())
    return g_strdup(text.c_str());
  if (GetRole() == ui::AX_ROLE_TEXT_FIELD) {
    GetHtmlAttribute("placeholder", &text);
    if (!text.empty())
      return g_strdup(text.c_str());
  }
  text = GetStringAttribute(ui::AX_ATTR_DESCRIPTION);
  if (!text.empty())
    return g_strdup(text.c_str());

  return g_strdup(GetObjectValue().c_str());
}

std::string BrowserAccessibilityEfl::GetObjectValue() const {
  std::string value = GetStringAttribute(ui::AX_ATTR_VALUE);
  if (!value.empty())
    return value;
  for (uint32_t i = 0; i < InternalChildCount(); i++) {
    BrowserAccessibility* child = InternalGetChild(i);
    if (child) {
      std::string child_value =
          ToBrowserAccessibilityEfl(child)->GetObjectValue();
      if (value.length() > 0 &&
          !base::IsAsciiWhitespace(value[value.length() - 1]) &&
          (!child_value.empty() && !base::IsAsciiWhitespace(child_value[0])))
        value += " ";
      value += child_value;
    }
  }
  return value;
}

gfx::Rect BrowserAccessibilityEfl::GetScaledGlobalBoundsRect() const {
  if (!IsMobileProfile())
    return gfx::Rect();

  float device_scale_factor =
      display::Screen::GetScreen()->GetPrimaryDisplay().device_scale_factor();
  auto rect = ConvertRectToPixel(device_scale_factor, GetScreenBoundsRect());
  // TODO (m.niesluchow@samsung.com)
  // Mobile device scale factor is 2.0, so there is a problem in casting web
  // content to pixels. There is a one pixel information error, so it is better
  // to have smaller rect.
  // In newer opensource chromium code this problem does not exist due to using
  // gfx::RectF instead of gfx::Rect.
  rect.SetRect(rect.x() + 1, rect.y() + 1,
               rect.width() <= 1 ? rect.width() : (rect.width() - 2),
               rect.height() <= 1 ? rect.height() : (rect.height() - 2));
  auto root = manager()->GetRoot();
  if (root != this)
    rect.Intersect(
        ToBrowserAccessibilityEfl(root)->GetScaledGlobalBoundsRect());
  return rect;
}

BrowserAccessibility* BrowserAccessibilityEfl::GetChildAtPoint(
    const gfx::Point& point) const {
  if (!IsMobileProfile())
    return nullptr;

  for (int i = static_cast<int>(PlatformChildCount()) - 1; i >= 0; --i) {
    BrowserAccessibility* child = PlatformGetChild(i);
    if (child->GetRole() == ui::AX_ROLE_COLUMN)
      continue;
    if (child->GetScreenBoundsRect().Contains(point))
      return child;
  }
  return nullptr;
}

void BrowserAccessibilityEfl::RecalculateHighlightable(
    bool recalculate_ancestors) {
  if (!IsMobileProfile())
    return;

  for (uint32_t i = 0; i < InternalChildCount(); i++) {
    auto child = ToBrowserAccessibilityEfl(InternalGetChild(i));
    if (child)
      child->RecalculateHighlightable(false);
  }
  RecalculateHighlightableSingle();
  if (recalculate_ancestors) {
    BrowserAccessibility* parent = PlatformGetParent();
    while (parent) {
      BrowserAccessibilityEfl* parent_efl = ToBrowserAccessibilityEfl(parent);
      parent_efl->RecalculateHighlightableSingle();
      parent = parent_efl->PlatformGetParent();
    }
  }
}

void BrowserAccessibilityEfl::RecalculateHighlightableSingle() {
  if (!IsMobileProfile())
    return;

  highlightable_ = IsAccessible();
  AtkObject* atk_obj = GetAtkObject();
  if (atk_obj)
    atk_object_notify_state_change(atk_obj, ATK_STATE_HIGHLIGHTABLE,
                                   highlightable_);
}

bool BrowserAccessibilityEfl::IsUnknown() const {
  return (IsMobileProfile()) ? GetRole() == ui::AX_ROLE_UNKNOWN : false;
}

bool BrowserAccessibilityEfl::IsInlineTextBox() const {
  return (IsMobileProfile()) ? GetRole() == ui::AX_ROLE_INLINE_TEXT_BOX : false;
}

bool BrowserAccessibilityEfl::IsFocusable() const {
  return (IsMobileProfile()) ? HasState(ui::AX_STATE_FOCUSABLE) : false;
}

bool BrowserAccessibilityEfl::HasTextValue() const {
  if (!IsMobileProfile())
    return false;

  if (IsTextObjectType() && !GetStringAttribute(ui::AX_ATTR_VALUE).empty())
    return true;
  for (uint32_t i = 0; i < InternalChildCount(); i++) {
    BrowserAccessibility* child = InternalGetChild(i);
    if (child && ToBrowserAccessibilityEfl(child)->HasTextValue())
      return true;
  }
  return false;
}

bool BrowserAccessibilityEfl::HasText() const {
  if (!IsMobileProfile())
    return false;

  std::string text = GetStringAttribute(ui::AX_ATTR_VALUE);
  if (!text.empty())
    return true;
  std::string description = GetStringAttribute(ui::AX_ATTR_DESCRIPTION);
  if (!description.empty())
    return true;
  for (uint32_t i = 0; i < InternalChildCount(); i++) {
    BrowserAccessibility* child = InternalGetChild(i);
    if (child && ToBrowserAccessibilityEfl(child)->HasTextValue())
      return true;
  }
  return false;
}

bool BrowserAccessibilityEfl::MoreThanOneKnownChild() const {
  if (!IsMobileProfile())
    return false;

  uint32_t count = 0;
  for (uint32_t i = 0; i < InternalChildCount(); i++) {
    BrowserAccessibilityEfl* child_efl =
        ToBrowserAccessibilityEfl(InternalGetChild(i));
    if (child_efl && (child_efl->IsUnknown() ||
                      child_efl->GetRole() == ui::AX_ROLE_LINE_BREAK))
      continue;
    ++count;
    if (count > 1)
      return true;
  }
  return false;
}

bool BrowserAccessibilityEfl::IsDocument() const {
  if (!IsMobileProfile())
    return false;

  switch (GetRole()) {
    case ui::AX_ROLE_DOCUMENT:
    case ui::AX_ROLE_ROOT_WEB_AREA:
    case ui::AX_ROLE_WEB_AREA:
      return true;
    default:
      return false;
  }
  return false;
}

bool BrowserAccessibilityEfl::IsAccessible() const {
  if (!IsMobileProfile())
    return false;

  if (IsUnknown())
    return false;
  if (IsFocusable() || ui::IsControl(GetRole()))
    return true;
  // Text object types are accessible for its direct reading purpose, while
  // sections allow to group accessible nodes for better ui experiance.
  if (!IsTextObjectType() && !IsSection())
    return false;
  // There is no need to group only one child (skipping unknown ones).
  if (IsSection() && !MoreThanOneKnownChild())
    return false;
  // Some text object may be empty.
  if (!HasText())
    return false;
  // We continue further check on parent as static text can be grouped.
  if (!IsStaticText() && !IsInlineTextBox())
    return true;

  // Check if parent can be a grouper of static text. If not then static text
  // is accessible.
  BrowserAccessibility* parent = PlatformGetParent();
  if (!parent)
    return true;
  BrowserAccessibilityEfl* parent_efl = ToBrowserAccessibilityEfl(parent);
  if (parent_efl->IsUnknown())
    return true;
  // Don't let document be grouping object
  if (parent_efl->IsDocument())
    return true;
  if (parent_efl->IsFocusable() || ui::IsControl(parent_efl->GetRole()))
    return false;
  if (!parent_efl->IsTextObjectType() && !parent_efl->IsSection())
    return true;
  if (parent_efl->IsSection() && !parent_efl->MoreThanOneKnownChild())
    return true;

  return false;
}

#if defined(TIZEN_ATK_FEATURE_VD)
std::string BrowserAccessibilityEfl::GetSupplementaryText() const {
  // name and description have been got from BrowserAccessibilityAuraLinux
  // if return here again, it will be speak twice
  if (!GetStringAttribute(ui::AX_ATTR_NAME).empty() ||
      !GetStringAttribute(ui::AX_ATTR_DESCRIPTION).empty())
    return std::string();

  // Always prefer value first
  std::string text = GetStringAttribute(ui::AX_ATTR_VALUE);
  if (!text.empty())
    return text;

  if (GetRole() == ui::AX_ROLE_TEXT_FIELD) {
    GetHtmlAttribute("placeholder", &text);
    if (!text.empty())
      return text;
  }

  if (HasOnlyTextChildren() ||
      (IsFocusable() && HasOnlyTextAndImageChildren())) {
    text.append(GetChildrenText());
    if (!text.empty())
      return text;
  }

  return GetObjectValue();
}

std::string BrowserAccessibilityEfl::GetChildrenText() const {
  std::string text;
  for (uint32_t i = 0; i < InternalChildCount(); i++) {
    BrowserAccessibility* child = InternalGetChild(i);
    if (child) {
      BrowserAccessibilityEfl* obj = ToBrowserAccessibilityEfl(child);
      std::string name = obj->GetStringAttribute(ui::AX_ATTR_NAME);
      if (!text.empty() && !name.empty())
        text.append(" ");
      text.append(name);

      std::string description =
          obj->GetStringAttribute(ui::AX_ATTR_DESCRIPTION);
      if (!text.empty() && !description.empty())
        text.append(" ");
      text.append(description);

      std::string supplementary = obj->GetSupplementaryText();
      if (!text.empty() && !supplementary.empty())
        text.append(" ");
      text.append(supplementary);
    }
  }
  return text;
}

bool BrowserAccessibilityEfl::HasOnlyTextChildren() const {
  for (uint32_t i = 0; i < InternalChildCount(); i++) {
    BrowserAccessibility* child = InternalGetChild(i);
    if (child && ToBrowserAccessibilityEfl(child) &&
        !ToBrowserAccessibilityEfl(child)->IsTextObjectType())
      return false;
  }
  return true;
}

bool BrowserAccessibilityEfl::HasOnlyTextAndImageChildren() const {
  for (uint32_t i = 0; i < InternalChildCount(); i++) {
    BrowserAccessibility* child = InternalGetChild(i);
    if (child && child->GetRole() != ui::AX_ROLE_STATIC_TEXT &&
        child->GetRole() != ui::AX_ROLE_IMAGE) {
      return false;
    }
  }
  return true;
}

bool BrowserAccessibilityEfl::HasSpeechContent() {
  if (!GetStringAttribute(ui::AX_ATTR_NAME).empty() ||
      !GetStringAttribute(ui::AX_ATTR_DESCRIPTION).empty())
    return true;

  int mask = 0;
  SetInterfaceMaskFromObject(mask);
  return ((mask & (1 << ATK_TEXT_INTERFACE)) && !GetSupplementaryText().empty())
             ? true
             : false;
}

std::string BrowserAccessibilityEfl::GetSpeechContent() {
  std::string result;
  if (!GetStringAttribute(ui::AX_ATTR_NAME).empty()) {
    result.append(GetStringAttribute(ui::AX_ATTR_NAME));
    result.append(" ");
  }

  if (!GetStringAttribute(ui::AX_ATTR_DESCRIPTION).empty()) {
    result.append(GetStringAttribute(ui::AX_ATTR_DESCRIPTION));
    result.append(" ");
  }

  int mask = 0;
  SetInterfaceMaskFromObject(mask);
  if ((mask & (1 << ATK_TEXT_INTERFACE)) && !GetSupplementaryText().empty()) {
    result.append(GetSupplementaryText());
    result.append(" ");
  }

  return result;
}

std::string BrowserAccessibilityEfl::GetSubTreeSpeechContent() const {
  std::string result;

  // if name from contents, will get it from children
  if (GetIntAttribute(ui::AX_ATTR_NAME_FROM) != ui::AX_NAME_FROM_CONTENTS &&
      !GetStringAttribute(ui::AX_ATTR_NAME).empty()) {
    result.append(GetStringAttribute(ui::AX_ATTR_NAME));
    result.append(" ");
  }

  if (!GetStringAttribute(ui::AX_ATTR_DESCRIPTION).empty()) {
    result.append(GetStringAttribute(ui::AX_ATTR_DESCRIPTION));
    result.append(" ");
  }

  for (uint32_t i = 0; i < InternalChildCount(); i++) {
    BrowserAccessibility* child = InternalGetChild(i);
    if (child && ToBrowserAccessibilityEfl(child)) {
      BrowserAccessibilityEfl* obj = ToBrowserAccessibilityEfl(child);
      std::string child_string = obj->GetSpeechContent();
      if (!result.empty() && !child_string.empty())
        result.append(" ");
      result.append(child_string);
    }
  }

  return result;
}
#endif

}  // namespace content
