/* Copyright (c) 2016 Samsung Electronics. All rights reserved.
 */

/* From samsung/ppb_extension_system_samsung.idl,
 *   modified Thu Feb 25 15:18:37 2016.
 */

#ifndef PPAPI_C_SAMSUNG_PPB_EXTENSION_SYSTEM_SAMSUNG_H_
#define PPAPI_C_SAMSUNG_PPB_EXTENSION_SYSTEM_SAMSUNG_H_

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_stdint.h"
#include "ppapi/c/pp_var.h"

#define PPB_EXTENSIONSYSTEM_SAMSUNG_INTERFACE_0_1 \
  "PPB_ExtensionSystem_Samsung;0.1"
#define PPB_EXTENSIONSYSTEM_SAMSUNG_INTERFACE \
  PPB_EXTENSIONSYSTEM_SAMSUNG_INTERFACE_0_1

/**
 * @file
 * This file defines the <code>PPB_ExtensionSystem_Samsung</code> interface.
 */

/**
 * @addtogroup Interfaces
 * @{
 */
/**
 * The PPB_ExtensionSystem_Samsung interface contains pointers to functions
 * related to the extension system. The extension system can be different for
 * each browser.
 */
struct PPB_ExtensionSystem_Samsung_0_1 {
  /**
   * GetEmbedderName() provides string with embedder name (embedder of current
   * extension).
   *
   * @param[in] instance A <code>PP_Instance</code> identifying one instance
   * of a module.
   *
   * @return A <code>PP_Var</code> with name of extension embedder.
   */
  struct PP_Var (*GetEmbedderName)(PP_Instance instance);
  /**
   * GetCurrentExtensionInfo() gets dictionary with information for current
   * extension. Keys and values of the dictionary are dependant on the
   * embedder, and they can differ between embedders. If current embedder does
   * not support extension system undefined value is returned.
   *
   * @param[in] instance A <code>PP_Instance</code> identifying one instance
   * of a module.
   *
   * @return A <code>PP_Var</code> with information of current extension.
   */
  struct PP_Var (*GetCurrentExtensionInfo)(PP_Instance instance);
  /**
   * GenericSyncCall() executes operation associated with the current
   * extension. The operation is synchronous and blocks the caller until
   * completes. See embedder documentation to know what operations are
   * possible.
   *
   * @param[in] instance A <code>PP_Instance</code> identifying one instance
   * of a module.
   * @param[in] operation_name A string with name of operation to execute.
   * @param[in] operation_argument A variable to be passed to embedder
   * @param[out] result A variable containing result of execution (embedder
   * defined).
   *
   * @return An int32_t containing an error code from <code>pp_errors.h</code>.
   */
  int32_t (*GenericSyncCall)(PP_Instance instance,
                             struct PP_Var operation_name,
                             struct PP_Var operation_argument,
                             struct PP_Var* result);
};

typedef struct PPB_ExtensionSystem_Samsung_0_1 PPB_ExtensionSystem_Samsung;
/**
 * @}
 */

#endif /* PPAPI_C_SAMSUNG_PPB_EXTENSION_SYSTEM_SAMSUNG_H_ */
