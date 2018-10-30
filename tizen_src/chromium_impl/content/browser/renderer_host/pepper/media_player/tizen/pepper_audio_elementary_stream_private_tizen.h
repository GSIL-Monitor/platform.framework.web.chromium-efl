// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_AUDIO_ELEMENTARY_STREAM_PRIVATE_TIZEN_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_AUDIO_ELEMENTARY_STREAM_PRIVATE_TIZEN_H_

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "content/browser/renderer_host/pepper/media_player/pepper_audio_elementary_stream_private.h"
#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_elementary_stream_private_tizen.h"
#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_player_adapter_interface.h"
#include "ppapi/c/samsung/pp_media_common_samsung.h"

namespace content {

struct PepperTizenAudioStreamTraits {
  typedef ppapi::AudioCodecConfig CodecConfig;
  typedef PepperAudioStreamInfo StreamInfo;
  static constexpr PP_ElementaryStream_Type_Samsung Type =
      PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_AUDIO;
  static int32_t SetStreamInfo(PepperPlayerAdapterInterface* player,
                               const StreamInfo& player_stream_info) {
    return player->SetAudioStreamInfo(player_stream_info);
  }
};

class PepperAudioElementaryStreamPrivateTizen
    : public PepperElementaryStreamPrivateTizen<
          PepperAudioElementaryStreamPrivate,
          PepperTizenAudioStreamTraits> {
 public:
  static scoped_refptr<PepperAudioElementaryStreamPrivateTizen> Create(
      PepperESDataSourcePrivate* es_data_source);
  ~PepperAudioElementaryStreamPrivateTizen() override;

 private:
  explicit PepperAudioElementaryStreamPrivateTizen(
      PepperESDataSourcePrivate* es_data_source);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_AUDIO_ELEMENTARY_STREAM_PRIVATE_TIZEN_H_
