// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_MEDIA_PLAYER_PRIVATE_FACTORY_TIZEN_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_MEDIA_PLAYER_PRIVATE_FACTORY_TIZEN_H_

#include <string>

#include "base/memory/singleton.h"
#include "content/browser/renderer_host/pepper/media_player/pepper_media_player_private_factory.h"

namespace content {

class PepperMediaPlayerPrivateFactoryTizen
    : public PepperMediaPlayerPrivateFactory {
 public:
  static PepperMediaPlayerPrivateFactoryTizen* GetInstance();

  scoped_refptr<PepperMediaPlayerPrivate> CreateMediaPlayer(
      PP_MediaPlayerMode player_mode) override;
  scoped_refptr<PepperURLDataSourcePrivate> CreateURLDataSource(
      const std::string& url) override;
  scoped_refptr<PepperESDataSourcePrivate> CreateESDataSource() override;

 private:
  friend struct base::DefaultSingletonTraits<
      PepperMediaPlayerPrivateFactoryTizen>;

  PepperMediaPlayerPrivateFactoryTizen();
  ~PepperMediaPlayerPrivateFactoryTizen() override;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_MEDIA_PLAYER_PRIVATE_FACTORY_TIZEN_H_
