// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/remote_controller_resource.h"

#include <vector>

#include "base/logging.h"
#include "ppapi/proxy/plugin_dispatcher.h"
#include "ppapi/proxy/ppapi_messages.h"

namespace ppapi {
namespace proxy {

namespace {

int32_t CheckParameters(uint32_t key_count, const char* keys[]) {
  if (key_count == 0 || !keys) {
    LOG(ERROR) << "keys is " << keys << ", key_count= " << key_count;
    return PP_ERROR_BADARGUMENT;
  }

  for (uint32_t i = 0; i < key_count; ++i) {
    if (!keys[i]) {
      LOG(ERROR) << "keys[" << i << "] is NULL";
      return PP_ERROR_BADARGUMENT;
    }
  }
  return PP_OK;
}

}  // namespace

RemoteControllerResource::RemoteControllerResource(Connection connection,
                                                   PP_Instance instance)
    : PluginResource(connection, instance) {
  SendCreate(BROWSER, PpapiHostMsg_RemoteController_Create());
}

RemoteControllerResource::~RemoteControllerResource() = default;

// Resource implementation.
thunk::PPB_RemoteController_API*
RemoteControllerResource::AsPPB_RemoteController_API() {
  return this;
}

// PPB_RemoteController_API implementation.
int32_t RemoteControllerResource::RegisterKeys(PP_Instance instance,
                                               uint32_t key_count,
                                               const char* keys[]) {
  int32_t ret = CheckParameters(key_count, keys);
  if (ret != PP_OK)
    return ret;

  std::vector<std::string> keys_vec(keys, keys + key_count);
  return SyncCall<PpapiHostMsg_RemoteController_RegisterKeysReply>(
      BROWSER, PpapiHostMsg_RemoteController_RegisterKeys(keys_vec));
}

int32_t RemoteControllerResource::UnRegisterKeys(PP_Instance instance,
                                                 uint32_t key_count,
                                                 const char* keys[]) {
  int32_t ret = CheckParameters(key_count, keys);
  if (ret != PP_OK)
    return ret;

  std::vector<std::string> keys_vec(keys, keys + key_count);
  return SyncCall<PpapiHostMsg_RemoteController_UnRegisterKeysReply>(
      BROWSER, PpapiHostMsg_RemoteController_UnRegisterKeys(keys_vec));
}

}  // namespace proxy
}  // namespace ppapi
