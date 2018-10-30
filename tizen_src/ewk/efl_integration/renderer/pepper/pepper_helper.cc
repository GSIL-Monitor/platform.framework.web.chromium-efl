// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copied from chrome/renderer/pepper with minor modifications.

#include "renderer/pepper/pepper_helper.h"

#include "content/public/renderer/renderer_ppapi_host.h"
#include "ppapi/host/ppapi_host.h"
#include "renderer/pepper/pepper_shared_memory_message_filter.h"
#include "components/nacl/common/features.h"
#include "ppapi/features/features.h"

#if BUILDFLAG(ENABLE_NACL) || defined(TIZEN_PEPPER_PLAYER)
#include "content/renderer/pepper/renderer_pepper_host_factory_efl.h"
#endif

PepperHelper::PepperHelper(content::RenderFrame* render_frame)
    : RenderFrameObserver(render_frame) {}

PepperHelper::~PepperHelper() {}

void PepperHelper::DidCreatePepperPlugin(content::RendererPpapiHost* host) {
#if BUILDFLAG(ENABLE_NACL) || defined(TIZEN_PEPPER_PLAYER)
  host->GetPpapiHost()->AddHostFactoryFilter(
      std::unique_ptr<ppapi::host::HostFactory>(
          new RendererPepperHostFactoryEfl(host)));
#endif
  host->GetPpapiHost()->AddInstanceMessageFilter(
      std::unique_ptr<ppapi::host::InstanceMessageFilter>(
          new PepperSharedMemoryMessageFilter(host)));
}
