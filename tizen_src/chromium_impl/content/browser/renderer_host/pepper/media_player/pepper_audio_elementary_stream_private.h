// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_AUDIO_ELEMENTARY_STREAM_PRIVATE_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_AUDIO_ELEMENTARY_STREAM_PRIVATE_H_

#include "content/browser/renderer_host/pepper/media_player/pepper_elementary_stream_private.h"
#include "ppapi/shared_impl/media_player/media_codecs_configs.h"

#include "base/callback.h"

namespace content {

// Platform depended part of Audio Elementary Stream PPAPI implementation,
// see ppapi/api/samsung/ppb_media_data_source_samsung.idl.

class PepperAudioElementaryStreamPrivate
    : public PepperElementaryStreamPrivate {
 public:
  virtual ~PepperAudioElementaryStreamPrivate() {}

  virtual void InitializeDone(PP_StreamInitializationMode mode,
                              const ppapi::AudioCodecConfig&,
                              const base::Callback<void(int32_t)>&) = 0;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_AUDIO_ELEMENTARY_STREAM_PRIVATE_H_
