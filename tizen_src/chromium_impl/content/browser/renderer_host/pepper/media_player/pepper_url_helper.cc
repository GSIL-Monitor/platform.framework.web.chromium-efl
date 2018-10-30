// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/media_player/pepper_url_helper.h"
#include "base/bind.h"
#include "base/files/file_path.h"
#include "content/public/browser/browser_ppapi_host.h"

#if defined(OS_TIZEN_TV_PRODUCT)
#include "base/command_line.h"
#include "common/application_type.h"
#include "ewk/efl_integration/common/content_switches_efl.h"
#include "wrt/wrt_dynamicplugin.h"
#endif

namespace content {
namespace PepperUrlHelper {
GURL ResolveURL(BrowserPpapiHost* host,
                PP_Instance instance,
                const std::string& url) {
  GURL resolved_url;
  if (url.front() != '.' && url.front() != '/') {
    resolved_url = GURL(url);
  } else {
    // For files this takes care for paths relative to parent (like ../ ) and
    // returns path starting with root / . Thx for this we don't need to check
    // this two cases below.
    resolved_url = host->GetDocumentURLForInstance(instance).Resolve(url);
  }

#if defined(OS_TIZEN_TV_PRODUCT)
  if (IsTIZENWRT() && resolved_url.SchemeIsFile() && resolved_url.is_valid()) {
    std::string tizen_app_id =
        base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
            switches::kTizenAppId);
    if (!tizen_app_id.empty()) {
      std::string old_url = host->GetDocumentURLForInstance(instance).path();
      std::string s_new_url;
      bool is_decrypted_file;
      // Get current document directory and resolve this to system path
      WrtDynamicPlugin::Get().ParseURL(
          &old_url, &s_new_url, tizen_app_id.c_str(), &is_decrypted_file);
      base::FilePath current_document(s_new_url);
      // resolved_url always starts with / (see above) , so we can do simple
      // concatenation
      resolved_url = GURL(current_document.DirName().value() +
                          resolved_url.PathForRequest());
    }
  }
#endif

  return resolved_url;
}
}  // namespace PepperUrlHelper
}  // namespace content
