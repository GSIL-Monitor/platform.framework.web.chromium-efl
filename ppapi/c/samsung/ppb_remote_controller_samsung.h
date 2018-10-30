/* Copyright 2016 Samsung Electronics. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* From samsung/ppb_remote_controller_samsung.idl,
 *   modified Fri Oct  7 12:45:45 2016.
 */

#ifndef PPAPI_C_SAMSUNG_PPB_REMOTE_CONTROLLER_SAMSUNG_H_
#define PPAPI_C_SAMSUNG_PPB_REMOTE_CONTROLLER_SAMSUNG_H_

#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_stdint.h"

#define PPB_REMOTECONTROLLER_SAMSUNG_INTERFACE_0_1 \
  "PPB_RemoteController_Samsung;0.1"
#define PPB_REMOTECONTROLLER_SAMSUNG_INTERFACE \
  PPB_REMOTECONTROLLER_SAMSUNG_INTERFACE_0_1

/**
 * @file
 * This file defines <code>PPB_RemoteController_Samsung</code> interface,
 * which can be used for various actions related to TV Remote Controller like
 * registering for input Keys.
 *
 * See
 * <code>https://www.samsungdforum.com/TizenGuide/tizen231/index.html</code>
 * for description of Remote Controller and its keys.
 */

/**
 * @addtogroup Interfaces
 * @{
 */
/**
 * The PPB_RemoteControl_Samsung interface contains pointer to functions
 * related to Remote Controller.
 *
 * See
 * <code>https://www.samsungdforum.com/TizenGuide/tizen231/index.html</code>
 * for description of Remote Controller and its keys.
 */
struct PPB_RemoteController_Samsung_0_1 {
  /**
   * RegisterKeys() function registers given key arrays to be grabbed by
   * the application/widget containing pepper plugin calling this method.
   *
   * <strong>Note:</strong>
   * After registering for grabbing keys, events related to that key
   * will be delivered directly to the application/widget.
   *
   * <strong>Note:</strong>
   * For some embedders, we can`t tell if key that we try to register have
   * failed because it have been already registered. So if at least one key
   * have been successfully processed, we assume that other keys that failed,
   * have been already registered before this call.
   *
   * @param[in] instance A <code>PP_Instance</code> identifying instance
   * of the module
   * @param[in] key_count A number of keys which will be grabbed.
   * @param[in] keys An array containing list of keys which should be grabbed.
   *
   * @return An int32_t containing an error code from <code>pp_errors.h</code>.
   * Returns <code>PP_ERROR_BADARGUMENT</code> if <code>key_count</code> is
   * equal 0 or one of <code>keys</code> is not supported anymore.
   * Returns <code>PP_ERROR_NOTSUPPORTED</code> if currently launched embedder
   * doesn`t support key registering.
   * Returns <code>PP_ERROR_FAILED</code> if registering all <code>keys</code>
   * have failed.
   * Returns <code>PP_OK</code> if at lest one key from <code>keys</code> have
   * been registered.
   */
  int32_t (*RegisterKeys)(PP_Instance instance,
                          uint32_t key_count,
                          const char* keys[]);
  /**
   * UnregisterKeys() function unregisters given key arrays from being grabbed
   * by the application/widget containing pepper plugin calling this method.
   *
   * <strong>Note:</strong>
   * For some embedders, we can`t tell if key that we try to unregister have
   * failed because it have been already unregistered. So if at least one key
   * have been successfully processed, we assume that other keys that failed,
   * have been already unregistered before this call.
   *
   * @param[in] instance A <code>PP_Instance</code> identifying instance
   * of the module
   * @param[in] key_count A number of keys which will be grabbed.
   * @param[in] keys An array containing list of keys which should be grabbed.
   *
   * @return An int32_t containing an error code from <code>pp_errors.h</code>.
   * Returns <code>PP_ERROR_BADARGUMENT</code> if <code>key_count</code> is
   * equal 0 or one of <code>keys</code> is not supported anymore.
   * Returns <code>PP_ERROR_NOTSUPPORTED</code> if currently launched embedder
   * doesn`t support key registering.
   * Returns <code>PP_ERROR_FAILED</code> if registering all <code>keys</code>
   * have failed.
   * Returns <code>PP_OK</code> if at lest one key from <code>keys</code> have
   * been registered.
   */
  int32_t (*UnRegisterKeys)(PP_Instance instance,
                            uint32_t key_count,
                            const char* keys[]);
};

typedef struct PPB_RemoteController_Samsung_0_1 PPB_RemoteController_Samsung;
/**
 * @}
 */

#endif /* PPAPI_C_SAMSUNG_PPB_REMOTE_CONTROLLER_SAMSUNG_H_ */
