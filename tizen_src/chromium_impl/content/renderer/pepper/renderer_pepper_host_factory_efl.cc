// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/pepper/renderer_pepper_host_factory_efl.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "components/nacl/common/features.h"
#include "content/public/renderer/renderer_ppapi_host.h"
#include "content/renderer/pepper/pepper_uma_host.h"
#include "ppapi/proxy/ppapi_messages.h"

#if defined(TIZEN_PEPPER_PLAYER)
#include "content/renderer/pepper/pepper_media_player_renderer_host.h"
#include "ppapi/c/samsung/pp_media_player_samsung.h"
#include "ppapi/proxy/ppapi_message_utils.h"
#endif

using ppapi::host::ResourceHost;

#if defined(TIZEN_PEPPER_PLAYER)
using ppapi::UnpackMessage;
#endif

RendererPepperHostFactoryEfl::RendererPepperHostFactoryEfl(
    content::RendererPpapiHost* host)
    : host_(host) {}

RendererPepperHostFactoryEfl::~RendererPepperHostFactoryEfl() {}

std::unique_ptr<ResourceHost>
RendererPepperHostFactoryEfl::CreateResourceHost(
    ppapi::host::PpapiHost* host,
    PP_Resource resource,
    PP_Instance instance,
    const IPC::Message& message) {
  DCHECK_EQ(host_->GetPpapiHost(), host);

  // Make sure the plugin is giving us a valid instance for this resource.
  if (!host_->IsValidInstance(instance))
    return {};

  // Permissions for the following interfaces will be checked at the
  // time of the corresponding instance's method calls.  Currently these
  // interfaces are available only for whitelisted apps which may not have
  // access to the other private interfaces.
  switch (message.type()) {
#if BUILDFLAG(ENABLE_NACL)
    case PpapiHostMsg_UMA_Create::ID: {
      return std::unique_ptr<ResourceHost>(
          new PepperUMAHost(host_, instance, resource));
    }
#endif  // BUILDFLAG(ENABLE_NACL)
#if defined(TIZEN_PEPPER_PLAYER)
    case PpapiHostMsg_MediaPlayer_Create::ID: {
      PP_MediaPlayerMode player_mode;
      if (!UnpackMessage<PpapiHostMsg_MediaPlayer_Create>(message,
                                                          &player_mode)) {
        NOTREACHED();
        return {};
      }
#if !defined(TIZEN_PEPPER_PLAYER_D2TV)
      // Prevent creation of a D2TV media player resource if this mode is not
      // compiled in.
      if (player_mode == PP_MEDIAPLAYERMODE_D2TV)
        return {};
#endif
      return base::MakeUnique<content::PepperMediaPlayerRendererHost>(
          host_, instance, resource, player_mode);
    }
#endif  // defined(TIZEN_PEPPER_PLAYER)
  }

  return {};
}

