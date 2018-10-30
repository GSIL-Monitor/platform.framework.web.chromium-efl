// Copyright (c) 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_WEBUI_VERSION_UI_EFL_H_
#define BROWSER_WEBUI_VERSION_UI_EFL_H_

#include "base/macros.h"
#include "content/public/browser/web_ui_controller.h"

// The WebUI handler for chrome://version.
class VersionUIEfl : public content::WebUIController {
 public:
  explicit VersionUIEfl(content::WebUI* web_ui);
  ~VersionUIEfl() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(VersionUIEfl);
};

#endif  // BROWSER_WEBUI_VERSION_UI_EFL_H_
