// Copyright 2015-17 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "eweb_accessibility_object.h"

#include "content/browser/accessibility/browser_accessibility.h"
#include "content/browser/accessibility/browser_accessibility_auralinux.h"
#include "content/browser/accessibility/browser_accessibility_efl.h"
#include "content/browser/accessibility/browser_accessibility_manager_auralinux.h"
#include "content/browser/accessibility/browser_accessibility_manager_efl.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/common/content_export.h"
#include "tizen/system_info.h"

static const char* const kWebViewAccessibilityObjectName = "eweb accessibility";
static const char* const kWebViewAccessibilityObjectDescription =
    "chromium-efl accessibility for webview";

struct _WebViewAccessibilityObjectPrivate {
  EWebAccessibilityObject* m_object;
};

G_DEFINE_TYPE_WITH_PRIVATE(WebViewAccessibilityObject,
                           web_view_accessibility_object,
                           ATK_TYPE_PLUG)

static void web_view_accessibility_object_init(
    WebViewAccessibilityObject* accessible) {
  accessible->priv = static_cast<_WebViewAccessibilityObjectPrivate*>(
      web_view_accessibility_object_get_instance_private(accessible));
}

static void web_view_accessibility_object_initialize(AtkObject* atk_object,
                                                     gpointer data) {
  if (ATK_OBJECT_CLASS(web_view_accessibility_object_parent_class)
          ->initialize) {
    ATK_OBJECT_CLASS(web_view_accessibility_object_parent_class)
        ->initialize(atk_object, data);
  }

  WEB_VIEW_ACCESSIBILITY_OBJECT(atk_object)->priv->m_object =
      reinterpret_cast<EWebAccessibilityObject*>(data);
}

static void web_view_accessibility_object_finalize(GObject* atk_object) {
  G_OBJECT_CLASS(web_view_accessibility_object_parent_class)
      ->finalize(atk_object);
}

static EWebAccessibilityObject* ToEWebAccessibilityObject(
    AtkObject* atk_object) {
  if (!atk_object) {
    LOG(ERROR) << "atk_object is null";
    return nullptr;
  }

  if (!IS_WEB_VIEW_ACCESSIBILITY_OBJECT(atk_object)) {
    LOG(ERROR) << "atk_object is not webview accessibility object";
    return nullptr;
  }

  WebViewAccessibilityObject* ax_object =
      WEB_VIEW_ACCESSIBILITY_OBJECT(atk_object);

  return ax_object->priv->m_object;
}

static gint web_view_accessibility_object_get_index_in_parent(
    AtkObject* atk_object) {
  g_return_val_if_fail(IS_WEB_VIEW_ACCESSIBILITY_OBJECT(atk_object), -1);
  auto obj = ToEWebAccessibilityObject(atk_object);
  return obj->GetIndexInParent();
}

static gint web_view_accessibility_object_get_n_children(
    AtkObject* atk_object) {
  g_return_val_if_fail(IS_WEB_VIEW_ACCESSIBILITY_OBJECT(atk_object), -1);
  auto obj = ToEWebAccessibilityObject(atk_object);
  return obj->GetNChildren();
}

static AtkObject* web_view_accessibility_object_ref_child(AtkObject* atk_object,
                                                          gint index) {
  g_return_val_if_fail(IS_WEB_VIEW_ACCESSIBILITY_OBJECT(atk_object), nullptr);
  auto obj = ToEWebAccessibilityObject(atk_object);
  return obj->RefChild(index);
}

static const gchar* web_view_accessibility_object_get_name(
    AtkObject* atk_object) {
  return kWebViewAccessibilityObjectName;
}

static const gchar* web_view_accessibility_object_get_description(
    AtkObject* atk_object) {
  return kWebViewAccessibilityObjectDescription;
}

static AtkStateSet* web_view_accessibility_object_ref_state_set(
    AtkObject* atk_object) {
  g_return_val_if_fail(IS_WEB_VIEW_ACCESSIBILITY_OBJECT(atk_object), nullptr);
  auto obj = ToEWebAccessibilityObject(atk_object);
  return obj->RefStateSet();
}

