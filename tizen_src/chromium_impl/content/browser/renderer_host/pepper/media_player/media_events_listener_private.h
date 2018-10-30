// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_MEDIA_EVENTS_LISTENER_PRIVATE_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_MEDIA_EVENTS_LISTENER_PRIVATE_H_

#include "ppapi/c/pp_time.h"
#include "ppapi/c/samsung/pp_media_player_samsung.h"

#include "base/memory/ref_counted.h"

namespace content {

// Platform depended part of Media Events Listener PPAPI implementation,
// see ppapi/api/samsung/ppp_media_player_samsung.idl.

class MediaEventsListenerPrivate {
 public:
  virtual ~MediaEventsListenerPrivate() {}

  virtual void OnTimeUpdate(PP_TimeTicks) = 0;
  virtual void OnEnded() = 0;
  virtual void OnError(PP_MediaPlayerError) = 0;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_MEDIA_EVENTS_LISTENER_PRIVATE_H_
