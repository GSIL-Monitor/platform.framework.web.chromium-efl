// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_URL_DATA_SOURCE_PRIVATE_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_URL_DATA_SOURCE_PRIVATE_H_

#include <string>

#include "content/browser/renderer_host/pepper/media_player/pepper_media_data_source_private.h"
#include "ppapi/c/samsung/pp_media_player_samsung.h"

namespace content {

// Platform depended part of URL Data Source PPAPI implementation,
// see ppapi/api/samsung/ppb_media_data_source_samsung.idl.

class PepperURLDataSourcePrivate : public PepperMediaDataSourcePrivate {
 public:
  virtual ~PepperURLDataSourcePrivate() {}

  virtual void SetStreamingProperty(PP_StreamingProperty,
                                    const std::string&,
                                    const base::Callback<void(int32_t)>&) = 0;
  virtual void GetStreamingProperty(
      PP_StreamingProperty,
      const base::Callback<void(int32_t, const std::string&)>&) = 0;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_URL_DATA_SOURCE_PRIVATE_H_
