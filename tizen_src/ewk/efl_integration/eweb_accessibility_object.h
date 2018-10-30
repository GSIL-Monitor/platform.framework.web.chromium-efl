// Copyright 2015-17 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWEB_ACCESSIBILITY_OBJECT_H
#define EWEB_ACCESSIBILITY_OBJECT_H

#include <Evas.h>
#include <atk/atk.h>

#include "eweb_view.h"

namespace content {
class BrowserAccessibility;
class BrowserAccessibilityManager;
class BrowserAccessibilityAuraLinux;
class BrowserAccessibilityManagerAuraLinux;
class BrowserAccessibilityManager;
}  // namespace content

G_BEGIN_DECLS

#define WEB_VIEW_ACCESSIBILITY_OBJECT_TYPE \
  (web_view_accessibility_object_get_type())
#define WEB_VIEW_ACCESSIBILITY_OBJECT(obj)                               \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), WEB_VIEW_ACCESSIBILITY_OBJECT_TYPE, \
                              WebViewAccessibilityObject))
#define WEB_VIEW_ACCESSIBILITY_OBJECT_CLASS(klass)                      \
  (G_TYPE_CHECK_CLASS_CAST((klass), WEB_VIEW_ACCESSIBILITY_OBJECT_TYPE, \
                           WebViewAccessibilityObjectClass))
#define IS_WEB_VIEW_ACCESSIBILITY_OBJECT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), WEB_VIEW_ACCESSIBILITY_OBJECT_TYPE))
#define IS_WEB_VIEW_ACCESSIBILITY_OBJECT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), WEB_VIEW_ACCESSIBILITY_OBJECT_TYPE))
#define WEB_VIEW_ACCESSIBILITY_OBJECT_GET_CLASS(obj)                    \
  (G_TYPE_INSTANCE_GET_CLASS((obj), WEB_VIEW_ACCESSIBILITY_OBJECT_TYPE, \
                             WebViewAccessibilityObjectClass))

typedef struct _WebViewAccessibilityObject WebViewAccessibilityObject;
typedef struct _WebViewAccessibilityObjectClass WebViewAccessibilityObjectClass;
typedef struct _WebViewAccessibilityObjectPrivate
    WebViewAccessibilityObjectPrivate;

struct _WebViewAccessibilityObject {
  AtkPlug parent;
  _WebViewAccessibilityObjectPrivate* priv;
};

struct _WebViewAccessibilityObjectClass {
  AtkPlugClass parent_class;
};

GType web_view_accessibility_object_get_type();

G_END_DECLS

// This class represents EWebView root accessibility object.
// EWebView root accessibility object is an entry point for browser
// to have top-down navigation through accessibility objects.
class EWebAccessibilityObject {
 public:
  EWebAccessibilityObject(EWebView*);
  ~EWebAccessibilityObject();

  WebViewAccessibilityObject* rootObject() const;

  AtkObject* RefAccessibleAtPoint(gint x, gint y, AtkCoordType coord_type);
  void GetScreenGeometry(int* x, int* y, int* width, int* height) const;
  void GetWindowGeometry(int* x, int* y, int* width, int* height) const;
  gint GetIndexInParent() const;
  gint GetNChildren() const;
  AtkObject* RefChild(gint index) const;
  AtkStateSet* RefStateSet() const;
  content::BrowserAccessibilityManager* GetBrowserAccessibilityManager() const;

 private:
  static void NativeViewShowCallback(void*, Evas*, Evas_Object*, void*);
  static void NativeViewHideCallback(void*, Evas*, Evas_Object*, void*);
  AtkObject* GetChild() const;

  EWebView* ewebview_;
  WebViewAccessibilityObject* root_object_;
};

#endif  // EWEB_ACCESSIBILITY_OBJECT_H