static void web_view_accessibility_object_class_init(
    WebViewAccessibilityObjectClass* klass) {
  GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
  AtkObjectClass* atkObjectClass = ATK_OBJECT_CLASS(klass);

  gobject_class->finalize = web_view_accessibility_object_finalize;
  atkObjectClass->initialize = web_view_accessibility_object_initialize;
  atkObjectClass->get_index_in_parent =
      web_view_accessibility_object_get_index_in_parent;
  atkObjectClass->get_n_children = web_view_accessibility_object_get_n_children;
  atkObjectClass->ref_child = web_view_accessibility_object_ref_child;
  atkObjectClass->get_name = web_view_accessibility_object_get_name;
  atkObjectClass->get_description =
      web_view_accessibility_object_get_description;
  atkObjectClass->ref_state_set = web_view_accessibility_object_ref_state_set;
}

static AtkObject* web_view_accessibility_object_ref_accessible_at_point(
    AtkComponent* component,
    gint x,
    gint y,
    AtkCoordType coord_type) {
  auto obj = ToEWebAccessibilityObject(ATK_OBJECT(component));
  return obj->RefAccessibleAtPoint(x, y, coord_type);
}

static void web_view_accessibility_object_get_extents(AtkComponent* component,
                                                      gint* x,
                                                      gint* y,
                                                      gint* width,
                                                      gint* height,
                                                      AtkCoordType coord_type) {
  auto obj = ToEWebAccessibilityObject(ATK_OBJECT(component));
  if (ATK_XY_SCREEN) {
    obj->GetScreenGeometry(x, y, width, height);
  } else {  // ATK_XY_WINDOW
    obj->GetWindowGeometry(x, y, width, height);
  }
}

static void web_view_accessibility_object_get_position(
    AtkComponent* component,
    gint* x,
    gint* y,
    AtkCoordType coord_type) {
  auto obj = ToEWebAccessibilityObject(ATK_OBJECT(component));
  if (ATK_XY_SCREEN) {
    obj->GetScreenGeometry(x, y, nullptr, nullptr);
  } else {  // ATK_XY_WINDOW
    obj->GetWindowGeometry(x, y, nullptr, nullptr);
  }
}

static void web_view_accessibility_object_get_size(AtkComponent* component,
                                                   gint* width,
                                                   gint* height) {
  auto obj = ToEWebAccessibilityObject(ATK_OBJECT(component));
  obj->GetWindowGeometry(nullptr, nullptr, width, height);
}

EWebAccessibilityObject::EWebAccessibilityObject(EWebView* ewebview)
    : ewebview_(ewebview) {
  evas_object_event_callback_add(ewebview_->native_view(), EVAS_CALLBACK_SHOW,
                                 NativeViewShowCallback, this);
  evas_object_event_callback_add(ewebview_->native_view(), EVAS_CALLBACK_HIDE,
                                 NativeViewHideCallback, this);
  root_object_ = WEB_VIEW_ACCESSIBILITY_OBJECT(
      g_object_new(WEB_VIEW_ACCESSIBILITY_OBJECT_TYPE, nullptr));
  AtkObject* object = ATK_OBJECT(root_object_);
  atk_object_initialize(object, this);

  AtkComponent* component = ATK_COMPONENT(root_object_);
  AtkComponentIface* iface = ATK_COMPONENT_GET_IFACE(component);
  iface->ref_accessible_at_point =
      web_view_accessibility_object_ref_accessible_at_point;
  iface->get_extents = web_view_accessibility_object_get_extents;
  iface->get_position = web_view_accessibility_object_get_position;
  iface->get_size = web_view_accessibility_object_get_size;

  AtkObject* ewebview_child = GetChild();
  if (!ewebview_child) {
    LOG(ERROR)
        << "Can't set parent for EWebView's child atk object - child not found";
    return;
  }

  if (IsMobileProfile()) {
    auto accessibility_manager = GetBrowserAccessibilityManager();
    if (!accessibility_manager) {
      LOG(ERROR) << "Unable to get accessibility manager";
      return;
    }
    content::BrowserAccessibilityManagerAuraLinux*
        accessibility_manager_auralinux =
            accessibility_manager->ToBrowserAccessibilityManagerAuraLinux();
    accessibility_manager_auralinux->SetParentObject(object);
  }

  atk_object_set_parent(ewebview_child, object);
}

EWebAccessibilityObject::~EWebAccessibilityObject() {
  auto ewebview_child = GetChild();
  if (ewebview_child)
    atk_object_set_parent(ewebview_child, nullptr);

  if (IsMobileProfile()) {
    auto accessibility_manager = GetBrowserAccessibilityManager();
    if (accessibility_manager) {
      auto accessibility_manager_efl =
          accessibility_manager->ToBrowserAccessibilityManagerEfl();
      accessibility_manager_efl->SetParentObject(nullptr);
      accessibility_manager_efl->InvalidateHighlighted(true);
    }
  }

  evas_object_event_callback_del(ewebview_->native_view(), EVAS_CALLBACK_SHOW,
                                 NativeViewShowCallback);
  evas_object_event_callback_del(ewebview_->native_view(), EVAS_CALLBACK_HIDE,
                                 NativeViewHideCallback);
  g_object_unref(root_object_);
}

