/* Copyright (c) 2017 Samsung Electronics. All rights reserved.
 */

/* From samsung/ppb_audio_config_samsung.idl,
 *   modified Mon Oct 23 16:05:56 2017.
 */

#ifndef PPAPI_C_SAMSUNG_PPB_AUDIO_CONFIG_SAMSUNG_H_
#define PPAPI_C_SAMSUNG_PPB_AUDIO_CONFIG_SAMSUNG_H_

#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_stdint.h"

#define PPB_AUDIO_CONFIG_SAMSUNG_INTERFACE_1_0 "PPB_AudioConfig_Samsung;1.0"
#define PPB_AUDIO_CONFIG_SAMSUNG_INTERFACE \
  PPB_AUDIO_CONFIG_SAMSUNG_INTERFACE_1_0

/**
 * @file
 * This file defines the PPB_AudioConfig_Samsung interface for choosing
 * audio modes for <code>PPB_AudioConfig</code> resource.
 */

/**
 * @addtogroup Enums
 * @{
 */
/**
 * PP_AudioMode is an enumeration of the different audio modes.
 */
typedef enum {
  /*
   * Platform will try to detect the most suitable mode for a plugin.
   */
  PP_AUDIOMODE_AUTO = 0,
  PP_AUDIOMODE_GAME = 1,
  PP_AUDIOMODE_MUSIC = 2,
  PP_AUDIOMODE_LAST = PP_AUDIOMODE_MUSIC
} PP_AudioMode;
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_AudioMode, 4);
/**
 * @}
 */

/**
 * @addtogroup Interfaces
 * @{
 */
/**
 * Extension for <code>PPB_AudioConfig</code> interface, which allows choosing
 * audio mode.
 */
struct PPB_AudioConfig_Samsung_1_0 {
  /**
   * Sets specified audio mode. This method needs to be called before
   * creating <code>PPB_Audio</code> resource.
   *
   * @param[in] resource A <code>PP_Resource</code> corresponding to a
   * <code>PP_AudioConfig</code> resource.
   *
   * @param[in] audio_mode A <code>PP_AudioMode</code> which
   * will be used for <code>PP_Audio</code> resource.
   *
   * @return An int32_t containing a result code from <code>pp_errors.h</code>.
   */
  int32_t (*SetAudioMode)(PP_Resource resource, PP_AudioMode audio_mode);
};

typedef struct PPB_AudioConfig_Samsung_1_0 PPB_AudioConfig_Samsung;
/**
 * @}
 */

#endif /* PPAPI_C_SAMSUNG_PPB_AUDIO_CONFIG_SAMSUNG_H_ */
