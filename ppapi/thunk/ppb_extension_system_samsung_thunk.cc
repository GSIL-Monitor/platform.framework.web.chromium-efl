// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/shared_impl/tracked_callback.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppb_instance_api.h"
#include "ppapi/thunk/resource_creation_api.h"
#include "ppapi/thunk/thunk.h"

#include "ppapi/c/samsung/ppb_extension_system_samsung.h"
#include "ppapi/thunk/ppb_extension_system_samsung_api.h"

namespace ppapi {
namespace thunk {

namespace {

PP_Var GetEmbedderName(PP_Instance instance) {
  EnterInstanceAPI<PPB_ExtensionSystem_API> enter(instance);
  if (enter.failed())
    return PP_MakeUndefined();
  return enter.functions()->GetEmbedderName();
}

PP_Var GetCurrentExtensionInfo(PP_Instance instance) {
  EnterInstanceAPI<PPB_ExtensionSystem_API> enter(instance);
  if (enter.failed())
    return PP_MakeUndefined();
  return enter.functions()->GetCurrentExtensionInfo();
}

int32_t GenericSyncCall(PP_Instance instance,
                        PP_Var operation_name,
                        PP_Var operation_data,
                        PP_Var* operation_result) {
  EnterInstanceAPI<PPB_ExtensionSystem_API> enter(instance);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(enter.functions()->GenericSyncCall(
      operation_name, operation_data, operation_result));
}

const PPB_ExtensionSystem_Samsung_0_1 g_ppb_extensionsystem_samsung_thunk_0_1 =
    {&GetEmbedderName, &GetCurrentExtensionInfo, &GenericSyncCall};

}  // namespace

const PPB_ExtensionSystem_Samsung_0_1*
GetPPB_ExtensionSystem_Samsung_0_1_Thunk() {
  return &g_ppb_extensionsystem_samsung_thunk_0_1;
}

}  // namespace thunk
}  // namespace ppapi
