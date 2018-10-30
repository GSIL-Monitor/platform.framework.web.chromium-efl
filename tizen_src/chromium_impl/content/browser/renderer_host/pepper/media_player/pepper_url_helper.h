// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_URL_HELPER_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_URL_HELPER_H_

#include <string>

#include "ppapi/c/pp_instance.h"
#include "url/gurl.h"

namespace content {

class BrowserPpapiHost;

namespace PepperUrlHelper {
GURL ResolveURL(BrowserPpapiHost* host,
                PP_Instance instance,
                const std::string& url);
}
}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_URL_HELPER_H_
