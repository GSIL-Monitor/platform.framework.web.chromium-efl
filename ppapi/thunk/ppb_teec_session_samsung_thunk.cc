// Copyright (c) 2017 Samsung Electronics. All rights reserved.

#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/samsung/ppb_teec_session_samsung.h"
#include "ppapi/shared_impl/resource_tracker.h"
#include "ppapi/shared_impl/tracked_callback.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppapi_thunk_export.h"
#include "ppapi/thunk/ppb_teec_session_api.h"

namespace ppapi {
namespace thunk {

namespace {

PP_Resource Create(PP_Resource context) {
  VLOG(4) << "PPB_TEECSession_Samsung::Create()";
  ppapi::ProxyAutoLock lock;

  EnterResourceNoLock<PPB_TEECContext_API> enter_teec_context(context, true);
  if (enter_teec_context.failed())
    return 0;

  PP_Instance instance = enter_teec_context.resource()->pp_instance();
  EnterResourceCreationNoLock enter(instance);
  if (enter.failed())
    return 0;

  return enter.functions()->CreateTEECSession(instance, context);
}

PP_Bool IsTEECSession(PP_Resource session) {
  VLOG(4) << "PPB_TEECSession_Samsung::IsTEECContext()";
  EnterResource<PPB_TEECSession_API> enter(session, true);
  return PP_FromBool(enter.succeeded());
}

int32_t Open(PP_Resource resource,
             const struct PP_TEEC_UUID* destination,
             uint32_t connection_method,
             uint32_t connection_data_size,
             const void* connection_data,
             struct PP_TEEC_Operation* operation,
             struct PP_TEEC_Result* result,
             struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_TEECSession_Samsung::Open()";
  EnterResource<PPB_TEECSession_API> enter(resource, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(enter.object()->Open(
      destination, connection_method, connection_data_size, connection_data,
      operation, result, enter.callback()));
}

int32_t InvokeCommand(PP_Resource resource,
                      uint32_t commandID,
                      struct PP_TEEC_Operation* operation,
                      struct PP_TEEC_Result* result,
                      struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_TEECSession_Samsung::InvokeCommand()";
  EnterResource<PPB_TEECSession_API> enter(resource, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(enter.object()->InvokeCommand(
      commandID, operation, result, enter.callback()));
}

int32_t RequestCancellation(PP_Resource resource,
                            const struct PP_TEEC_Operation* operation,
                            struct PP_CompletionCallback callback) {
  EnterResource<PPB_TEECSession_API> enter(resource, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(
      enter.object()->RequestCancellation(operation, enter.callback()));
}

const PPB_TEECSession_Samsung_1_0 g_ppb_teecsession_samsung_thunk_1_0 = {
    &Create, &IsTEECSession, &Open, &InvokeCommand, &RequestCancellation};

}  // namespace

PPAPI_THUNK_EXPORT const PPB_TEECSession_Samsung_1_0*
GetPPB_TEECSession_Samsung_1_0_Thunk() {
  return &g_ppb_teecsession_samsung_thunk_1_0;
}

}  // namespace thunk
}  // namespace ppapi
