// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/browser_pepper_host_factory_efl.h"

#include <memory>

#include "build/build_config.h"
#include "content/public/browser/browser_ppapi_host.h"
#include "ppapi/host/ppapi_host.h"
#include "ppapi/host/resource_host.h"
#include "ppapi/proxy/ppapi_message_utils.h"
#include "ppapi/proxy/ppapi_messages.h"

#if defined(TIZEN_PEPPER_PLAYER)
#include "content/browser/renderer_host/pepper/media_player/pepper_es_data_source_host.h"
#include "content/browser/renderer_host/pepper/media_player/pepper_media_player_host.h"
#include "content/browser/renderer_host/pepper/media_player/pepper_url_data_source_host.h"
#include "ppapi/c/samsung/pp_media_player_samsung.h"
#include "ppapi/proxy/ppapi_message_utils.h"
#endif

#if defined(TIZEN_PEPPER_EXTENSIONS)
#include "content/browser/renderer_host/pepper/pepper_extension_system_host.h"
#include "content/browser/renderer_host/pepper/pepper_remote_controller_host.h"
#endif

#if defined(TIZEN_PEPPER_TEEC)
#include "content/browser/renderer_host/pepper/pepper_teec_context_host.h"
#include "content/browser/renderer_host/pepper/pepper_teec_session_host.h"
#include "content/browser/renderer_host/pepper/pepper_teec_shared_memory_host.h"
#endif

using ppapi::host::ResourceHost;
using ppapi::UnpackMessage;

BrowserPepperHostFactoryEfl::BrowserPepperHostFactoryEfl(
    content::BrowserPpapiHost* host)
    : host_(host) {}

BrowserPepperHostFactoryEfl::~BrowserPepperHostFactoryEfl() {}

std::unique_ptr<ResourceHost> BrowserPepperHostFactoryEfl::CreateResourceHost(
    ppapi::host::PpapiHost* host,
    PP_Resource resource,
    PP_Instance instance,
    const IPC::Message& message) {
  DCHECK(host == host_->GetPpapiHost());

  switch (message.type()) {
#if defined(TIZEN_PEPPER_PLAYER)
    case PpapiHostMsg_MediaPlayer_Create::ID: {
      PP_MediaPlayerMode player_mode;
      if (!UnpackMessage<PpapiHostMsg_MediaPlayer_Create>(message,
                                                          &player_mode)) {
        NOTREACHED();
        return std::unique_ptr<ResourceHost>();
      }
      return std::unique_ptr<ResourceHost>(new content::PepperMediaPlayerHost(
          host_, instance, resource, player_mode));
    }
    case PpapiHostMsg_URLDataSource_Create::ID: {
      std::string url;
      if (!UnpackMessage<PpapiHostMsg_URLDataSource_Create>(message, &url)) {
        LOG(ERROR) << "Cannot unpack URLDataSource url argument.";
        return std::unique_ptr<ResourceHost>();
      }
      return std::unique_ptr<ResourceHost>(
          new content::PepperURLDataSourceHost(host_, instance, resource, url));
    }
    case PpapiHostMsg_ESDataSource_Create::ID: {
      return std::unique_ptr<ResourceHost>(
          new content::PepperESDataSourceHost(host_, instance, resource));
    }
#endif  // defined(TIZEN_PEPPER_PLAYER)
#if defined(TIZEN_PEPPER_EXTENSIONS)
    case PpapiHostMsg_ExtensionSystem_Create::ID: {
      return std::unique_ptr<ResourceHost>(
          new content::PepperExtensionSystemHost(host_, instance, resource));
    }
    case PpapiHostMsg_RemoteController_Create::ID: {
      return std::unique_ptr<ResourceHost>(
          new content::PepperRemoteControllerHost(host_, instance, resource));
    }
#endif  // defined(TIZEN_PEPPER_EXTENSIONS)
#if defined(TIZEN_PEPPER_TEEC)
    case PpapiHostMsg_TEECContext_Create::ID: {
      return std::unique_ptr<ResourceHost>(
          new content::PepperTEECContextHost(host_, instance, resource));
    }
    case PpapiHostMsg_TEECSession_Create::ID: {
      PP_Resource context;
      if (!ppapi::UnpackMessage<PpapiHostMsg_TEECSession_Create>(message,
                                                                 &context)) {
        LOG(ERROR) << "Cannot unpack TEECSession context argument.";
        return std::unique_ptr<ResourceHost>();
      }

      return std::unique_ptr<ResourceHost>(new content::PepperTEECSessionHost(
          host_, instance, resource, context));
    }
    case PpapiHostMsg_TEECSharedMemory_Create::ID: {
      PP_Resource context;
      uint32_t flags;
      if (!ppapi::UnpackMessage<PpapiHostMsg_TEECSharedMemory_Create>(
              message, &context, &flags)) {
        LOG(ERROR) << "Cannot unpack TEECSession arguments.";
        return std::unique_ptr<ResourceHost>();
      }

      return std::unique_ptr<ResourceHost>(
          new content::PepperTEECSharedMemoryHost(host_, instance, resource,
                                                  context, flags));
    }
#endif  // defined(TIZEN_PEPPER_TEEC)
  }

  return std::unique_ptr<ResourceHost>();
}
