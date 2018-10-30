// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_MEDIA_PLAYER_PRIVATE_FACTORY_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_MEDIA_PLAYER_PRIVATE_FACTORY_H_

#include <string>

#include "base/memory/ref_counted.h"
#include "ppapi/c/pp_rect.h"
#include "ppapi/c/samsung/pp_media_player_samsung.h"

namespace content {

class PepperMediaPlayerPrivate;
class PepperESDataSourcePrivate;
class PepperURLDataSourcePrivate;

// Interface which is implemented by all platform-specific Pepper Media Player
// factories. Factory is responsible for creating Media Player as well as
// Data Sources. Only one factory could be available at runtime.

class PepperMediaPlayerPrivateFactory {
 public:
  virtual ~PepperMediaPlayerPrivateFactory() {}

  // Implementation of this method will be provided by platform specific
  // implementation selected at compile time.
  static PepperMediaPlayerPrivateFactory& GetInstance();

  virtual scoped_refptr<PepperMediaPlayerPrivate> CreateMediaPlayer(
      PP_MediaPlayerMode player_mode) = 0;
  virtual scoped_refptr<PepperURLDataSourcePrivate> CreateURLDataSource(
      const std::string& url) = 0;
  virtual scoped_refptr<PepperESDataSourcePrivate> CreateESDataSource() = 0;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_MEDIA_PLAYER_PRIVATE_FACTORY_H_
