// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_SUBTITLE_LISTENER_PRIVATE_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_SUBTITLE_LISTENER_PRIVATE_H_

#include <string>

#include "base/memory/ref_counted.h"
#include "ppapi/c/pp_size.h"
#include "ppapi/c/pp_time.h"

namespace content {

// Platform depended part of Subtitle PPAPI implementation,
// see ppapi/api/samsung/ppp_media_player_samsung.idl.

class SubtitleListenerPrivate {
 public:
  virtual ~SubtitleListenerPrivate() {}
  virtual void OnShowSubtitle(PP_TimeDelta, const std::string& utf8_text) = 0;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_SUBTITLE_LISTENER_PRIVATE_H_
