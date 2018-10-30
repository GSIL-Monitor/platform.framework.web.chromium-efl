// Copyright (c) 2015 Samsung Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/accessibility/browser_accessibility_manager_efl.h"

#include <atk/atk.h>

#include "content/browser/accessibility/browser_accessibility_efl.h"
#include "tizen/system_info.h"
#include "ui/accessibility/ax_enums.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_conversions.h"

namespace content {

// static
BrowserAccessibilityManager* BrowserAccessibilityManager::Create(
    const ui::AXTreeUpdate& initial_tree,
    BrowserAccessibilityDelegate* delegate,
    BrowserAccessibilityFactory* factory) {
  return new BrowserAccessibilityManagerEfl(nullptr, initial_tree, delegate,
                                            factory);
}

BrowserAccessibilityManagerEfl*
BrowserAccessibilityManager::ToBrowserAccessibilityManagerEfl() {
  return static_cast<BrowserAccessibilityManagerEfl*>(this);
}

BrowserAccessibilityManagerEfl::BrowserAccessibilityManagerEfl(
    AtkObject* parent_object,
    const ui::AXTreeUpdate& initial_tree,
    BrowserAccessibilityDelegate* delegate,
    BrowserAccessibilityFactory* factory)
    : BrowserAccessibilityManagerAuraLinux(parent_object,
                                           initial_tree,
                                           delegate,
                                           factory),
      highlighted_obj_(nullptr) {}

BrowserAccessibilityManagerEfl::~BrowserAccessibilityManagerEfl() {}

int32_t BrowserAccessibilityManagerEfl::GetFocusedObjectID() {
  BrowserAccessibility* root = GetRoot();
  if (!root)
    return -1;

  int32_t focus_id = root->manager()->GetTreeData().sel_focus_object_id;
  return focus_id ? focus_id : -1;
}

BrowserAccessibilityEfl* BrowserAccessibilityManagerEfl::GetFocusedObject() {
  BrowserAccessibility* result = GetFromID(GetFocusedObjectID());
  return result ? ToBrowserAccessibilityEfl(result) : nullptr;
}

void BrowserAccessibilityManagerEfl::NotifyAccessibilityEvent(
    BrowserAccessibilityEvent::Source source,
    ui::AXEvent event_type,
    BrowserAccessibility* node) {
  BrowserAccessibilityEfl* obj = ToBrowserAccessibilityEfl(node);
  if (!obj || !obj->GetAtkObject())
    return;

  BrowserAccessibility* active_descendant = nullptr;
  AtkObject* atk_object = obj->GetAtkObject();
  switch (event_type) {
    case ui::AX_EVENT_CHECKED_STATE_CHANGED:
      if (obj->IsCheckable()) {
        atk_object_notify_state_change(atk_object, ATK_STATE_CHECKED,
                                       obj->IsChecked());
      }
      break;
    case ui::AX_EVENT_FOCUS:
      atk_object_notify_state_change(atk_object, ATK_STATE_FOCUSED, true);
      break;
    case ui::AX_EVENT_TEXT_SELECTION_CHANGED: {
      auto focused_object = GetFocusedObject();
      if (focused_object) {
        if (focused_object->IsStaticText() &&
            focused_object->PlatformGetParent()) {
          auto parent =
              ToBrowserAccessibilityEfl(focused_object->PlatformGetParent());
          atk_object_notify_state_change(parent->GetAtkObject(),
                                         ATK_STATE_FOCUSED, true);
        }
        // TODO(g.ludwikowsk): use correct offset for new caret position here
        g_signal_emit_by_name(focused_object->GetAtkObject(),
                              "text-caret-moved", 0);
      }
    } break;
    case ui::AX_EVENT_VALUE_CHANGED:
      if (ATK_IS_VALUE(atk_object)) {
        AtkPropertyValues propertyValues = {"accessible-value", G_VALUE_INIT,
                                            G_VALUE_INIT};
        g_signal_emit_by_name(atk_object, "property-change::accessible-value",
                              &propertyValues, nullptr);
      }
      break;
    case ui::AX_EVENT_TEXT_CHANGED:
#if defined(TIZEN_ATK_FEATURE_VD)
      if (obj->GetRole() == ui::AX_ROLE_STATIC_TEXT &&
          (obj->HasStringAttribute(ui::AX_ATTR_CONTAINER_LIVE_STATUS) ||
           obj->HasStringAttribute(ui::AX_ATTR_LIVE_STATUS))) {
        NotifyLiveRegionChanged(obj);
        break;
      }
#endif
      // TODO(k.czech): send text changed notification
      break;
#if defined(TIZEN_ATK_FEATURE_VD)
    // when a node create in aria live region, parent element will notify
    // AX_EVENT_CHILDREN_CHANGED and AX_EVENT_LIVE_REGION_CHANGED from blink
    // we have customize some code in
    // BrowserAccessibilityManager::OnAtomicUpdateFinished to detect the created
    // children, and notify AX_EVENT_CHILDREN_CHANGED for the created child from
    // treeChange the following solution is base on the notify from treechange,
    // take example: a <p> create will create 2 object: a paragraph object and a
    // text object
    // If possible we should just notify children changed event, not faking a
    // text changed event that need web engine to consider all
    // of the situdation to combinate a speech content for a subtree (many
    // aria-tags need to consider, and that's what the screen reader do)
    // but currently screen reader not support children changed event
    case ui::AX_EVENT_LIVE_REGION_CHANGED:
      if (source != BrowserAccessibilityEvent::FromTreeChange)
        break;

      NotifyLiveRegionChanged(obj);
      break;

#endif
    case ui::AX_EVENT_HIDE:
      if (IsMobileProfile()) {
        MaybeInvalidateHighlighted(obj);
        obj->RecalculateHighlightable(true);
      }
      break;
    case ui::AX_EVENT_ACTIVEDESCENDANTCHANGED:
      active_descendant = GetActiveDescendant(node);
      if (active_descendant)
        atk_object_notify_state_change(
            ToBrowserAccessibilityEfl(active_descendant)->GetAtkObject(),
            ATK_STATE_FOCUSED, true);
      break;
    default:
      break;
  }
}

void BrowserAccessibilityManagerEfl::SetHighlighted(
    BrowserAccessibilityAuraLinux* obj) {
  if (!IsMobileProfile())
    return;

  if (highlighted_obj_ == obj)
    return;

  InvalidateHighlighted(false);

  ScrollToMakeVisible(*obj, gfx::Rect(obj->GetFrameBoundsRect().size()));
  SetFocus(*obj);
  highlighted_obj_ = obj;
  if (highlighted_obj_->GetAtkObject() &&
      !highlighted_obj_->GetBoolAttribute(ui::AX_ATTR_BUSY))
    atk_object_notify_state_change(highlighted_obj_->GetAtkObject(),
                                   ATK_STATE_HIGHLIGHTED, true);
}

void BrowserAccessibilityManagerEfl::MaybeInvalidateHighlighted(
    BrowserAccessibilityAuraLinux* obj) {
  if (!IsMobileProfile())
    return;

  if (!IsHighlighted(obj))
    return;
  InvalidateHighlighted(true);
}

void BrowserAccessibilityManagerEfl::InvalidateHighlighted(bool focus_root) {
  if (!IsMobileProfile())
    return;

  if (!highlighted_obj_)
    return;
  // highlighted_obj_ needs to be nullptr before notification as during it
  // states are obtained. ATK_STATE_HIGHLIGHTED depends on highlighted_obj_
  // itself
  auto highlighted_obj = highlighted_obj_;
  highlighted_obj_ = nullptr;
  if (focus_root) {
    auto root = GetRoot();
    if (root)
      SetFocus(*root);
  }
  if (highlighted_obj->GetAtkObject())
    atk_object_notify_state_change(highlighted_obj->GetAtkObject(),
                                   ATK_STATE_HIGHLIGHTED, false);
}

#if defined(TIZEN_ATK_FEATURE_VD)
void BrowserAccessibilityManagerEfl::NotifyLiveRegionChanged(
    BrowserAccessibilityEfl* obj) {
  std::string text_to_emit;
  int member_of_id = 0;

  if (!obj || !obj->GetAtkObject() || obj->GetBoolAttribute(ui::AX_ATTR_BUSY) ||
      obj->GetBoolAttribute(ui::AX_ATTR_CONTAINER_LIVE_BUSY) ||
      ui::AX_ROLE_LINE_BREAK == obj->GetRole())
    return;

  if (!obj->GetBoolAttribute(ui::AX_ATTR_LIVE_ATOMIC) &&
      obj->GetBoolAttribute(ui::AX_ATTR_CONTAINER_LIVE_ATOMIC)) {
    if (obj->GetIntAttribute(ui::AX_ATTR_MEMBER_OF_ID, &member_of_id) &&
        GetFromID(member_of_id) &&
        ToBrowserAccessibilityEfl(GetFromID(member_of_id)))

      text_to_emit = ToBrowserAccessibilityEfl(GetFromID(member_of_id))
                         ->GetSubTreeSpeechContent();
  } else {
    text_to_emit = obj->GetSpeechContent();
  }

  if (text_to_emit.empty())
    return;

  g_signal_emit_by_name(obj->GetAtkObject(), "text-insert", 0,
                        text_to_emit.length(), text_to_emit.data());
}
#endif

}  // namespace content
