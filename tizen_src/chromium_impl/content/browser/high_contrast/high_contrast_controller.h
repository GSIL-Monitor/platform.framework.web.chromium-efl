// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HIGH_CONTRAST_CONTROLLER_H_
#define HIGH_CONTRAST_CONTROLLER_H_

#include <stdio.h>
#include <vconf/vconf.h>
#include <string>
#include <vector>

#include "url/gurl.h"

namespace content {
class RenderWidgetHostViewEfl;

class HighContrastController {
 public:
  void Initialize();

  void SetUseHighContrast(bool use_high_contrast);
  bool UseHighContrast() { return use_high_contrast_; }

  void SetHighContrastEnabled(bool high_contrast_enabled);
  bool HighContrastEnabled() { return high_contrast_enabled_; }

  static HighContrastController& Shared();
  bool CanApplyHighContrast(const GURL& url);
  void GetAllWindows();
  // called when VCONFKEY_HIGH_CONTRAST changed
  static void HandleDidChangedHighContrast(keynode_t* keynode, void* data);

  bool AddForbiddenUrl(const char*);
  bool IsForbiddenUrl(const std::string& url);
  bool RemoveForbiddenUrl(const char*);

 private:
  HighContrastController();
  ~HighContrastController();

  bool use_high_contrast_;
  bool high_contrast_enabled_;
  std::vector<std::string> forbidden_urls_;
};

}  // namespace content

#endif  // HIGH_CONTRAST_CONTROLLER_H_
