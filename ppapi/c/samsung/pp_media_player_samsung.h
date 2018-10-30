/* Copyright (c) 2016 Samsung Electronics. All rights reserved.
 */

/* From samsung/pp_media_player_samsung.idl,
 *   modified Tue Aug  1 16:26:08 2017.
 */

#ifndef PPAPI_C_SAMSUNG_PP_MEDIA_PLAYER_SAMSUNG_H_
#define PPAPI_C_SAMSUNG_PP_MEDIA_PLAYER_SAMSUNG_H_

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_size.h"
#include "ppapi/c/pp_stdint.h"
#include "ppapi/c/pp_time.h"
#include "ppapi/c/samsung/pp_media_common_samsung.h"

/**
 * @file
 * This file defines common structures used with the
 * <code>PPB_MediaPlayer</code> interface.
 *
 * Part of Pepper Media Player interfaces (Samsung's extension).
 * See comments in ppb_media_player_samsung.
 */


/**
 * @addtogroup Enums
 * @{
 */
/**
 * List of media player states.
 */
typedef enum {
  /**
   * Indicates invalid state for the player
   */
  PP_MEDIAPLAYERSTATE_NONE = 0,
  /**
   * Player has been created but no data sources hasn't been attached yet.
   */
  PP_MEDIAPLAYERSTATE_UNINITIALIZED = 1,
  /**
   * Player has been created and data sources has been attached.
   * Playback is ready to be started.
   */
  PP_MEDIAPLAYERSTATE_READY = 2,
  /**
   * Player is playing media. Media time is advancing.
   */
  PP_MEDIAPLAYERSTATE_PLAYING = 3,
  /**
   * Playback has been paused and can be resumed.
   */
  PP_MEDIAPLAYERSTATE_PAUSED = 4,
  PP_MEDIAPLAYERSTATE_LAST = PP_MEDIAPLAYERSTATE_PAUSED
} PP_MediaPlayerState;
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_MediaPlayerState, 4);

/**
 * List of errors which might be reported by the player.
 */
typedef enum {
  /** No error has occurred */
  PP_MEDIAPLAYERERROR_NONE = 0,
  /** Input error has occured */
  PP_MEDIAPLAYERERROR_BAD_ARGUMENT = 1,
  /** Network error has occurred */
  PP_MEDIAPLAYERERROR_NETWORK = 2,
  /** Demuxer error has occured */
  PP_MEDIAPLAYERERROR_DEMUX = 3,
  /** Decryptor error has occured */
  PP_MEDIAPLAYERERROR_DECRYPT = 4,
  /** Decoder error has occurred */
  PP_MEDIAPLAYERERROR_DECODE = 5,
  /** Rendering error has occurred */
  PP_MEDIAPLAYERERROR_RENDER = 6,
  /** Media is encoded using unsupported codec */
  PP_MEDIAPLAYERERROR_UNSUPPORTED_CODEC = 7,
  /** Container is not supported */
  PP_MEDIAPLAYERERROR_UNSUPPORTED_CONTAINER = 8,
  /** Resource error has occured (i.e. no space left on device, no such file) */
  PP_MEDIAPLAYERERROR_RESOURCE = 9,
  /** Unknown error has occurred */
  PP_MEDIAPLAYERERROR_UNKNOWN = 10,
  /** Subtitles file format is not supported. */
  PP_MEDIAPLAYERERROR_UNSUPPORTED_SUBTITLE_FORMAT = 11,
  PP_MEDIAPLAYERERROR_LAST = PP_MEDIAPLAYERERROR_UNSUPPORTED_SUBTITLE_FORMAT
} PP_MediaPlayerError;
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_MediaPlayerError, 4);
/**
 * @}
 */

/**
 * @addtogroup Structs
 * @{
 */
/**
 * Structure describing video track from played media.
 */
struct PP_VideoTrackInfo {
  /**
   * Index of the track in currently played media.
   * Note this index is only valid for currently attached data source.
   */
  uint32_t index;
  /**
   * Minimal bandwidth (in bits per second - bps) of delivery channel
   * required to deliver given video track.
   */
  uint32_t bitrate;
  /**
   * Decoded video frame size in pixels.
   */
  struct PP_Size size;
};
PP_COMPILE_ASSERT_STRUCT_SIZE_IN_BYTES(PP_VideoTrackInfo, 16);

