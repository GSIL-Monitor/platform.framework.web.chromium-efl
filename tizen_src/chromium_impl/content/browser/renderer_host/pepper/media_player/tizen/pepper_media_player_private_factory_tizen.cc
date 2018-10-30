// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_media_player_private_factory_tizen.h"

#include "base/lazy_instance.h"
#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_es_data_source_private_tizen.h"
#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_media_player_private_tizen.h"
#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_mmplayer_adapter.h"
#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_url_data_source_private_tizen.h"
#if defined(TIZEN_PEPPER_PLAYER_D2TV)
#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_smplayer_adapter.h"
#endif

namespace content {

// static
// Implementation of factory method from PepperMediaPlayerPrivateFactory
// followed pattern from PluginService and PluginServiceImpl
PepperMediaPlayerPrivateFactory&
PepperMediaPlayerPrivateFactory::GetInstance() {
  auto instance = PepperMediaPlayerPrivateFactoryTizen::GetInstance();
  DCHECK(instance != nullptr);
  return *instance;
}

// static
PepperMediaPlayerPrivateFactoryTizen*
PepperMediaPlayerPrivateFactoryTizen::GetInstance() {
  return base::Singleton<PepperMediaPlayerPrivateFactoryTizen>::get();
}

scoped_refptr<PepperMediaPlayerPrivate>
PepperMediaPlayerPrivateFactoryTizen::CreateMediaPlayer(
    PP_MediaPlayerMode player_mode) {
  switch (player_mode) {
    case PP_MEDIAPLAYERMODE_DEFAULT:
      return PepperMediaPlayerPrivateTizen::Create(
          std::unique_ptr<PepperPlayerAdapterInterface>(
              new PepperMMPlayerAdapter()));
    case PP_MEDIAPLAYERMODE_D2TV:
#if defined(TIZEN_PEPPER_PLAYER_D2TV)
      return PepperMediaPlayerPrivateTizen::Create(
          std::unique_ptr<PepperPlayerAdapterInterface>(
              new PepperSMPlayerAdapter()));
#else
      NOTREACHED();
      return nullptr;
#endif
    default:
      NOTREACHED();
      return nullptr;
  }
}

scoped_refptr<PepperURLDataSourcePrivate>
PepperMediaPlayerPrivateFactoryTizen::CreateURLDataSource(
    const std::string& url) {
  return PepperURLDataSourcePrivateTizen::Create(url);
}

scoped_refptr<PepperESDataSourcePrivate>
PepperMediaPlayerPrivateFactoryTizen::CreateESDataSource() {
  return PepperESDataSourcePrivateTizen::Create();
}

PepperMediaPlayerPrivateFactoryTizen::PepperMediaPlayerPrivateFactoryTizen() =
    default;

PepperMediaPlayerPrivateFactoryTizen::~PepperMediaPlayerPrivateFactoryTizen() =
    default;

}  // namespace content
