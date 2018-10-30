// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_DRM_LISTENER_PRIVATE_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_DRM_LISTENER_PRIVATE_H_

#include <vector>

#include "ppapi/c/samsung/pp_media_player_samsung.h"

namespace content {

// Platform depended part of DRM Listener PPAPI implementation,
// see ppapi/api/samsung/ppp_media_player_samsung.idl.

class DRMListenerPrivate {
 public:
  virtual ~DRMListenerPrivate() {}

  virtual void OnInitDataLoaded(PP_MediaPlayerDRMType,
                                const std::vector<uint8_t>& data) = 0;
  virtual void OnLicenseRequest(const std::vector<uint8_t>& request) = 0;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_DRM_LISTENER_PRIVATE_H_
