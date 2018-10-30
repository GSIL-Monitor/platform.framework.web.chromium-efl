// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_audio_elementary_stream_private_tizen.h"

namespace content {

// static
scoped_refptr<PepperAudioElementaryStreamPrivateTizen>
PepperAudioElementaryStreamPrivateTizen::Create(
    PepperESDataSourcePrivate* source) {
  return scoped_refptr<PepperAudioElementaryStreamPrivateTizen>{
      new PepperAudioElementaryStreamPrivateTizen(source)};
}

PepperAudioElementaryStreamPrivateTizen::
    PepperAudioElementaryStreamPrivateTizen(PepperESDataSourcePrivate* source)
    : PepperElementaryStreamPrivateTizen<PepperAudioElementaryStreamPrivate,
                                         PepperTizenAudioStreamTraits>(source) {
}

PepperAudioElementaryStreamPrivateTizen::
    ~PepperAudioElementaryStreamPrivateTizen() {}

}  // namespace content
