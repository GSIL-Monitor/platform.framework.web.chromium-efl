/* Copyright (c) 2017 Samsung Electronics. All rights reserved.
 */

/* From samsung/ppb_teec_context_samsung.idl,
 *   modified Tue Apr 25 15:34:16 2017.
 */

#ifndef PPAPI_C_SAMSUNG_PPB_TEEC_CONTEXT_SAMSUNG_H_
#define PPAPI_C_SAMSUNG_PPB_TEEC_CONTEXT_SAMSUNG_H_

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_stdint.h"
#include "ppapi/c/samsung/pp_teec_samsung.h"

#define PPB_TEECCONTEXT_SAMSUNG_INTERFACE_1_0 "PPB_TEECContext_Samsung;1.0"
#define PPB_TEECCONTEXT_SAMSUNG_INTERFACE PPB_TEECCONTEXT_SAMSUNG_INTERFACE_1_0

/**
 * @file
 */

/**
 * @addtogroup Interfaces
 * @{
 */
struct PPB_TEECContext_Samsung_1_0 {
  /**
   * Creates a new TEEC context resource.
   *
   * @param[in] instance A <code>PP_Instance</code> identifying the instance
   * with the TEEC context.
   * @param[in] string describing the TEE to connect to.
   *
   * @return A <code>PP_Resource</code> corresponding to a TEEC Context if
   * successful or 0 otherwise.
   */
  PP_Resource (*Create)(PP_Instance instance);
  /**
   * Determines if the given resource is a TEEC context.
   *
   * @param[in] resource A <code>PP_Resource</code> identifying a resource.
   *
   * @return <code>PP_TRUE</code> if the resource is a
   * <code>PPB_TEECContext_Samsung</code>, <code>PP_FALSE</code>
   * if the resource is invalid or some other type.
   */
  PP_Bool (*IsTEECContext)(PP_Resource resource);
  /**
   * This function initializes a new TEEC Context, forming a connection
   * between this Client Application and the TEE identified by the string
   * identifier name.
   */
  int32_t (*Open)(PP_Resource resource,
                  const char* name,
                  struct PP_TEEC_Result* result,
                  struct PP_CompletionCallback callback);
};

typedef struct PPB_TEECContext_Samsung_1_0 PPB_TEECContext_Samsung;
/**
 * @}
 */

#endif /* PPAPI_C_SAMSUNG_PPB_TEEC_CONTEXT_SAMSUNG_H_ */
