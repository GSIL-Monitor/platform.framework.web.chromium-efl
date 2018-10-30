// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/high_contrast/high_contrast_controller.h"
#include "content/browser/renderer_host/render_widget_host_view_efl.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_iterator.h"

#define VCONFKEY_HIGH_CONTRAST "db/menu/system/accessibility/highcontrast"

namespace content {
HighContrastController::HighContrastController()
    : use_high_contrast_(false), high_contrast_enabled_(false) {
  Initialize();
}

HighContrastController::~HighContrastController() {
  vconf_ignore_key_changed(VCONFKEY_HIGH_CONTRAST,
                           HandleDidChangedHighContrast);
}

void HighContrastController::Initialize() {
  // register callback
  vconf_notify_key_changed(VCONFKEY_HIGH_CONTRAST, HandleDidChangedHighContrast,
                           this);
}

void HighContrastController::SetUseHighContrast(bool use_high_contrast) {
  if (use_high_contrast == use_high_contrast_)
    return;

  use_high_contrast_ = use_high_contrast;

  int active = 0;
  vconf_get_int(VCONFKEY_HIGH_CONTRAST, &active);
  high_contrast_enabled_ = active ? true : false;
}

void HighContrastController::SetHighContrastEnabled(
    bool high_contrast_enabled) {
  high_contrast_enabled_ = high_contrast_enabled;
}

HighContrastController& HighContrastController::Shared() {
  static HighContrastController& high_contrast_controller =
      *new HighContrastController;
  return high_contrast_controller;
}

void HighContrastController::HandleDidChangedHighContrast(keynode_t* keynode,
                                                          void* data) {
  HighContrastController* high_contrast_controller =
      static_cast<HighContrastController*>(data);
  if (!high_contrast_controller)
    return;
  int active = 0;
  vconf_get_int(VCONFKEY_HIGH_CONTRAST, &active);
  high_contrast_controller->high_contrast_enabled_ = active ? true : false;
  high_contrast_controller->GetAllWindows();
}

bool HighContrastController::CanApplyHighContrast(const GURL& url) {
  if (high_contrast_enabled_ && use_high_contrast_ &&
      !IsForbiddenUrl(url.possibly_invalid_spec()))
    return true;
  return false;
}

void HighContrastController::GetAllWindows() {
  std::unique_ptr<RenderWidgetHostIterator> widgets(
      RenderWidgetHost::GetRenderWidgetHosts());
  while (RenderWidgetHost* widget = widgets->GetNextHost()) {
    if (widget->GetView()) {
      RenderWidgetHostViewEfl* rwhv =
          static_cast<RenderWidgetHostViewEfl*>(widget->GetView());
      if (rwhv && rwhv->web_contents()) {
        rwhv->SetLayerInverted(
            CanApplyHighContrast(rwhv->web_contents()->GetVisibleURL()));
      }
    }
  }
}

bool HighContrastController::AddForbiddenUrl(const char* url) {
  for (auto it = forbidden_urls_.begin(); it != forbidden_urls_.end(); ++it) {
    if (std::string::npos != (*it).find(url))
      return false;
  }

  forbidden_urls_.push_back(url);
  return true;
}

bool HighContrastController::IsForbiddenUrl(const std::string& url) {
  for (auto it = forbidden_urls_.begin(); it != forbidden_urls_.end(); ++it) {
    if (std::string::npos != url.find(*it))
      return true;
  }

  return false;
}

bool HighContrastController::RemoveForbiddenUrl(const char* url) {
  bool is_url_exist = false;
  for (auto it = forbidden_urls_.begin(); it != forbidden_urls_.end();) {
    if (std::string::npos != (*it).find(url)) {
      it = forbidden_urls_.erase(it);
      is_url_exist = true;
    } else {
      ++it;
    }
  }

  return is_url_exist;
}

}  // namespace content
