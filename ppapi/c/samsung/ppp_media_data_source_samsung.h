/* Copyright (c) 2016 Samsung Electronics. All rights reserved.
 */

/* From samsung/ppp_media_data_source_samsung.idl,
 *   modified Wed Mar 30 10:21:58 2016.
 */

#ifndef PPAPI_C_SAMSUNG_PPP_MEDIA_DATA_SOURCE_SAMSUNG_H_
#define PPAPI_C_SAMSUNG_PPP_MEDIA_DATA_SOURCE_SAMSUNG_H_

#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_stdint.h"
#include "ppapi/c/pp_time.h"

#define PPP_ELEMENTARYSTREAMLISTENER_SAMSUNG_INTERFACE_1_0 \
    "PPP_ElementaryStreamListener_Samsung;1.0"
#define PPP_ELEMENTARYSTREAMLISTENER_SAMSUNG_INTERFACE \
    PPP_ELEMENTARYSTREAMLISTENER_SAMSUNG_INTERFACE_1_0

/**
 * @file
 * This file defines events listeners used with the
 * <code>PPB_MediaDataSource_Samsung</code> interface
 * and its descendants.
 *
 * Part of Pepper Media Player interfaces (Samsung's extension).
 * See comments in ppb_media_data_source_samsung.idl.
 */


/**
 * @addtogroup Interfaces
 * @{
 */
/**
 * Structure containing pointers to functions provided by plugin,
 * and called when player or playback related event occurs.
 */
struct PPP_ElementaryStreamListener_Samsung_1_0 {
  /**
   * Event is fired when internal queue is running out of data.
   * Application should start pushing more data via
   * <code>PPB_ElementaryStream_Samsung::AppendPacket</code> or
   * <code>PPB_ElementaryStream_Samsung::AppendEncryptedPacket</code> function.
   *
   * @param[in] bytes_max Maximum number of bytes which can stored in
   * the internal queue.
   * @param[in] user_data A pointer to user data passed to a listener function.
   */
  void (*OnNeedData)(int32_t bytes_max, void* user_data);
  /**
   * Event is fired when internal queue is full. Application should stop pushing
   * buffers.
   *
   * @param[in] user_data A pointer to user data passed to a listener function.
   */
  void (*OnEnoughData)(void* user_data);
  /**
   * Event is fired to notify application to change position of the stream.
   * After receiving this event, application should push buffers from the new
   * position.
   *
   * @param[in] new_position The new position in the stream.
   * @param[in] user_data A pointer to user data passed to a listener function.
   */
  void (*OnSeekData)(PP_TimeTicks new_position, void* user_data);
};

typedef struct PPP_ElementaryStreamListener_Samsung_1_0
    PPP_ElementaryStreamListener_Samsung;
/**
 * @}
 */

#endif  /* PPAPI_C_SAMSUNG_PPP_MEDIA_DATA_SOURCE_SAMSUNG_H_ */

