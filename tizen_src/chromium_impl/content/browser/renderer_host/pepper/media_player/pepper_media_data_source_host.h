// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_MEDIA_DATA_SOURCE_HOST_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_MEDIA_DATA_SOURCE_HOST_H_

#include "ppapi/host/resource_host.h"

namespace content {

class PepperMediaDataSourcePrivate;
class BrowserPpapiHost;

class PepperMediaDataSourceHost : public ppapi::host::ResourceHost {
 public:
  PepperMediaDataSourceHost(BrowserPpapiHost*, PP_Instance, PP_Resource);
  virtual ~PepperMediaDataSourceHost();

  bool IsMediaDataSourceHost() override;

  virtual PepperMediaDataSourcePrivate* MediaDataSourcePrivate() = 0;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_MEDIA_DATA_SOURCE_HOST_H_
