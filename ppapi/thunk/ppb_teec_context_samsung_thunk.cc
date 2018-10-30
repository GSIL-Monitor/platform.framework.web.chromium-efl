// Copyright (c) 2017 Samsung Electronics. All rights reserved.

#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/samsung/ppb_teec_context_samsung.h"
#include "ppapi/shared_impl/resource_tracker.h"
#include "ppapi/shared_impl/tracked_callback.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppapi_thunk_export.h"
#include "ppapi/thunk/ppb_teec_context_api.h"

namespace ppapi {
namespace thunk {

namespace {

PP_Resource Create(PP_Instance instance) {
  VLOG(4) << "PPB_TEECContext_Samsung::Create()";
  EnterResourceCreation enter(instance);
  if (enter.failed())
    return 0;
  return enter.functions()->CreateTEECContext(instance);
}

PP_Bool IsTEECContext(PP_Resource controller) {
  VLOG(4) << "PPB_TEECContext_Samsung::IsTEECContext()";
  EnterResource<PPB_TEECContext_API> enter(controller, true);
  return PP_FromBool(enter.succeeded());
}

int32_t Open(PP_Resource resource,
             const char* name,
             struct PP_TEEC_Result* result,
             struct PP_CompletionCallback callback) {
  EnterResource<PPB_TEECContext_API> enter(resource, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(enter.object()->Open(name, result, enter.callback()));
}

const PPB_TEECContext_Samsung_1_0 g_ppb_teeccontext_samsung_thunk_1_0 = {
    &Create, &IsTEECContext, &Open};

}  // namespace

PPAPI_THUNK_EXPORT const PPB_TEECContext_Samsung_1_0*
GetPPB_TEECContext_Samsung_1_0_Thunk() {
  return &g_ppb_teeccontext_samsung_thunk_1_0;
}

}  // namespace thunk
}  // namespace ppapi
