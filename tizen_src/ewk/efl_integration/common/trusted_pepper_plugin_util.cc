// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/trusted_pepper_plugin_util.h"

#include <vector>

#include "base/command_line.h"
#include "common/application_type.h"
#include "common/content_switches_efl.h"
#include "common/trusted_pepper_plugin_info_cache.h"
#include "content/browser/plugin_service_impl.h"
#include "content/renderer/pepper/pepper_plugin_registry.h"

using content::PepperPluginInfo;
using content::PepperPluginRegistry;
using content::PluginServiceImpl;
using pepper::TrustedPepperPluginInfoCache;

namespace pepper {

void UpdatePluginService(Ewk_Application_Type embedder_type) {
  // Don't need to check kEnableTrustedPepperPlugins command line switch,
  // because in that case Trusted Pepper Plugins will be added to
  // PluginServiceImpl at its initialization by call to
  // ContentClientEfl::AddPepperPlugins.
  if (embedder_type == EWK_APPLICATION_TYPE_WEBBROWSER)
    return;

  std::vector<PepperPluginInfo> plugins;
  TrustedPepperPluginInfoCache::GetInstance()->GetPlugins(&plugins);
  PluginServiceImpl::GetInstance()->AddPepperPlugins(plugins);
}

void UpdatePluginRegistry() {
  // Don't need to check kEnableTrustedPepperPlugins command line switch,
  // because in that case Trusted Pepper Plugins will be added to
  // PepperPluginRegistry at its initialization by call to
  // ContentClientEfl::AddPepperPlugins.
  if (content::IsWebBrowser())
    return;

  std::vector<PepperPluginInfo> plugins;
  TrustedPepperPluginInfoCache::GetInstance()->GetPlugins(&plugins);
  PepperPluginRegistry::GetInstance()->AddOutOfProcessPlugins(plugins);
}

bool AreTrustedPepperPluginsDisabled() {
  auto command_line = base::CommandLine::ForCurrentProcess();
  return
#if defined(OS_TIZEN_TV_PRODUCT)
      content::IsWebBrowser() &&
#endif
      !command_line->HasSwitch(switches::kEnableTrustedPepperPlugins);
}
}  // namespace pepper
