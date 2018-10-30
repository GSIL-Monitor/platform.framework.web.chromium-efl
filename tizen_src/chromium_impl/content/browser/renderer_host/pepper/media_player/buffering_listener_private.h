// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_BUFFERING_LISTENER_PRIVATE_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_BUFFERING_LISTENER_PRIVATE_H_

namespace content {

// Platform depended part of Buffering Listener PPAPI implementation,
// see ppapi/api/samsung/ppp_media_player_samsung.idl.

class BufferingListenerPrivate {
 public:
  virtual ~BufferingListenerPrivate() {}

  virtual void OnBufferingStart() = 0;
  virtual void OnBufferingProgress(int percent) = 0;
  virtual void OnBufferingComplete() = 0;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_BUFFERING_LISTENER_PRIVATE_H_