/**
 * Structure describing audio track from played media.
 */
struct PP_AudioTrackInfo {
  /**
   * Index of the track in currently played media.
   * Note this index is only valid for currently attached data source.
   */
  uint32_t index;
  /**
   * C style string representing language of audio track
   * encoded in IETF RFC 5646
   */
  char language[64];
};
PP_COMPILE_ASSERT_STRUCT_SIZE_IN_BYTES(PP_AudioTrackInfo, 68);

/**
 * Structure describing text/subtitles track from played media.
 */
struct PP_TextTrackInfo {
  /**
   * Index of the track in currently played media.
   * Note this index is only valid for currently attached data source.
   */
  uint32_t index;
  /**
   * Is current text track internal (contained in media clip)
   * or external (loaded from separate file).
   *
   * Remarks:
   * This flag is deprecated. Value of this variable is undefined and should be
   * ignored.
   */
  PP_Bool is_external;
  /**
   * C style string representing language of text track
   * encoded in IETF RFC 5646
   */
  char language[64];
};
PP_COMPILE_ASSERT_STRUCT_SIZE_IN_BYTES(PP_TextTrackInfo, 72);
/**
 * @}
 */

/**
 * @addtogroup Enums
 * @{
 */
/**
 * List of supported types of DRM systems.
 */
typedef enum {
  /** Unknown DRM system */
  PP_MEDIAPLAYERDRMTYPE_UNKNOWN = 0,
  /** Playready DRM system */
  PP_MEDIAPLAYERDRMTYPE_PLAYREADY = 1,
  /** Marlin DRM system */
  PP_MEDIAPLAYERDRMTYPE_MARLIN = 2,
  /** Verimatrix DRM system */
  PP_MEDIAPLAYERDRMTYPE_VERIMATRIX = 3,
  /** Widevine classic DRM system */
  PP_MEDIAPLAYERDRMTYPE_WIDEVINE_CLASSIC = 4,
  /** Widevine modular (used with MPEG-DASH) DRM system */
  PP_MEDIAPLAYERDRMTYPE_WIDEVINE_MODULAR = 5,
  /** Securemedia DRM system */
  PP_MEDIAPLAYERDRMTYPE_SECUREMEDIA = 6,
  /** SDRM system */
  PP_MEDIAPLAYERDRMTYPE_SDRM = 7,
  /** ClearKey system */
  PP_MEDIAPLAYERDRMTYPE_CLEARKEY = 8,
  /** External DRM system provided by Trusted Application */
  PP_MEDIAPLAYERDRMTYPE_EXTERNAL = 9,
  PP_MEDIAPLAYERDRMTYPE_LAST = PP_MEDIAPLAYERDRMTYPE_EXTERNAL
} PP_MediaPlayerDRMType;
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_MediaPlayerDRMType, 4);

/**
 * List of possible DRM system operation.
 */
typedef enum {
  /**
   * Setting properties of a DRM instance.
   */
  PP_MEDIAPLAYERDRMOPERATION_SETPROPERTIES = 1,
  /**
   * Enabling challenge message instead of DRM's
   * connecting to a license server.
   */
  PP_MEDIAPLAYERDRMOPERATION_GENCHALLENGE = 2,
  /**
   * Installing a license into a DRM instance.
   */
  PP_MEDIAPLAYERDRMOPERATION_INSTALLLICENSE = 3,
  /**
   * Deleting a license into a DRM instance.
   */
  PP_MEDIAPLAYERDRMOPERATION_DELETELICENSE = 4,
  /**
   * Requesting key to DRM license server with the initiator information.
   */
  PP_MEDIAPLAYERDRMOPERATION_PROCESSINITIATOR = 5,
  /**
   * Querying the version of DRM system.
   */
  PP_MEDIAPLAYERDRMOPERATION_GETVERSION = 6,
  PP_MEDIAPLAYERDRMOPERATION_LAST = PP_MEDIAPLAYERDRMOPERATION_GETVERSION
} PP_MediaPlayerDRMOperation;
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_MediaPlayerDRMOperation, 4);

/**
 * Enum describing available streaming properties which can be set
 * by the user.
 */
