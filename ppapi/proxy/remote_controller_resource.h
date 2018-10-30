// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_REMOTE_CONTROLLER_RESOURCE_H_
#define PPAPI_PROXY_REMOTE_CONTROLLER_RESOURCE_H_

#include <string>

#include "ppapi/c/samsung/ppb_remote_controller_samsung.h"
#include "ppapi/proxy/plugin_resource.h"
#include "ppapi/proxy/ppapi_proxy_export.h"
#include "ppapi/shared_impl/scoped_pp_var.h"
#include "ppapi/thunk/ppb_remote_controller_samsung_api.h"

namespace ppapi {
namespace proxy {

class RemoteControllerResource : public PluginResource,
                                 public thunk::PPB_RemoteController_API {
 public:
  RemoteControllerResource(Connection connection, PP_Instance instance);

  ~RemoteControllerResource() override;

  // Resource implementation.
  thunk::PPB_RemoteController_API* AsPPB_RemoteController_API() override;

  // PPB_RemoteController_API implementation.
  int32_t RegisterKeys(PP_Instance instance,
                       uint32_t key_count,
                       const char* keys[]) override;
  int32_t UnRegisterKeys(PP_Instance instance,
                         uint32_t key_count,
                         const char* keys[]) override;
};

}  // namespace proxy
}  // namespace ppapi

#endif  // PPAPI_PROXY_REMOTE_CONTROLLER_RESOURCE_H_
