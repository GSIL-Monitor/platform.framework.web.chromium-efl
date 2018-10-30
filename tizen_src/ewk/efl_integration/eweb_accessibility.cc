// Copyright 2015-17 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "eweb_accessibility.h"

#include <atk-bridge.h>

#include "base/logging.h"
#include "content/browser/accessibility/browser_accessibility_manager_efl.h"
#include "content/browser/accessibility/browser_accessibility_state_impl.h"
#include "eweb_accessibility_util.h"
#include "eweb_view.h"
#include "tizen/system_info.h"

#if defined(TIZEN_ATK_FEATURE_VD)
#include "common/application_type.h"
#endif

static const char* const kPlugIdKey = "__PlugID";

EWebAccessibility::EWebAccessibility(EWebView* ewebview) : ewebview_(ewebview) {
  EWebAccessibilityUtil::GetInstance();
}

EWebAccessibility::~EWebAccessibility() {}

AtkObject* EWebAccessibility::RootObject() {
  if (!IsMobileProfile())
    return nullptr;

  if (!accessibility_object_)
    accessibility_object_.reset(new EWebAccessibilityObject(ewebview_));
  return ATK_OBJECT(accessibility_object_->rootObject());
}

void EWebAccessibility::OnFocusOut() {
  if (!IsMobileProfile())
    return;

  if (!accessibility_object_)
    return;
  auto manager = accessibility_object_->GetBrowserAccessibilityManager();
  if (!manager)
    return;
  manager->ToBrowserAccessibilityManagerEfl()->InvalidateHighlighted(true);
}

void EWebAccessibility::AddPlug() {
  if (!IsMobileProfile())
    return;

  if (!accessibility_object_)
    accessibility_object_.reset(new EWebAccessibilityObject(ewebview_));

  auto plug_id = atk_plug_get_id(ATK_PLUG(accessibility_object_->rootObject()));
  if (plug_id) {
    evas_object_data_set(ewebview_->evas_object(), kPlugIdKey, strdup(plug_id));
    LOG(INFO) << "Web accessibility address was set to " << plug_id;
  } else if (accessibility_object_) {
    accessibility_object_.reset();
    atk_bridge_adaptor_cleanup();
    LOG(ERROR) << "atk_plug_get_id failed.";
  }
}

void EWebAccessibility::RemovePlug() {
  if (!IsMobileProfile())
    return;

  auto plug_id = static_cast<char*>(
      evas_object_data_get(ewebview_->evas_object(), kPlugIdKey));
  if (plug_id) {
    evas_object_data_set(ewebview_->evas_object(), kPlugIdKey, nullptr);
    free(plug_id);
  }
  accessibility_object_.reset();
}

void EWebAccessibility::NotifyAccessibilityStatus(bool is_enabled) {
  if (IsMobileProfile())
    (is_enabled) ? AddPlug() : RemovePlug();

#if defined(TIZEN_ATK_FEATURE_VD)
  if (content::IsWebBrowser())
    ewebview_->UpdateSpatialNavigationStatus(is_enabled);
#else
  ewebview_->UpdateSpatialNavigationStatus(is_enabled);
#endif
}
