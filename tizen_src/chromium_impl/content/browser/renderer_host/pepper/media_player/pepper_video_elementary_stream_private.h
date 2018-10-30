// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_VIDEO_ELEMENTARY_STREAM_PRIVATE_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_VIDEO_ELEMENTARY_STREAM_PRIVATE_H_

#include "content/browser/renderer_host/pepper/media_player/pepper_elementary_stream_private.h"
#include "ppapi/shared_impl/media_player/media_codecs_configs.h"

#include "base/callback.h"

namespace content {

// Platform depended part of Video Elementary Stream PPAPI implementation,
// see ppapi/api/samsung/ppb_media_data_source_samsung.idl.

class PepperVideoElementaryStreamPrivate
    : public PepperElementaryStreamPrivate {
 public:
  virtual ~PepperVideoElementaryStreamPrivate() {}

  virtual void InitializeDone(
      PP_StreamInitializationMode mode,
      const ppapi::VideoCodecConfig&,
      const base::Callback<void(int32_t)>& callback) = 0;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_VIDEO_ELEMENTARY_STREAM_PRIVATE_H_
