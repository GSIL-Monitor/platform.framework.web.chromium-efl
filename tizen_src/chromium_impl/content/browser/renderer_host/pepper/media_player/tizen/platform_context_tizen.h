// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PLATFORM_CONTEXT_TIZEN_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PLATFORM_CONTEXT_TIZEN_H_

#include "base/single_thread_task_runner.h"
#include "content/browser/renderer_host/pepper/media_player/pepper_media_data_source_private.h"

namespace content {

class PepperTizenPlayerDispatcher;
class PepperTizenDRMManager;

class PepperMediaDataSourcePrivate::PlatformContext {
 public:
  PlatformContext();
  ~PlatformContext();

  void set_dispatcher(PepperTizenPlayerDispatcher* dispatcher) {
    dispatcher_ = dispatcher;
  }

  PepperTizenPlayerDispatcher* dispatcher() { return dispatcher_; }

  void set_drm_manager(PepperTizenDRMManager* drm_manager) {
    drm_manager_ = drm_manager;
  }

  PepperTizenDRMManager* drm_manager() { return drm_manager_; }

 private:
  // non owning pointer (managed by PepperMediaPlayerPrivateTizen)
  PepperTizenPlayerDispatcher* dispatcher_;
  // non owning pointer (managed by PepperMediaPlayerPrivateTizen)
  PepperTizenDRMManager* drm_manager_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PLATFORM_CONTEXT_TIZEN_H_
