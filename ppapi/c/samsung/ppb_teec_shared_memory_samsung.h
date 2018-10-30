/* Copyright (c) 2017 Samsung Electronics. All rights reserved.
 */

/* From samsung/ppb_teec_shared_memory_samsung.idl,
 *   modified Thu Apr 27 16:12:59 2017.
 */

#ifndef PPAPI_C_SAMSUNG_PPB_TEEC_SHARED_MEMORY_SAMSUNG_H_
#define PPAPI_C_SAMSUNG_PPB_TEEC_SHARED_MEMORY_SAMSUNG_H_

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_stdint.h"
#include "ppapi/c/samsung/pp_teec_samsung.h"

#define PPB_TEECSHAREDMEMORY_SAMSUNG_INTERFACE_1_0 \
  "PPB_TEECSharedMemory_Samsung;1.0"
#define PPB_TEECSHAREDMEMORY_SAMSUNG_INTERFACE \
  PPB_TEECSHAREDMEMORY_SAMSUNG_INTERFACE_1_0

/**
 * @file
 */

/**
 * @addtogroup Interfaces
 * @{
 */
/**
 * This type denotes a Shared Memory block which has either been registered
 * with the implementation or allocated by it.
 */
struct PPB_TEECSharedMemory_Samsung_1_0 {
  /**
   * Creates a new TEEC shared memory resource.
   *
   * @param[in] teec_context A <code>PP_Resource</code> identifying
   * the parent TEEC_Context for the TEEC shared memory.
   * @param[in] flags describing if memory can be used as input and/or output.
   * See the PP_TEEC_MemoryType type for more details.
   *
   * @return A <code>PP_Resource</code> corresponding to a TEEC Shared Memory
   * if successful or 0 otherwise.
   */
  PP_Resource (*Create)(PP_Resource context, uint32_t flags);
  /**
   * Determines if the given resource is a TEEC shared memory.
   *
   * @param[in] resource A <code>PP_Resource</code> identifying a resource.
   *
   * @return <code>PP_TRUE</code> if the resource is a
   * <code>PPB_TEECSharedMemory_Samsung</code>, <code>PP_FALSE</code>
   * if the resource is invalid or some other type.
   */
  PP_Bool (*IsTEECSharedMemory)(PP_Resource resource);
  /**
   * This function registers a block of existing Client Application memory as
   * a block of Shared Memory
   */
  int32_t (*Register)(PP_Resource shared_mem,
                      const void* buffer,
                      uint32_t size,
                      struct PP_TEEC_Result* result,
                      struct PP_CompletionCallback callback);
  /**
   * This function allocates a new block of memory as a block of Shared Memory
   */
  int32_t (*Allocate)(PP_Resource shared_mem,
                      uint32_t size,
                      struct PP_TEEC_Result* result,
                      struct PP_CompletionCallback callback);
  /**
   * Maps this shared memory into the plugin address space
   * and returns a pointer to the beginning of the data.
   */
  void* (*Map)(PP_Resource shared_mem);
};

typedef struct PPB_TEECSharedMemory_Samsung_1_0 PPB_TEECSharedMemory_Samsung;
/**
 * @}
 */

#endif /* PPAPI_C_SAMSUNG_PPB_TEEC_SHARED_MEMORY_SAMSUNG_H_ */