typedef enum {
  /**
   * The cookie for streaming playback.
   * This corresponds to a string (PP_VARTYPE_STRING)
   */
  PP_STREAMINGPROPERTY_COOKIE = 1,
  /**
   * The user agent for streaming playback.
   * This corresponds to a string (PP_VARTYPE_STRING)
   */
  PP_STREAMINGPROPERTY_USER_AGENT = 2,
  /**
   * Sets adaptive streaming format specific properties.
   *
   * Eg. for smooth streaming (strings should be concatenated):
   *    "|BITRATES=50000~2500000"
   *    "|STARTBITRATE=150000"
   *    "|SKIPBITRATE=100000"
   *
   * Eg. for DRM (strings should be concatenated):
   *    "DEVICE_ID=null"
   *    "DEVICET_TYPE_ID=60"
   *    "DRM_URL=https://..."
   *
   * This corresponds to a string (PP_VARTYPE_STRING).
   */
  PP_STREAMINGPROPERTY_ADAPTIVE_INFO = 3,
  /**
   * Sets streaming type.
   *
   * Eg. :
   *  "WIDEVINE"
   *  "HLS"
   *  "VUDU"
   *  "HAS"
   *  "SMOOTH"
   *
   * This corresponds to a string (PP_VARTYPE_STRING).
   */
  PP_STREAMINGPROPERTY_TYPE = 4,
  /**
   * Gets available bitrates in bps (bits per second).
   *
   * Eg. (values are concatenated and separated by '|'):
   *  "100|500|1000"
   *
   * Result is a string (PP_VARTYPE_STRING).
   */
  PP_STREAMINGPROPERTY_AVAILABLE_BITRATES = 5,
  /**
   * Gets throughput in bps.
   *
   * Throughput is how much data actually does travel through
   * the 'channel' successfully.
   *
   * Result is a string (PP_VARTYPE_STRING).
   */
  PP_STREAMINGPROPERTY_THROUGHPUT = 6,
  /**
   * Gets duration of the clip in seconds.
   *
   * Result is a string (PP_VARTYPE_STRING).
   */
  PP_STREAMINGPROPERTY_DURATION = 7,
  /**
   * Gets current bitrate in bps.
   *
   * Result is a string (PP_VARTYPE_STRING).
   */
  PP_STREAMINGPROPERTY_CURRENT_BITRATE = 8,
  PP_STREAMINGPROPERTY_LAST = PP_STREAMINGPROPERTY_CURRENT_BITRATE
} PP_StreamingProperty;
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_StreamingProperty, 4);

/**
 * An enum that controls Media Player mode. Some non-trivial Media Player
 * functions  might be supported only if certain <code>PP_MediaPlayerMode</code>
 * is set.
 */
typedef enum {
  /**
   * Default Media Player mode that is suitable for the most common playback
   * scenarios. This mode is used in <code>PPB_MediaPlayer_Samsung::Create(
   * PP_Instance)</code> when no mode is specified.
   */
  PP_MEDIAPLAYERMODE_DEFAULT = 0,
  /**
   * Media Player mode that is dedicated for a D2TV playback.
   */
  PP_MEDIAPLAYERMODE_D2TV = 1,
  PP_MEDIAPLAYERMODE_LAST = PP_MEDIAPLAYERMODE_D2TV
} PP_MediaPlayerMode;
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_MediaPlayerMode, 4);

/**
 * An enum that controls binding Media Player to <code>PPB_Instance</code>
 * graphics during construction. For more details see descriptions of
 * <code>PPB_MediaPlayer_Samsung::Create(PP_Instance)</code> and <code>
 * PPB_MediaPlayer_Samsung::CreateWithOptions(PP_Instance, PP_MediaPlayerMode,
 * PP_BindToInstanceMode)</code>.
 */
typedef enum {
  /**
   * Automatically bind Media Player to <code>PPB_Instance</code> graphics.
   * This should be always used when no other resource that binds to
   * <code>PPB_Instance</code> is used alongside the player.
   */
  PP_BINDTOINSTANCEMODE_BIND = 0,
  /**
   * Don't bind Media Player to <code>PPB_Instance</code> graphics. Useful when
   * other graphical resources that need to be bound to <code>
   * PPB_Instance</code> graphics are used alongside the player.
   */
  PP_BINDTOINSTANCEMODE_DONT_BIND = 1,
  PP_BINDTOINSTANCEMODE_LAST = PP_BINDTOINSTANCEMODE_DONT_BIND
} PP_BindToInstanceMode;
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_BindToInstanceMode, 4);

