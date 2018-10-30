// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/media_player/pepper_media_data_source_host.h"

#include "content/public/browser/browser_ppapi_host.h"

namespace content {

PepperMediaDataSourceHost::PepperMediaDataSourceHost(BrowserPpapiHost* host,
                                                     PP_Instance instance,
                                                     PP_Resource resource)
    : ResourceHost(host->GetPpapiHost(), instance, resource) {}

PepperMediaDataSourceHost::~PepperMediaDataSourceHost() {}

bool PepperMediaDataSourceHost::IsMediaDataSourceHost() {
  return true;
}

}  // namespace content
