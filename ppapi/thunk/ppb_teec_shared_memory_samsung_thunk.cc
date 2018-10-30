// Copyright (c) 2017 Samsung Electronics. All rights reserved.

#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/samsung/ppb_teec_shared_memory_samsung.h"
#include "ppapi/shared_impl/resource_tracker.h"
#include "ppapi/shared_impl/tracked_callback.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppapi_thunk_export.h"
#include "ppapi/thunk/ppb_teec_shared_memory_api.h"

namespace ppapi {
namespace thunk {

namespace {

PP_Resource Create(PP_Resource context, uint32_t flags) {
  VLOG(4) << "PPB_TEECSharedMemory_Samsung::Create()";
  ppapi::ProxyAutoLock lock;
  EnterResourceNoLock<PPB_TEECContext_API> enter_teec_context(context, true);
  if (enter_teec_context.failed())
    return 0;

  PP_Instance instance = enter_teec_context.resource()->pp_instance();
  EnterResourceCreationNoLock enter(instance);
  if (enter.failed())
    return 0;

  return enter.functions()->CreateTEECSharedMemory(instance, context, flags);
}

PP_Bool IsTEECSharedMemory(PP_Resource controller) {
  VLOG(4) << "PPB_TEECSharedMemory_Samsung::IsTEECSharedMemory()";
  EnterResource<PPB_TEECSharedMemory_API> enter(controller, true);
  return PP_FromBool(enter.succeeded());
}

int32_t Register(PP_Resource shared_mem,
                 const void* buffer,
                 uint32_t size,
                 struct PP_TEEC_Result* result,
                 struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_TEECSharedMemory_Samsung::Register()";
  EnterResource<PPB_TEECSharedMemory_API> enter(shared_mem, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(
      enter.object()->Register(buffer, size, result, enter.callback()));
}

int32_t Allocate(PP_Resource shared_mem,
                 uint32_t size,
                 struct PP_TEEC_Result* result,
                 struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_TEECSharedMemory_Samsung::Allocate()";
  EnterResource<PPB_TEECSharedMemory_API> enter(shared_mem, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(
      enter.object()->Allocate(size, result, enter.callback()));
}

void* Map(PP_Resource shared_mem) {
  VLOG(4) << "PPB_TEECSharedMemory_Samsung::Map()";
  EnterResource<PPB_TEECSharedMemory_API> enter(shared_mem, true);
  if (enter.failed())
    return NULL;
  return enter.object()->Map();
}

const PPB_TEECSharedMemory_Samsung_1_0
    g_ppb_teecsharedmemory_samsung_thunk_1_0 = {&Create, &IsTEECSharedMemory,
                                                &Register, &Allocate, &Map};

}  // namespace

PPAPI_THUNK_EXPORT const PPB_TEECSharedMemory_Samsung_1_0*
GetPPB_TEECSharedMemory_Samsung_1_0_Thunk() {
  return &g_ppb_teecsharedmemory_samsung_thunk_1_0;
}

}  // namespace thunk
}  // namespace ppapi
