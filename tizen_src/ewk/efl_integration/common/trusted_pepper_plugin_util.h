// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TIZEN_SRC_EWK_EFL_INTEGRATION_COMMON_TRUSTED_PEPPER_PLUGIN_UTIL_H_
#define TIZEN_SRC_EWK_EFL_INTEGRATION_COMMON_TRUSTED_PEPPER_PLUGIN_UTIL_H_

#include "public/ewk_context_product.h"

namespace pepper {

// Updates list of pepper plugins held by PluginServiceImpl.
// Call this function in browser process after embedder type has been
// properly initialized.
void UpdatePluginService(Ewk_Application_Type embedder_type);

// Update list of pepper plugins held by PepperPluginRegistry.
// Call this function in renderer process after embedder type has been
// properly initialized.
void UpdatePluginRegistry();

// Due to security reasons such trusted pepper plugins are disabled in
// WebBrowser embedder, unless explicitly allowed by command line switch.
bool AreTrustedPepperPluginsDisabled();
}  // namespace pepper

#endif  // TIZEN_SRC_EWK_EFL_INTEGRATION_COMMON_TRUSTED_PEPPER_PLUGIN_UTIL_H_
