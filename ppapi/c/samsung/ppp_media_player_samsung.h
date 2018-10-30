/* Copyright (c) 2016 Samsung Electronics. All rights reserved.
 */

/* From samsung/ppp_media_player_samsung.idl,
 *   modified Wed Mar 30 10:21:58 2016.
 */

#ifndef PPAPI_C_SAMSUNG_PPP_MEDIA_PLAYER_SAMSUNG_H_
#define PPAPI_C_SAMSUNG_PPP_MEDIA_PLAYER_SAMSUNG_H_

#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_stdint.h"
#include "ppapi/c/pp_time.h"
#include "ppapi/c/samsung/pp_media_player_samsung.h"

#define PPP_MEDIAEVENTSLISTENER_SAMSUNG_INTERFACE_1_0 \
    "PPP_MediaEventsListener_Samsung;1.0"
#define PPP_MEDIAEVENTSLISTENER_SAMSUNG_INTERFACE \
    PPP_MEDIAEVENTSLISTENER_SAMSUNG_INTERFACE_1_0

#define PPP_SUBTITLELISTENER_SAMSUNG_INTERFACE_1_0 \
    "PPP_SubtitleListener_Samsung;1.0"
#define PPP_SUBTITLELISTENER_SAMSUNG_INTERFACE \
    PPP_SUBTITLELISTENER_SAMSUNG_INTERFACE_1_0

#define PPP_BUFFERINGLISTENER_SAMSUNG_INTERFACE_1_0 \
    "PPP_BufferingListener_Samsung;1.0"
#define PPP_BUFFERINGLISTENER_SAMSUNG_INTERFACE \
    PPP_BUFFERINGLISTENER_SAMSUNG_INTERFACE_1_0

#define PPP_DRMLISTENER_SAMSUNG_INTERFACE_1_0 "PPP_DRMListener_Samsung;1.0"
#define PPP_DRMLISTENER_SAMSUNG_INTERFACE PPP_DRMLISTENER_SAMSUNG_INTERFACE_1_0

/**
 * @file
 * This file defines events listeners used with the
 * <code>PPB_MediaPlayer_Samsung</code> interface.
 *
 * Part of Pepper Media Player interfaces (Samsung's extension).
 * See comments in ppb_media_player_samsung.idl.
 */


/**
 * @addtogroup Interfaces
 * @{
 */
/**
 * Structure containing pointers to functions provided by plugin,
 * and called when player or playback related event occurs.
 */
struct PPP_MediaEventsListener_Samsung_1_0 {
  /**
   * Event sent periodically during clip playback and indicates playback
   * progress.
   *
   * Event will be sent at least twice per second.
   *
   * @param[in] time current media time.
   * @param[in] user_data A pointer to user data passed to a listener function.
   */
  void (*OnTimeUpdate)(PP_TimeTicks time, void* user_data);
  /**
   * Played clip has ended.
   *
   * @param[in] user_data A pointer to user data passed to a listener function.
   */
  void (*OnEnded)(void* user_data);
  /**
   * Error during playback has occurred.
   *
   * @param[in] error An error code signalizing type of occurred error.
   * @param[in] user_data A pointer to user data passed to a listener function.
   */
  void (*OnError)(PP_MediaPlayerError error, void* user_data);
};

typedef struct PPP_MediaEventsListener_Samsung_1_0
    PPP_MediaEventsListener_Samsung;

/**
 * Listener for receiving subtitle updates provided by the player's internal
 * subtitle parser. This listener will be notified every time active and visible
 * text track contains a subtitle that should be displayed at the current
 * playback position.
 */
struct PPP_SubtitleListener_Samsung_1_0 {
  /**
   * Event sent when a subtitle should be displayed.
   *
   * @param[in] duration Duration for which the text should be displayed.
   * @param[in] text The UTF-8 encoded string that contains a subtitle text
   * that should be displayed. Please note text encoding will be UTF-8
   * independently from the source subtitle file encoding.
   * @param[in] user_data A pointer to user data passed to a listener function.
   */
  void (*OnShowSubtitle)(PP_TimeDelta duration,
                         const char* text,
                         void* user_data);
};

typedef struct PPP_SubtitleListener_Samsung_1_0 PPP_SubtitleListener_Samsung;

/**
 * Listener for receiving initial media buffering related events, sent
 * before playback can be started.
 *
 * Those event can be used by the application to show buffering progress bar
 * to the user.
 *
 * Those events are sent only when media buffering is managed by the player
 * implementation (see <code>PPB_URLDataSource_Samsung</code>), not by the
 * user (see <code>PPB_ESDataSource_Samsung</code>).
 */
struct PPP_BufferingListener_Samsung_1_0 {
  /**
   * Initial media buffering has been started by the player.
   *
   * @param[in] user_data A pointer to user data passed to a listener function.
   */
  void (*OnBufferingStart)(void* user_data);
  /**
   * Initial buffering in progress.
   *
   * @param[in] percent Indicates how much of the initial data has been
   * buffered by the player.
   * @param[in] user_data A pointer to user data passed to a listener function.
   */
  void (*OnBufferingProgress)(uint32_t percent, void* user_data);
  /**
   * Initial media buffering has been completed by the player, after that
   * event playback might be started.
   *
   * @param[in] user_data A pointer to user data passed to a listener function.
   */
  void (*OnBufferingComplete)(void* user_data);
};

typedef struct PPP_BufferingListener_Samsung_1_0 PPP_BufferingListener_Samsung;

/**
 * Listener for receiving DRM related events.
 */
struct PPP_DRMListener_Samsung_1_0 {
  /**
   * During parsing media container encrypted track was found.
   *
   * @param[in] drm_type A type of DRM system
   * @param[in] init_data_size Size in bytes of |init_data| buffer.
   * @param[in] init_data A pointer to the buffer containing DRM specific
   * initialization data
   * @param[in] user_data A pointer to user data passed to a listener function.
   */
  void (*OnInitdataLoaded)(PP_MediaPlayerDRMType drm_type,
                           uint32_t init_data_size,
                           const void* init_data,
                           void* user_data);
  /**
   * Decryption license needs to be requested from the server and
   * provided to the player.
   *
   * @param[in] request_size Size in bytes of |request| buffer.
   * @param[in] request A pointer to the buffer containing DRM specific request.
   * @param[in] user_data A pointer to user data passed to a listener function.
   */
  void (*OnLicenseRequest)(uint32_t request_size,
                           const void* request,
                           void* user_data);
};

typedef struct PPP_DRMListener_Samsung_1_0 PPP_DRMListener_Samsung;
/**
 * @}
 */

#endif  /* PPAPI_C_SAMSUNG_PPP_MEDIA_PLAYER_SAMSUNG_H_ */

