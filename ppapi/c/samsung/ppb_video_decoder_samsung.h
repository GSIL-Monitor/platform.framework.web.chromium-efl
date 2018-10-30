/* Copyright (c) 2018 Samsung Electronics. All rights reserved.
 */

/* From samsung/ppb_video_decoder_samsung.idl,
 *   modified Fri Oct  5 10:10:10 2018.
 */

#ifndef PPAPI_C_SAMSUNG_PPB_VIDEO_DECODER_SAMSUNG_H_
#define PPAPI_C_SAMSUNG_PPB_VIDEO_DECODER_SAMSUNG_H_

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_stdint.h"

#define PPB_VIDEODECODER_SAMSUNG_INTERFACE_1_0 "PPB_VideoDecoder_Samsung;1.0"
#define PPB_VIDEODECODER_SAMSUNG_INTERFACE \
    PPB_VIDEODECODER_SAMSUNG_INTERFACE_1_0

/**
 * @file
 * This file defines the <code>PPB_VideoDecoder_Samsung</code> interface.
 */


/**
 * @addtogroup Enums
 * @{
 */
/**
 * PP_VideoLatencyMode is an enumeration of the different video latency modes.
 */
typedef enum {
  PP_VIDEOLATENCYMODE_NORMAL = 0,
  PP_VIDEOLATENCYMODE_LOW = 1,
  PP_VIDEOLATENCYMODE_LAST = PP_VIDEOLATENCYMODE_LOW
} PP_VideoLatencyMode;
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_VideoLatencyMode, 4);
/**
 * @}
 */

/**
 * @addtogroup Interfaces
 * @{
 */
/**
 * It is an extension for <code>PPB_VideoDecoder</code> interface,
 * which allows setting low latency mode for video decoding.
 */
struct PPB_VideoDecoder_Samsung_1_0 {
  /**
   * Determines if the given resource is a Video Decoder.
   *
   * @param[in] resource A <code>PP_Resource</code> identifying a resource.
   *
   * @return <code>PP_TRUE</code> if the resource is a
   * <code>PPB_VideoDecoder</code>, <code>PP_FALSE</code> if the resource is
   * invalid or some other type.
   */
  PP_Bool (*IsVideoDecoderSamsung)(PP_Resource resource);
  /**
   * Sets specified latency mode for <code>PPB_VideoDecoder</code> resource.
   * This method must be called before calling Initialize method from
   * <code>PPB_VideoDecoder</code> API.
   *
   * Setting low latency mode results in much smaller delay between input data
   * and decoded frames. In this mode only 'I' and 'P' frames can be used.
   * Passing 'B' frame may result in error.
   *
   * In low latency mode, it is possible to call GetPicture method from
   * <code>PPB_VideoDecoder</code> API after each decode.
   *
   * If not specified, PP_VIDEOLATENCYMODE_NORMAL will be used.
   *
   * @param[in] video_decoder A <code>PP_Resource</code> identifying the video
   * decoder.
   * @param[in] mode A <code>PP_VideoLatencyMode</code> to return to
   * the decoder.
   *
   * @return An int32_t containing a result code from <code>pp_errors.h</code>.
   */
  int32_t (*SetLatencyMode)(PP_Resource video_decoder,
                            PP_VideoLatencyMode mode);
};

typedef struct PPB_VideoDecoder_Samsung_1_0 PPB_VideoDecoder_Samsung;
/**
 * @}
 */

#endif  /* PPAPI_C_SAMSUNG_PPB_VIDEO_DECODER_SAMSUNG_H_ */

