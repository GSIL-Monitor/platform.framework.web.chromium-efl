// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_ES_DATA_SOURCE_PRIVATE_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_ES_DATA_SOURCE_PRIVATE_H_

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "content/browser/renderer_host/pepper/media_player/pepper_media_data_source_private.h"
#include "ppapi/c/samsung/pp_media_player_samsung.h"

namespace content {

class PepperAudioElementaryStreamPrivate;
class PepperVideoElementaryStreamPrivate;

// Platform depended part of ES Data Source PPAPI implementation,
// see ppapi/api/samsung/ppb_media_data_source_samsung.idl.

class PepperESDataSourcePrivate : public PepperMediaDataSourcePrivate {
 public:
  virtual ~PepperESDataSourcePrivate() {}

  virtual void AddAudioStream(
      const base::Callback<
          void(int32_t,
               scoped_refptr<PepperAudioElementaryStreamPrivate>)>&) = 0;
  virtual void AddVideoStream(
      const base::Callback<
          void(int32_t,
               scoped_refptr<PepperVideoElementaryStreamPrivate>)>&) = 0;

  virtual void SetDuration(PP_TimeDelta duration,
                           const base::Callback<void(int32_t)>&) = 0;
  virtual void SetEndOfStream(const base::Callback<void(int32_t)>&) = 0;
};

}  // namespace  content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_ES_DATA_SOURCE_PRIVATE_H_