// static
void EWebAccessibilityObject::NativeViewShowCallback(void* data,
                                                     Evas*,
                                                     Evas_Object*,
                                                     void*) {
  auto thiz = static_cast<EWebAccessibilityObject*>(data);
  atk_object_notify_state_change(ATK_OBJECT(thiz->root_object_),
                                 ATK_STATE_SHOWING, TRUE);
}
// static
void EWebAccessibilityObject::NativeViewHideCallback(void* data,
                                                     Evas*,
                                                     Evas_Object*,
                                                     void*) {
  auto thiz = static_cast<EWebAccessibilityObject*>(data);
  atk_object_notify_state_change(ATK_OBJECT(thiz->root_object_),
                                 ATK_STATE_SHOWING, FALSE);
}

content::BrowserAccessibilityManager*
EWebAccessibilityObject::GetBrowserAccessibilityManager() const {
  auto frame = static_cast<content::RenderFrameHostImpl*>(
      ewebview_->web_contents().GetMainFrame());
  if (!frame) {
    LOG(ERROR) << "Unable to get render frame host impl";
    return nullptr;
  }
  return frame->GetOrCreateBrowserAccessibilityManager();
}

AtkObject* EWebAccessibilityObject::RefAccessibleAtPoint(
    gint x,
    gint y,
    AtkCoordType coord_type) {
  auto child = GetChild();
  if (!child) {
    LOG(ERROR) << "Child was not found";
    return nullptr;
  }
  return atk_component_ref_accessible_at_point(ATK_COMPONENT(child), x, y,
                                               coord_type);
}

void EWebAccessibilityObject::GetScreenGeometry(int* x,
                                                int* y,
                                                int* width,
                                                int* height) const {
  auto ee =
      ecore_evas_ecore_evas_get(evas_object_evas_get(ewebview_->native_view()));
  ecore_evas_screen_geometry_get(ee, x, y, width, height);
}

void EWebAccessibilityObject::GetWindowGeometry(int* x,
                                                int* y,
                                                int* width,
                                                int* height) const {
  evas_object_geometry_get(ewebview_->native_view(), x, y, width, height);
}

gint EWebAccessibilityObject::GetIndexInParent() const {
  // AtkPlug is the only child of AtkSocket.
  return 0;
}

gint EWebAccessibilityObject::GetNChildren() const {
  // One and only child of AtkPlug is the browser root accessibility object.
  auto child = GetChild();
  if (!child) {
    LOG(ERROR) << "Child was not found";
    return 0;
  }
  return 1;
}

AtkObject* EWebAccessibilityObject::RefChild(gint index) const {
  if (index != 0) {
    LOG(ERROR) << "Invalid index passed: " << index;
    return nullptr;
  }
  auto child = GetChild();
  if (!child) {
    LOG(ERROR) << "Child was not found";
    return nullptr;
  }
  g_object_ref(child);
  return child;
}

AtkStateSet* EWebAccessibilityObject::RefStateSet() const {
  auto atk_state_set = atk_state_set_new();
  if (evas_object_visible_get(ewebview_->native_view()))
    atk_state_set_add_state(atk_state_set, ATK_STATE_SHOWING);
  return atk_state_set;
}

AtkObject* EWebAccessibilityObject::GetChild() const {
  auto accessibility_manager = GetBrowserAccessibilityManager();
  if (!accessibility_manager) {
    LOG(ERROR) << "Unable to get accessibility manager";
    return nullptr;
  }

  content::BrowserAccessibility* obj = accessibility_manager->GetRoot();
  content::BrowserAccessibilityAuraLinux* auralinux_accessibility_root_object =
      ToBrowserAccessibilityAuraLinux(obj);
  if (!auralinux_accessibility_root_object) {
    LOG(ERROR) << "Unable to get auralinux accessibility root object";
    return nullptr;
  }
  AtkObject* auralinux_atk_root_object =
      auralinux_accessibility_root_object->GetAtkObject();
  if (!auralinux_atk_root_object)
    LOG(ERROR) << "Unable to get auralinux atk root object";
  return auralinux_atk_root_object;
}

WebViewAccessibilityObject* EWebAccessibilityObject::rootObject() const {
  return root_object_;
}
