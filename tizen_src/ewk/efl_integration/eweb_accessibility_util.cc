// Copyright 2015-17 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "eweb_accessibility_util.h"

#include <atk-bridge.h>
#include <atk/atk.h>

#include "common/web_contents_utils.h"
#include "content/browser/accessibility/browser_accessibility_state_impl.h"
#include "content/browser/web_contents/web_contents_impl_efl.h"
#include "eweb_accessibility.h"
#include "eweb_view.h"
#include "tizen/system_info.h"

#include <vconf.h>

using web_contents_utils::WebViewFromWebContents;

G_BEGIN_DECLS

#define EWEB_ACCESSIBILITY_ROOT_TYPE (eweb_accessibility_root_get_type())
#define EWEB_ACCESSIBILITY_ROOT(obj)                               \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), EWEB_ACCESSIBILITY_ROOT_TYPE, \
                              EWebAccessibilityRoot))
#define EWEB_ACCESSIBILITY_ROOT_CLASS(klass)                      \
  (G_TYPE_CHECK_CLASS_CAST((klass), EWEB_ACCESSIBILITY_ROOT_TYPE, \
                           EWebAccessibilityRootClass))
#define IS_EWEB_ACCESSIBILITY_ROOT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), EWEB_ACCESSIBILITY_ROOT_TYPE))
#define IS_EWEB_ACCESSIBILITY_ROOT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), EWEB_ACCESSIBILITY_ROOT_TYPE))
#define EWEB_ACCESSIBILITY_ROOT_GET_CLASS(obj)                    \
  (G_TYPE_INSTANCE_GET_CLASS((obj), EWEB_ACCESSIBILITY_ROOT_TYPE, \
                             EWebAccessibilityRootClass))

typedef struct _EWebAccessibilityRoot EWebAccessibilityRoot;
typedef struct _EWebAccessibilityRootClass EWebAccessibilityRootClass;
typedef struct _EWebAccessibilityRootPrivate EWebAccessibilityRootPrivate;

struct _EWebAccessibilityRootPrivate {};

struct _EWebAccessibilityRoot {
  AtkObject parent;
  _EWebAccessibilityRootPrivate* priv;
};

struct _EWebAccessibilityRootClass {
  AtkObjectClass parentClass;
};

GType eweb_accessibility_root_get_type();

G_END_DECLS

G_DEFINE_TYPE_WITH_PRIVATE(EWebAccessibilityRoot,
                           eweb_accessibility_root,
                           ATK_TYPE_OBJECT);

static void eweb_accessibility_root_init(
    EWebAccessibilityRoot* accessible_root) {
  accessible_root->priv = static_cast<_EWebAccessibilityRootPrivate*>(
      eweb_accessibility_root_get_instance_private(accessible_root));
}

static void eweb_accessibility_root_class_init(
    EWebAccessibilityRootClass* klass) {}

static AtkObject* eweb_util_get_root() {
  if (IsMobileProfile()) {
    for (const auto& wc : content::WebContentsImpl::GetAllWebContents()) {
      if (EWebView* ewebview = WebViewFromWebContents(wc))
        return ewebview->eweb_accessibility().RootObject();
    }
    return nullptr;
  }

  return ATK_OBJECT(g_object_new(EWEB_ACCESSIBILITY_ROOT_TYPE, nullptr));
}

static G_CONST_RETURN gchar* eweb_util_get_toolkit_name(void) {
  return "Chromium-EFL";
}

static G_CONST_RETURN gchar* eweb_util_get_toolkit_version(void) {
  return "1.0";
}

static void PropertyChangedCb(keynode_t* keynodeName, void* data) {
  EWebAccessibilityUtil* obj = static_cast<EWebAccessibilityUtil*>(data);
  if (!obj) {
    LOG(ERROR) << "obj is NULL";
    return;
  }

  int result = 0;
  if (vconf_get_bool(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS, &result) != 0) {
    LOG(ERROR)
        << "Could not read VCONFKEY_SETAPPL_ACCESSIBILITY_TTS key value.";
    return;
  }

#if defined(TIZEN_ATK_FEATURE_VD)
  obj->ToggleBrowserAccessibility(result && !obj->IsDeactivatedByApp(),
                                  nullptr);
#else
  obj->ToggleBrowserAccessibility(result, nullptr);
#endif
}

EWebAccessibilityUtil* EWebAccessibilityUtil::GetInstance() {
  return base::Singleton<EWebAccessibilityUtil>::get();
}

EWebAccessibilityUtil::EWebAccessibilityUtil()
    : atk_bridge_initialized_(false)
#if defined(TIZEN_ATK_FEATURE_VD)
    , deactivated_by_app_(false)
#endif
{
  if (getuid() == 0) {
    return;
  }

  AtkUtilClass* atkUtilClass = ATK_UTIL_CLASS(g_type_class_ref(ATK_TYPE_UTIL));
  atkUtilClass->get_toolkit_name = eweb_util_get_toolkit_name;
  atkUtilClass->get_toolkit_version = eweb_util_get_toolkit_version;
  atkUtilClass->get_root = eweb_util_get_root;

  vconf_notify_key_changed(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS,
                           PropertyChangedCb, this);
}

EWebAccessibilityUtil::~EWebAccessibilityUtil() {
  vconf_ignore_key_changed(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS,
                           PropertyChangedCb);
  CleanAtkBridgeAdaptor();
}

void EWebAccessibilityUtil::NotifyAccessibilityStatus(bool mode) {
  for (const auto& wc : content::WebContentsImpl::GetAllWebContents()) {
    if (EWebView* ewebview = WebViewFromWebContents(wc))
      ewebview->eweb_accessibility().NotifyAccessibilityStatus(mode);
  }
}

void EWebAccessibilityUtil::ToggleBrowserAccessibility(bool mode,
                                                       EWebView* ewebview) {
  content::BrowserAccessibilityStateImpl* state =
      content::BrowserAccessibilityStateImpl::GetInstance();
  if (mode) {
    InitAtkBridgeAdaptor();
    state->EnableAccessibility();
  }

  if (ewebview)
    ewebview->eweb_accessibility().NotifyAccessibilityStatus(mode);
  else
    NotifyAccessibilityStatus(mode);

  if (!mode) {
    state->DisableAccessibility();
    CleanAtkBridgeAdaptor();
  }
}

void EWebAccessibilityUtil::InitAtkBridgeAdaptor() {
  if (!atk_bridge_initialized_) {
    if (!atk_bridge_adaptor_init(nullptr, nullptr))
      atk_bridge_initialized_ = true;
    else
      LOG(ERROR) << "atk_bridge_adaptor_init failed.";
  }
}

void EWebAccessibilityUtil::CleanAtkBridgeAdaptor() {
  if (atk_bridge_initialized_) {
    atk_bridge_adaptor_cleanup();
    atk_bridge_initialized_ = false;
  }
}

#if defined(TIZEN_ATK_FEATURE_VD)
void EWebAccessibilityUtil::Deactivate(bool deactivated) {
  if (deactivated_by_app_ == deactivated)
    return;

  deactivated_by_app_ = deactivated;

  int result = 0;
  if (vconf_get_bool(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS, &result) != 0) {
    LOG(ERROR)
        << "Could not read VCONFKEY_SETAPPL_ACCESSIBILITY_TTS key value.";
    return;
  }

  if (result)
    ToggleBrowserAccessibility(result && !deactivated_by_app_, nullptr);
}
#endif