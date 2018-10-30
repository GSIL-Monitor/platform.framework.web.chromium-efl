/* Copyright (c) 2017 Samsung Electronics. All rights reserved.
 */

/* From samsung/ppb_teec_session_samsung.idl,
 *   modified Fri Apr 28 10:00:37 2017.
 */

#ifndef PPAPI_C_SAMSUNG_PPB_TEEC_SESSION_SAMSUNG_H_
#define PPAPI_C_SAMSUNG_PPB_TEEC_SESSION_SAMSUNG_H_

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_stdint.h"
#include "ppapi/c/samsung/pp_teec_samsung.h"

#define PPB_TEECSESSION_SAMSUNG_INTERFACE_1_0 "PPB_TEECSession_Samsung;1.0"
#define PPB_TEECSESSION_SAMSUNG_INTERFACE PPB_TEECSESSION_SAMSUNG_INTERFACE_1_0

/**
 * @file
 */

/**
 * @addtogroup Interfaces
 * @{
 */
struct PPB_TEECSession_Samsung_1_0 {
  /**
   * Creates a new TEEC session resource.
   *
   * @param[in] teec_context A <code>PP_Resource</code> identifying
   * the parent TEEC_Context for the TEE session.
   *
   * @return A <code>PP_Resource</code> corresponding to a TEEC Session if
   * successful or 0 otherwise.
   */
  PP_Resource (*Create)(PP_Resource teec_context);
  /**
   * Determines if the given resource is a TEEC Session.
   *
   * @param[in] resource A <code>PP_Resource</code> identifying a resource.
   *
   * @return <code>PP_TRUE</code> if the resource is a
   * <code>PPB_TEECSession_Samsung</code>, <code>PP_FALSE</code>
   * if the resource is invalid or some other type.
   */
  PP_Bool (*IsTEECSession)(PP_Resource resource);
  /**
   * This function opens a new Session between the Client Application and
   * the specified Trusted Application.
   */
  int32_t (*Open)(PP_Resource resource,
                  const struct PP_TEEC_UUID* destination,
                  uint32_t connection_method,
                  uint32_t connection_data_size,
                  const void* connection_data,
                  struct PP_TEEC_Operation* operation,
                  struct PP_TEEC_Result* result,
                  struct PP_CompletionCallback callback);
  /**
   * This function invokes a Command within the specified Session.
   */
  int32_t (*InvokeCommand)(PP_Resource resource,
                           uint32_t command_id,
                           struct PP_TEEC_Operation* operation,
                           struct PP_TEEC_Result* result,
                           struct PP_CompletionCallback callback);
  /**
   * This function requests the cancellation of a pending open Session
   * operation or a Command invocation operation.
   */
  int32_t (*RequestCancellation)(PP_Resource resource,
                                 const struct PP_TEEC_Operation* operation,
                                 struct PP_CompletionCallback callback);
};

typedef struct PPB_TEECSession_Samsung_1_0 PPB_TEECSession_Samsung;
/**
 * @}
 */

#endif /* PPAPI_C_SAMSUNG_PPB_TEEC_SESSION_SAMSUNG_H_ */
