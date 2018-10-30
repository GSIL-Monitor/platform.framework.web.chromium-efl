// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_REMOTE_CONTROLLER_WRT_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_REMOTE_CONTROLLER_WRT_H_

#include <memory>

#include "content/browser/renderer_host/pepper/pepper_remote_controller_host.h"

namespace content {
class BrowserPpapiHost;

// Factory method
std::unique_ptr<content::PepperRemoteControllerHost::PlatformDelegate>
CreateRemoteControllerWRT(PP_Instance instance,
                          content::BrowserPpapiHost* host);

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_REMOTE_CONTROLLER_WRT_H_
