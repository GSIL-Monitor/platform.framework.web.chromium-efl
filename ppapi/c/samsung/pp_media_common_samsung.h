/* Copyright (c) 2016 Samsung Electronics. All rights reserved.
 */

/* From samsung/pp_media_common_samsung.idl,
 *   modified Mon May 29 08:34:28 2017.
 */

#ifndef PPAPI_C_SAMSUNG_PP_MEDIA_COMMON_SAMSUNG_H_
#define PPAPI_C_SAMSUNG_PP_MEDIA_COMMON_SAMSUNG_H_

#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_stdint.h"

/**
 * @file
 * This file defines common structures used by Samsung media PPAPI interfaces.
 */


/**
 * @addtogroup Typedefs
 * @{
 */
/* Describes time delta in microseconds */
typedef int64_t PP_MicrosecondsDelta;
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_MicrosecondsDelta, 8);
/**
 * @}
 */

/**
 * @addtogroup Enums
 * @{
 */
/**
 * List of elementary stream types of which played media container
 * can consists of.
 *
 * Values of valid elementary streams can be used to index array.
 */
typedef enum {
  /**
   * Unknown Elementary Stream.
   */
  PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_UNKNOWN = -1,
  /**
   * Video Elementary Stream.
   */
  PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_VIDEO = 0,
  /**
   * Audio Elementary Stream.
   */
  PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_AUDIO = 1,
  /**
   * Text/Subtitles Elementary Stream.
   */
  PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_TEXT = 2,
  /**
   * Number of valid Elementary Streams, which can be
   * used as an array size.
   */
  PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_MAX = 3
} PP_ElementaryStream_Type_Samsung;
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_ElementaryStream_Type_Samsung, 4);

/**
 * Lists possible Elementary Stream initialization modes.
 *
 * Values of this enumeration are used to signal how much information
 * describing Elementary Stream itself (metadata) was extracted by the plugin.
 */
typedef enum {
  /**
   * Full initialization mode.
   *
   * In this mode all information related to the stream configuration
   * are extracted by the plugin (demuxer) and passed to the browser.
   */
  PP_STREAMINITIALIZATIONMODE_FULL = 0,
  /**
   * Minimal initialization mode.
   *
   * In this mode only basic stream information like codec and maximal
   * resolution (for Video Elementary Stream) is extracted by the plugin
   * (demuxer). All other information is extracted from the Elementary Stream
   * by the browser (decoder).
   *
   * As the browser may use H/W decoder to extract such information, this
   * mode may speed up overall time of pipeline initialization in MPEG2-TS
   * channel change scenarios.
   *
   * Minimal information requried to be provided by the plugin:
   * 1. Audio Elementary Stream:
   *    - codec type
   * 2. Video Elementary Stream:
   *    - codec type
   *    - maximal video resolution (width and height)
   */
  PP_STREAMINITIALIZATIONMODE_MINIMAL = 1
} PP_StreamInitializationMode;
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_StreamInitializationMode, 4);
/**
 * @}
 */

/**
 * @addtogroup Structs
 * @{
 */
/* The <code>PP_EncryptedSubsampleDescription</code> structure contains
 * information to support subsample decryption.
 *
 * An input block can be split into several continuous subsamples.
 * A <code>PP_DecryptSubsampleEntry</code> specifies the number of clear and
 * cipher bytes in each subsample. For example, the following block has three
 * subsamples:
 *
 * |<----- subsample1 ----->|<----- subsample2 ----->|<----- subsample3 ----->|
 * |   clear1   |  cipher1  |  clear2  |   cipher2   | clear3 |    cipher3    |
 *
 * For decryption, all of the cipher bytes in a block should be treated as a
 * contiguous (in the subsample order) logical stream. The clear bytes should
 * not be considered as part of decryption.
 *
 * Logical stream to decrypt:   |  cipher1  |   cipher2   |    cipher3    |
 * Decrypted stream:            | decrypted1|  decrypted2 |   decrypted3  |
 *
 * After decryption, the decrypted bytes should be copied over the position
 * of the corresponding cipher bytes in the original block to form the output
 * block. Following the above example, the decrypted block should be:
 *
 * |<----- subsample1 ----->|<----- subsample2 ----->|<----- subsample3 ----->|
 * |   clear1   | decrypted1|  clear2  |  decrypted2 | clear3 |   decrypted3  |
 */
struct PP_EncryptedSubsampleDescription {
  /*
   * Size in bytes of clear data in a subsample entry.
   */
  uint32_t clear_bytes;
  /*
   * Size in bytes of encrypted data in a subsample entry.
   */
  uint32_t cipher_bytes;
};
PP_COMPILE_ASSERT_STRUCT_SIZE_IN_BYTES(PP_EncryptedSubsampleDescription, 8);
/**
 * @}
 */

/**
 * @addtogroup Functions
 * @{
 */

/** Converts PP_MicrosecondsDelta to seconds */
PP_INLINE double PP_MicrosecondsDeltaToSeconds(PP_MicrosecondsDelta value) {
  return value / 1000000.0;
}

/** Converts seconds to PP_MicrosecondsDelta */
PP_INLINE PP_MicrosecondsDelta PP_SecondsToMicrosecondsDelta(double value) {
  return (int64_t)(value * 1000000.0 + 0.5);
}

/**
 * @}
 */

#endif  /* PPAPI_C_SAMSUNG_PP_MEDIA_COMMON_SAMSUNG_H_ */

