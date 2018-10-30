// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// From samsung/ppb_remote_controller_samsung.idl modified Tue Jul 12 15:37:43
// 2016.

#include "ppapi/c/pp_errors.h"
#include "ppapi/c/samsung/ppb_remote_controller_samsung.h"
#include "ppapi/shared_impl/tracked_callback.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppapi_thunk_export.h"
#include "ppapi/thunk/ppb_remote_controller_samsung_api.h"

namespace ppapi {
namespace thunk {

namespace {

int32_t RegisterKeys(PP_Instance instance,
                     uint32_t key_count,
                     const char* keys[]) {
  VLOG(4) << "PPB_RemoteController_Samsung::RegisterKeys()";
  EnterInstanceAPI<PPB_RemoteController_API> enter(instance);
  if (enter.failed())
    return enter.retval();
  return enter.functions()->RegisterKeys(instance, key_count, keys);
}

int32_t UnRegisterKeys(PP_Instance instance,
                       uint32_t key_count,
                       const char* keys[]) {
  VLOG(4) << "PPB_RemoteController_Samsung::UnRegisterKeys()";
  EnterInstanceAPI<PPB_RemoteController_API> enter(instance);
  if (enter.failed())
    return enter.retval();
  return enter.functions()->UnRegisterKeys(instance, key_count, keys);
}

const PPB_RemoteController_Samsung_0_1
    g_ppb_remotecontroller_samsung_thunk_0_1 = {&RegisterKeys, &UnRegisterKeys};

}  // namespace

PPAPI_THUNK_EXPORT const PPB_RemoteController_Samsung_0_1*
GetPPB_RemoteController_Samsung_0_1_Thunk() {
  return &g_ppb_remotecontroller_samsung_thunk_0_1;
}

}  // namespace thunk
}  // namespace ppapi