/**
 * An enum that controls Media Player display mode. For details, please see
 * descriptions of particular enum values. A default value is <code>
 * PP_MEDIAPLAYERDISPLAYMODE_STRETCH</code>.
 */
typedef enum {
  /**
   * When this mode is set a video will be stretched to match either:
   * - entire player area (which takes up entire plugin area by default),
   * - or an area explicitly set as display rect with <code>
   * PPB_MediaPlayer_Samsung::SetDisplayRect(...)</code>.
   */
  PP_MEDIAPLAYERDISPLAYMODE_STRETCH = 0,
  /**
   * When this mode is set a video will be scaled to fit a player area but will
   * also maintain it's aspect ratio. If player area aspect ratio doesn't match
   * video's aspect ratio, then resulting video will be letterboxed (or
   * pillarboxed).
   */
  PP_MEDIAPLAYERDISPLAYMODE_LETTERBOX = 1,
  PP_MEDIAPLAYERDISPLAYMODE_LAST = PP_MEDIAPLAYERDISPLAYMODE_LETTERBOX
} PP_MediaPlayerDisplayMode;
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_MediaPlayerDisplayMode, 4);

/**
 * An enum that controls Media Player Vr360 mode. For details, please see
 * descriptions of particular enum values. A default value is <code>
 * PP_MEDIAPLAYERVR360MODE_OFF</code>.
 */
typedef enum {
  /**
   * When this mode is set a video will be displayed normally.
   */
  PP_MEDIAPLAYERVR360MODE_OFF = 0,
  /**
   * When this mode is set a video will be displayed in Vr mode.
   */
  PP_MEDIAPLAYERVR360MODE_ON = 1,
  PP_MEDIAPLAYERVR360MODE_LAST = PP_MEDIAPLAYERVR360MODE_ON
} PP_MediaPlayerVr360Mode;
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_MediaPlayerVr360Mode, 4);
/**
 * @}
 */

/**
 * @addtogroup Structs
 * @{
 */
/**
 * Structure describing Elementary Stream packet. It contains
 * all necessary timing information to display sent packets in appropriate
 * order and synchronize different streams.
 */
struct PP_ESPacket {
  /**
   * Presentation Timestamp, indicates time relative to begging of
   * the stream when packet must be rendered.
   */
  PP_TimeTicks pts;
  /**
   * Decode Timestamp, indicates time relative to begging of the stream
   * when packet must be sent to decoder.
   *
   * Usually equals to pts except rare cases like B-Frames.
   */
  PP_TimeTicks dts;
  /**
   * Duration of the packet.
   */
  PP_TimeDelta duration;
  /**
   * Whether the packet represents a key frame.
   */
  PP_Bool is_key_frame;
  /**
   * Size in bytes of packet data
   */
  uint32_t size;
  /**
   * Base address of buffer containing data of the packet.
   */
  void* buffer;
};

/**
 * Structure describing encrypted packet.
 */
struct PP_ESPacketEncryptionInfo {
  /**
   * Size in bytes of the |key_id| field.
   */
  uint32_t key_id_size;
  /**
   * Base address of buffer containing id of the key needed to decrypt
   * given packet.
   */
  void* key_id;
  /**
   * Size in bytes of the |iv| field.
   */
  uint32_t iv_size;
  /**
   * Base address of buffer containing Initialization Vector (IV)
   * needed to decrypt given packet.
   */
  void* iv;
  /**
   * Number of subsamples for given ES packet (sample).
   */
  uint32_t num_subsamples;
  /**
   * Description of subsamples of which given ES packet consists of.
   * Size of the array is |num_subsamples|.
   */
  struct PP_EncryptedSubsampleDescription *subsamples;
};

/**
 * Structure describing TrustZone memory reference.
 */
struct PP_TrustZoneReference {
  /**
   * Handle to a TrustZone memory containing data.
   */
  uint32_t handle;
  /**
   * Size of the memory in TrustZone
   */
  uint32_t size;
};
PP_COMPILE_ASSERT_STRUCT_SIZE_IN_BYTES(PP_TrustZoneReference, 8);
/**
 * @}
 */

#endif  /* PPAPI_C_SAMSUNG_PP_MEDIA_PLAYER_SAMSUNG_H_ */

