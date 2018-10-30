// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_ELEMENTARY_STREAM_LISTENER_PRIVATE_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_ELEMENTARY_STREAM_LISTENER_PRIVATE_H_

#include "ppapi/c/pp_time.h"

// Platform depended part of Buffering Listener PPAPI implementation,
// see ppapi/api/samsung/ppp_media_data_source_samsung.idl.

namespace content {

class ElementaryStreamListenerPrivate {
 public:
  virtual ~ElementaryStreamListenerPrivate() {}

  virtual void OnNeedData(int32_t bytes_max) = 0;
  virtual void OnEnoughData() = 0;
  virtual void OnSeekData(PP_TimeTicks new_position) = 0;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_ELEMENTARY_STREAM_LISTENER_PRIVATE_H_
