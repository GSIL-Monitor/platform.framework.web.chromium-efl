/* Copyright (c) 2016 Samsung Electronics. All rights reserved.
 */

/* From samsung/ppb_media_player_samsung.idl,
 *   modified Thu Aug 17 11:51:57 2017.
 */

#ifndef PPAPI_C_SAMSUNG_PPB_MEDIA_PLAYER_SAMSUNG_H_
#define PPAPI_C_SAMSUNG_PPB_MEDIA_PLAYER_SAMSUNG_H_

#include "ppapi/c/pp_array_output.h"
#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_point.h"
#include "ppapi/c/pp_rect.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_size.h"
#include "ppapi/c/pp_stdint.h"
#include "ppapi/c/pp_time.h"
#include "ppapi/c/samsung/pp_media_common_samsung.h"
#include "ppapi/c/samsung/pp_media_player_samsung.h"
#include "ppapi/c/samsung/ppp_media_player_samsung.h"

#define PPB_MEDIAPLAYER_SAMSUNG_INTERFACE_1_0 "PPB_MediaPlayer_Samsung;1.0"
#define PPB_MEDIAPLAYER_SAMSUNG_INTERFACE_1_1 "PPB_MediaPlayer_Samsung;1.1"
#define PPB_MEDIAPLAYER_SAMSUNG_INTERFACE_1_2 "PPB_MediaPlayer_Samsung;1.2"
#define PPB_MEDIAPLAYER_SAMSUNG_INTERFACE_1_3 "PPB_MediaPlayer_Samsung;1.3"
#define PPB_MEDIAPLAYER_SAMSUNG_INTERFACE_1_4 "PPB_MediaPlayer_Samsung;1.4"
#define PPB_MEDIAPLAYER_SAMSUNG_INTERFACE PPB_MEDIAPLAYER_SAMSUNG_INTERFACE_1_4

/**
 * @file
 * This file defines the <code>PPB_MediaPlayer_Samsung</code> interface,
 * which provides multimedia streaming, playback control capabilities.
 *
 * Part of Pepper Media Player interfaces (Samsung's extension).
 *
 * Basic idea:
 * - <code>PPB_MediaPlayer_Samsung</code> is interface allowing application
 *   to control playback state, inquire about playback state and to
 *   set listeners for various playback related events (see
 *   <code>PPP_MediaEventsListener_Samsung</code>). It's also responsible for
 *   assigning data source which will feed player with media data.
 *
 * For full description of Data Sources see ppb_media_data_source_samsung.idl.
 */


/**
 * @addtogroup Interfaces
 * @{
 */
/**
 * Interface representing player and providing functionality of playback
 * control and attaching data source feeding player.
 *
 * Assumptions:
 * - Video is displayed in area of embed/object element of HTML page
 *   in which plugin is embedded.
 * - Video and graphics from module can be displayed simultaneously.
 *   (see <code>PPB_Instance.BindGraphics</code>). Application must
 *   set proper CSS style for embed element with NaCl application
 *   (mostly transparent background).
 */
struct PPB_MediaPlayer_Samsung_1_4 {
  /**
   * Creates a new media player resource with <code>PP_MEDIAPLAYERMODE_DEFAULT
   * </code> player mode and binds it to the instance.
   *
   * @param[in] instance A <code>PP_Instance</code> identifying the instance
   * with the media player.
   *
   * @return A <code>PP_Resource</code> corresponding to a media player if
   * successful or 0 otherwise.
   */
  PP_Resource (*Create)(PP_Instance instance);
  /**
   * Creates a player with specified modes of player and binding to
   * pp::Instance.
   *
   * @param[in] instance A <code>PP_Instance</code> identifying the instance
   * with the media player.
   * @param[in] player_mode A <code>PP_MediaPlayerMode</code> identifying the
   * player mode used for media player instance.
   * @param[in] bind_mode A <code>PP_BindToInstanceMode</code> specifying if
   * binding to <code>PPB_Instance</code> graphics should be performed.
   *
   * @return A <code>PP_Resource</code> corresponding to a media player if
   * successful or 0 otherwise.
   */
  PP_Resource (*CreateWithOptions)(PP_Instance instance,
                                   PP_MediaPlayerMode player_mode,
                                   PP_BindToInstanceMode bind_mode);
  /**
   * Determines if the given resource is a media player.
   *
   * @param[in] resource A <code>PP_Resource</code> identifying a resource.
   *
   * @return <code>PP_TRUE</code> if the resource is a
   * <code>PPB_MediaPlayer_Samsung</code>, <code>PP_FALSE</code>
   * if the resource is invalid or some other type.
   */
  PP_Bool (*IsMediaPlayer)(PP_Resource controller);
  /**
   * Attaches <code>PPP_MediaEventsListener_Samsung</code> to the player.
   * After attaching plugin is notified about media playback related event.
   * Previously attached listener (if any) is detached.
   *
   * All listeners will be called in the same thread. If this method was
   * invoked in a thread with a running message loop, the listener will run in
   * the current thread. Otherwise, it will run on the main thread.
   *
   * @param[in] player A <code>PP_Resource</code> identifying the media player.
   * @param[in] listener A <code>PPP_MediaEventsListener_Samsung</code>
   * interface whose methods will be notified about subscribed events.
   * Passing <code>NULL</code> will detach currently set listener.
   * @param[in] user_data A pointer to user data which  will be passed
   * to the listeners during method invocation (optional).
   *
   * @return <code>PP_TRUE</code> if listener has been successfully attached,
   * <code>PP_FALSE</code> otherwise. This method will return false only when
   * called on resource not representing <code>PPB_MediaPlayer_Samsung</code>.
   */
  PP_Bool (*SetMediaEventsListener)(
      PP_Resource player,
      const struct PPP_MediaEventsListener_Samsung_1_0* listener,
      void* user_data);
  /**
   * Attaches <code>PPP_SubtitleListener_Samsung</code> to the player.
   * After attaching plugin is notified about subtitle texts that are parsed by
   * the internal player subtitle parser at the time when they should be
   * displayed. Only notifications regarding texts that originate from active
   * text track are delivered this way.
   * Previously attached listener (if any) is detached.
   *
   * All listeners will be called in the same thread. If this method was
   * invoked in a thread with a running message loop, the listener will run in
   * the current thread. Otherwise, it will run on the main thread.
   *
   * @param[in] player A <code>PP_Resource</code> identifying the media player.
   * @param[in] listener A <code>PPP_SubtitleListener_Samsung</code>
   * interface whose methods will be notified about subscribed events.
   * Passing <code>NULL</code> will detach currently set listener.
   * @param[in] user_data A pointer to user data which  will be passed
   * to the listeners during method invocation (optional).
   *
   * @return <code>PP_TRUE</code> if listener has been successfully attached,
   * <code>PP_FALSE</code> otherwise. This method will return false only when
   * called on resource not representing <code>PPB_MediaPlayer_Samsung</code>.
   */
  PP_Bool (*SetSubtitleListener)(
      PP_Resource player,
      const struct PPP_SubtitleListener_Samsung_1_0* listener,
      void* user_data);
  /**
   * Attaches <code>PPP_BufferingListener_Samsung</code> to the player.
   * After attaching plugin is notified about initial media buffering events.
   * Previously attached listener (if any) is detached.
   *
   * All listeners will be called in the same thread. If this method was
   * invoked in a thread with a running message loop, the listener will run in
   * the current thread. Otherwise, it will run on the main thread.
   *
   * @param[in] player A <code>PP_Resource</code> identifying the media player.
   * @param[in] listener A <code>PPP_BufferingListener_Samsung</code>
   * interface whose methods will be notified about subscribed events.
   * Passing <code>NULL</code> will detach currently set listener.
   * @param[in] user_data A pointer to user data which  will be passed
   * to the listeners during method invocation (optional).
   *
   * @return <code>PP_TRUE</code> if listener has been successfully attached,
   * <code>PP_FALSE</code> otherwise. This method will return false only when
   * called on resource not representing <code>PPB_MediaPlayer_Samsung</code>.
   */
  PP_Bool (*SetBufferingListener)(
      PP_Resource data_source,
      const struct PPP_BufferingListener_Samsung_1_0* listener,
      void* user_data);
  /**
   * Attaches <code>PPP_DRMListener_Samsung</code> to the player.
   * After attaching plugin is notified about DRM related events.
   * Previously attached listener (if any) is detached.
   *
   * All listeners will be called in the same thread. If this method was
   * invoked in a thread with a running message loop, the listener will run in
   * the current thread. Otherwise, it will run on the main thread.
   *
   * @param[in] player A <code>PP_Resource</code> identifying the media player.
   * @param[in] listener A <code>PPP_DRMListener_Samsung</code>
   * interface whose methods will be notified about subscribed events.
   * Passing <code>NULL</code> will detach currently set listener.
   * @param[in] user_data A pointer to user data which  will be passed
   * to the listeners during method invocation (optional).
   *
   * @return <code>PP_TRUE</code> if listener has been successfully attached,
   * <code>PP_FALSE</code> otherwise. This method will return false only when
   * called on resource not representing <code>PPB_MediaPlayer_Samsung</code>.
   */
  PP_Bool (*SetDRMListener)(PP_Resource data_source,
                            const struct PPP_DRMListener_Samsung_1_0* listener,
                            void* user_data);
  /**
   * Attaches given <code>PPB_MediaDataSource</code> to the player.
   *
   * You can pass a <code>NULL</code> resource as buffer to detach currently
   * attached data source. Reattaching data source will return
   * <code>PP_OK</code> and do nothing.
   *
   * Attached data source must be valid. Otherwise
   * <code>PP_ERROR_BADARGUMENT</code> will be returned. Attaching data source
   * to the player will cause:
   * 1. Detaching currently attached data source
   * 2. Performing initialization of newly bound data source (this step
   *    is specific to data source which is being bound).
   *
   * @param[in] player A <code>PP_Resource</code> identifying the media player.
   * @param[in] data_source A <code>PP_Resource</code> identifying data source
   * to be attached to the player.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion.
   *
   * @return PP_OK on success, otherwise an error code from
   * <code>pp_errors.h</code>.
   */
  int32_t (*AttachDataSource)(PP_Resource player,
                              PP_Resource data_source,
                              struct PP_CompletionCallback callback);
  /* Playback control */
  /**
   * Requests to start playback of media from data source attached to the
   * media player.
   *
   * @param[in] player A <code>PP_Resource</code> identifying the media player.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion.
   *
   * @return PP_OK on success, otherwise an error code from
   * <code>pp_errors.h</code>. Meaning of errors:
   * - <code>PP_ERROR_BADARGUMENT</code> - if no data source is connected
   *   to the media player.
   * - <code>PP_ERROR_NOTSUPPORTED</code> - if given operation is not
   *   supported by attached data source.
   */
  int32_t (*Play)(PP_Resource player, struct PP_CompletionCallback callback);
  /**
   * Requests to pause playback of media from data source attached to the
   * media player.
   *
   * @param[in] player A <code>PP_Resource</code> identifying the media player.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion.
   *
   * @return PP_OK on success, otherwise an error code from
   * <code>pp_errors.h</code>. Meaning of errors:
   * - <code>PP_ERROR_BADARGUMENT</code> - if no data source is connected
   *   to the media player.
   * - <code>PP_ERROR_NOTSUPPORTED</code> - if given operation is not
   *   supported by attached data source.
   */
  int32_t (*Pause)(PP_Resource player, struct PP_CompletionCallback callback);
  /**
   * Requests to stop playback of media from data source attached to the
   * media player.
   *
   * @param[in] player A <code>PP_Resource</code> identifying the media player.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion.
   *
   * @return PP_OK on success, otherwise an error code from
   * <code>pp_errors.h</code>. Meaning of errors:
   * - <code>PP_ERROR_BADARGUMENT</code> - if no data source is connected
   *   to the media player.
   * - <code>PP_ERROR_NOTSUPPORTED</code> - if given operation is not
   *   supported by attached data source.
   */
  int32_t (*Stop)(PP_Resource player, struct PP_CompletionCallback callback);
  /**
   * Requests to seek media from attached data source to the given time stamp.
   * After calling Seek new packets should be sent to the player. Only after
   * receiving a number of packets the player can complete a seek operation and
   * run a callback.
   *
   * @param[in] player A <code>PP_Resource</code> identifying the media player.
   * @param[in] time A time stamp from begging of the clip to from which
   * playback should be resumed.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion.
   *
   * @return PP_OK on success, otherwise an error code from
   * <code>pp_errors.h</code>. Meaning of errors:
   * - <code>PP_ERROR_BADARGUMENT</code> - if no data source is connected
   *   to the media player.
   * - <code>PP_ERROR_NOTSUPPORTED</code> - if given operation is not
   *   supported by attached data source.
   */
  int32_t (*Seek)(PP_Resource player,
                  PP_TimeTicks time,
                  struct PP_CompletionCallback callback);
  /**
   * Sets playback rate, pass:
   * |rate| == 1.0      to mark normal playback
   * 0.0 < |rate| < 1.0 to mark speeds slower than normal
   * |rate| > 1.0       to mark speeds faster than normal
   *
   * @param[in] player A <code>PP_Resource</code> identifying the media player.
   * @param[in] rate A rate of the playback.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion.
   *
   * @return PP_OK on success, otherwise an error code from
   * <code>pp_errors.h</code>. Meaning of errors:
   * - <code>PP_ERROR_BADARGUMENT</code> - if no data source is connected
   *   to the media player.
   * - <code>PP_ERROR_NOTSUPPORTED</code> - if given operation is not
   *   supported by attached data source.
   */
  int32_t (*SetPlaybackRate)(PP_Resource player,
                             double rate,
                             struct PP_CompletionCallback callback);
  /* Playback time info */
  /**
   * Retrieves duration of the media played from attached data source.
   *
   * @param[in] player A <code>PP_Resource</code> identifying the media player.
   * @param[out] duration Retrieved duration of the media.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion.
   *
   * @return PP_OK on success, otherwise an error code from
   * <code>pp_errors.h</code>. Meaning of errors:
   * - <code>PP_ERROR_BADARGUMENT</code> - if no data source is connected
   *   to the media player.
   * - <code>PP_ERROR_NOTSUPPORTED</code> - if given operation is not
   *   supported by attached data source (e.g. live content playback).
   */
  int32_t (*GetDuration)(PP_Resource player,
                         PP_TimeDelta* duration,
                         struct PP_CompletionCallback callback);
  /**
   * Retrieves current time/position of the media played from attached
   * data source.
   *
   * This operation can be performed only for media player in
   * <code>PP_MEDIAPLAYERSTATE_PLAYING</code> or
   * <code>PP_MEDIAPLAYERSTATE_PAUSED</code> states.
   *
   * @param[in] player A <code>PP_Resource</code> identifying the media player.
   * @param[out] duration Retrieved current time/position of the media.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion.
   *
   * @return PP_OK on success, otherwise an error code from
   * <code>pp_errors.h</code>. Meaning of errors:
   * - <code>PP_ERROR_BADARGUMENT</code> - if no data source is connected
   *   to the media player.
   * - <code>PP_ERROR_NOTSUPPORTED</code> - if data can't be retrieved
   *   due to invalid player state.
   */
  int32_t (*GetCurrentTime)(PP_Resource player,
                            PP_TimeTicks* time,
                            struct PP_CompletionCallback callback);
  /* Playback state info */
  /**
   * Retrieves current state the media player.
   *
   * @param[in] player A <code>PP_Resource</code> identifying the media player.
   * @param[out] state Retrieved current state of the media player.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion.
   *
   * @return PP_OK on success, otherwise an error code from
   * <code>pp_errors.h</code>.
   */
  int32_t (*GetPlayerState)(PP_Resource player,
                            PP_MediaPlayerState* state,
                            struct PP_CompletionCallback callback);
  /* Tracks info */
  /**
   * Retrieves information of current video track from the media played from
   * attached data source.
   *
   * @param[in] player A <code>PP_Resource</code> identifying the media player.
   * @param[out] track_info Retrieved video track information.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion.
   *
   * @return PP_OK on success, otherwise an error code from
   * <code>pp_errors.h</code>. Meaning of errors:
   * - <code>PP_ERROR_BADARGUMENT</code> - if no data source is connected
   *   to the media player.
   * - <code>PP_ERROR_NOTSUPPORTED</code> - if no video track is available
   *   in media played from attached data source.
   */
  int32_t (*GetCurrentVideoTrackInfo)(PP_Resource player,
                                      struct PP_VideoTrackInfo* track_info,
                                      struct PP_CompletionCallback callback);
  /**
   * Retrieves information of all video tracks from the media played from
   * attached data source.
   *
   * @param[in] player A <code>PP_Resource</code> identifying the media player.
   * @param[in] output A <code>PP_ArrayOutput</code> to receive list of the
   * supported <code>PP_VideoTrackInfo</code> tracks descriptions.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion.
   *
   * @return If >= 0, the number of the tracks is returned, otherwise an
   * error code from <code>pp_errors.h</code>. Meaning of errors:
   * - <code>PP_ERROR_BADARGUMENT</code> - if no data source is connected
   *   to the media player.
   */
  int32_t (*GetVideoTracksList)(PP_Resource player,
                                struct PP_ArrayOutput output,
                                struct PP_CompletionCallback callback);
  /**
   * Retrieves information of current audio track from the media played from
   * attached data source.
   *
   * @param[in] player A <code>PP_Resource</code> identifying the media player.
   * @param[out] track_info Retrieved audio track information.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion.
   *
   * @return PP_OK on success, otherwise an error code from
   * <code>pp_errors.h</code>. Meaning of errors:
   * - <code>PP_ERROR_BADARGUMENT</code> - if no data source is connected
   *   to the media player.
   * - <code>PP_ERROR_NOTSUPPORTED</code> - if no audio track is available
   *   in media played from attached data source.
   */
  int32_t (*GetCurrentAudioTrackInfo)(PP_Resource player,
                                      struct PP_AudioTrackInfo* track_info,
                                      struct PP_CompletionCallback callback);
  /**
   * Retrieves information of all audio tracks from the media played from
   * attached data source.
   *
   * @param[in] player A <code>PP_Resource</code> identifying the media player.
   * @param[in] output A <code>PP_ArrayOutput</code> to receive list of the
   * supported <code>PP_AudioTrackInfo</code> tracks descriptions.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion.
   *
   * @return If >= 0, the number of the tracks is returned, otherwise an
   * error code from <code>pp_errors.h</code>. Meaning of errors:
   * - <code>PP_ERROR_BADARGUMENT</code> - if no data source is connected
   *   to the media player.
   */
  int32_t (*GetAudioTracksList)(PP_Resource player,
                                struct PP_ArrayOutput output,
                                struct PP_CompletionCallback callback);
  /**
   * Retrieves information of current text/subtitles track from the media played
   * from attached data source.
   *
   * @param[in] player A <code>PP_Resource</code> identifying the media player.
   * @param[out] track_info Retrieved video track information.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion.
   *
   * @return PP_OK on success, otherwise an error code from
   * <code>pp_errors.h</code>. Meaning of errors:
   * - <code>PP_ERROR_BADARGUMENT</code> - if no data source is connected
   *   to the media player.
   * - <code>PP_ERROR_NOTSUPPORTED</code> - if no text/subtitles track
   *   is available in media played from attached data source.
   */
  int32_t (*GetCurrentTextTrackInfo)(PP_Resource player,
                                     struct PP_TextTrackInfo* track_info,
                                     struct PP_CompletionCallback callback);
  /**
   * Retrieves information of all text/subtitles tracks from the media played
   * from attached data source.
   *
   * @param[in] player A <code>PP_Resource</code> identifying the media player.
   * @param[in] output A <code>PP_ArrayOutput</code> to receive list of the
   * supported <code>PP_TextTrackInfo</code> tracks descriptions.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion.
   *
   * @return If >= 0, the number of the tracks is returned, otherwise an
   * error code from <code>pp_errors.h</code>. Meaning of errors:
   * - <code>PP_ERROR_BADARGUMENT</code> - if no data source is connected
   *   to the media player.
   */
  int32_t (*GetTextTracksList)(PP_Resource player,
                               struct PP_ArrayOutput output,
                               struct PP_CompletionCallback callback);
  /**
   * Selects a track for the given stream type to be activated for media
   * played from the attached data source.
   *
   * Remarks:
   * If activated track is a text track, it will be automatically activated and
   * therefore it's subtitles will be delivered as events to the
   * <code>PPP_SubtitleListener_Samsung</code>.
   *
   * Constraints:
   * An ability to handle multiple video tracks at a time is not guaranteed to
   * be supported on all platforms, therefore calling this method with a
   * <code>PP_ElementaryStream_Type_Samsung_VIDEO</code> as a stream type may
   * result with a <code>PP_ERROR_NOTSUPPORTED</code> error code.
   *
   * @param[in] player A <code>PP_Resource</code> identifying the media player.
   * @param[in] track_type A type of the stream for which activate track.
   * @param[in] track_index An index of the track which has to be activated.
   * Valid track index can be obtained from one of PP_*TrackInfo structures
   * returned by corresponding call to Get*TracksInfo.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion.
   *
   * @return  PP_OK on success, otherwise an error code from
   * <code>pp_errors.h</code>. Meaning of errors:
   * - <code>PP_ERROR_BADARGUMENT</code> - if no data source is connected
   *   to the media player or passed |track_type| or |track_index| are
   *   not valid.
   * - <code>PP_ERROR_NOTSUPPORTED</code> - if called with
   *   <code>PP_ElementaryStream_Type_Samsung_VIDEO</code> on a platform that
   *   supports only one video track at a time.
   */
  int32_t (*SelectTrack)(PP_Resource player,
                         PP_ElementaryStream_Type_Samsung track_type,
                         uint32_t track_index,
                         struct PP_CompletionCallback callback);
  /* Subtitles (text track) control */
  /**
   * Adds external subtitles.
   * Returns <code>PP_OK/code> in case of success and writes added text track
   * information to |subtitles| param. After that newly added subtitles will be
   * activated and <code>PPB_SubtitleListener_Samsung</code> will be
   * notified about it's texts at the time those texts should be shown.
   *
   * Please note that player is responsible only for subtitle file parsing. No
   * subtitles are displayed by the player. Application can use
   * <code>PPB_SubtitleListener_Samsung</code> to get subtitle texts at correct
   * playback times and display them manually.
   *
   * Constraints:
   * Ability to add external subtitles after attaching data source is
   * implementation dependent. If it is impossible to add external subtitles
   * after data source is attached, this method will fail with
   * <code>PP_ERROR_NOTSUPPORTED</code> error code. Therefore this method is
   * guaranteed to succeed only before data source is attached.
   *
   * @param[in] player A <code>PP_Resource</code> identifying the media player.
   * @param[in] file_path A path of the subtitles.
   * @param[in] encoding Subtitle encoding. May be NULL or empty string, which
   * will cause subtitles to be interpreted as UTF-8 text.
   * @param[out] subtitles Added text/subtitles track information.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion.
   *
   * @return PP_OK on success, otherwise an error code from
   * <code>pp_errors.h</code>. Meaning of errors:
   * - <code>PP_ERROR_FILENOTFOUND</code> - if provided file_path is invalid.
   * - <code>PP_ERROR_BADARGUMENT</code> - if provided encoding is invalid.
   * - <code>PP_ERROR_NOTSUPPORTED</code> - if method was called after attaching
   *   a data source and such operation is not supported on the current
   *   implementation.
   */
  int32_t (*AddExternalSubtitles)(PP_Resource player,
                                  const char* file_path,
                                  const char* encoding,
                                  struct PP_TextTrackInfo* subtitles,
                                  struct PP_CompletionCallback callback);
  /**
   * Sets subtitles (text stream) <code>PPP_SubtitleListener_Samsung</code>
   * event emission delay regarding to the current media time.
   *
   * @param[in] player A <code>PP_Resource</code> identifying the media player.
   * @param[in] delay A delay to be set.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion.
   *
   * @return PP_OK on success, otherwise an error code from
   * <code>pp_errors.h</code>. Meaning of errors:
   * - <code>PP_ERROR_BADARGUMENT</code> - if no data source is connected
   *   to the media player.
   */
  int32_t (*SetSubtitlesDelay)(PP_Resource player,
                               PP_TimeDelta delay,
                               struct PP_CompletionCallback callback);
  /* Other - player control */
  /**
   * Sets display region in which video will be displayed. Passed position
   * is relative to the embed/object element of WebPage associated with given
   * plugin.
   *
   * @param[in] player A <code>PP_Resource</code> identifying the media player.
   * @param[in] rect Video region in which video will be displayed.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion.
   *
   * @return PP_OK on success, otherwise an error code from
   * <code>pp_errors.h</code>.
   */
  int32_t (*SetDisplayRect)(PP_Resource player,
                            const struct PP_Rect* rect,
                            struct PP_CompletionCallback callback);
  /**
   * Sets a display mode which will be used by player. If no display mode is
   * set, <code>PP_MEDIAPLAYERDISPLAYMODE_STRETCH</code> is used.
   *
   * @param[in] player A <code>PP_Resource</code> identifying the media player.
   * @param[in] display_mode A display mode to be used. See <code>
   * PP_MediaPlayerDisplayMode</code> enum for possible options.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion.
   *
   * @return PP_OK on success, otherwise an error code from
   * <code>pp_errors.h</code>.
   */
  int32_t (*SetDisplayMode)(PP_Resource player,
                            PP_MediaPlayerDisplayMode display_mode,
                            struct PP_CompletionCallback callback);
  /**
   * Calls DRM system specific operation.
   *
   * @param[in] player A <code>PP_Resource</code> identifying the media player.
   * @param[in] drm_type A DRM system to be used
   * @param[in] drm_operation A DRM specific operation to be performed.
   * @param[in] drm_data_size A size of data buffer passed to DRM system.
   * @param[in] drm_data A data buffer passed to DRM system.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion.
   *
   * @return PP_OK on success, otherwise an error code from
   * <code>pp_errors.h</code>. Meaning of errors:
   * - <code>PP_ERROR_BADARGUMENT</code> - if no data source is connected
   *   to the media player.
   */
  int32_t (*SetDRMSpecificData)(PP_Resource player,
                                PP_MediaPlayerDRMType drm_type,
                                PP_MediaPlayerDRMOperation drm_operation,
                                uint32_t drm_data_size,
                                const void* drm_data,
                                struct PP_CompletionCallback callback);
  /*
    * Set Vr360 mode.
    * This function should be called in player idle state.
    *
    * @param[in] player A <code>PP_Resource</code> identifying the media player.
    * @param[in] vr360_mode <code>PP_MediaPlayerVr360Mode</code>
    * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
    * completion.
    *
    * @return PP_OK on success, otherwise an error code from
    * <code>pp_errors.h</code>. Meaning of errors:
    * - <code>PP_ERROR_BADARGUMENT</code> - if parameter was wrong.
    * - <code>PP_ERROR_FAILED</code> - if player was in invalid state.
    */
  int32_t (*SetVr360Mode)(PP_Resource player,
                          PP_MediaPlayerVr360Mode vr360_mode,
                          struct PP_CompletionCallback callback);
  /*
   * Set Vr360 rotation.
   *
   * @param[in] player A <code>PP_Resource</code> identifying the media player.
   * @param[in] horizontal_angle - horizontal angle of rotation.
   * @param[in] vertical_angle - vertical angle of rotation.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion.
   *
   * @return PP_OK on success, otherwise an error code from
   * <code>pp_errors.h</code>. Meaning of errors:
   * - <code>PP_ERROR_BADARGUMENT</code> - if parameter was wrong.
   */
  int32_t (*SetVr360Rotation)(PP_Resource player,
                              float horizontal_angle,
                              float vertical_angle,
                              struct PP_CompletionCallback callback);
  /*
   * Set Vr360 zoom.
   *
   * @param[in] player A <code>PP_Resource</code> identifying the media player.
   * @param[in] zoom - zoom level.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion.
   *
   * @return PP_OK on success, otherwise an error code from
   * <code>pp_errors.h</code>. Meaning of errors:
   * - <code>PP_ERROR_BADARGUMENT</code> - if parameter was wrong.
   */
  int32_t (*SetVr360ZoomLevel)(PP_Resource player,
                               uint32_t zoom_level,
                               struct PP_CompletionCallback callback);
};

typedef struct PPB_MediaPlayer_Samsung_1_4 PPB_MediaPlayer_Samsung;

struct PPB_MediaPlayer_Samsung_1_0 {
  PP_Resource (*Create)(PP_Instance instance);
  PP_Bool (*IsMediaPlayer)(PP_Resource controller);
  PP_Bool (*SetMediaEventsListener)(
      PP_Resource player,
      const struct PPP_MediaEventsListener_Samsung_1_0* listener,
      void* user_data);
  PP_Bool (*SetSubtitleListener)(
      PP_Resource player,
      const struct PPP_SubtitleListener_Samsung_1_0* listener,
      void* user_data);
  PP_Bool (*SetBufferingListener)(
      PP_Resource data_source,
      const struct PPP_BufferingListener_Samsung_1_0* listener,
      void* user_data);
  PP_Bool (*SetDRMListener)(PP_Resource data_source,
                            const struct PPP_DRMListener_Samsung_1_0* listener,
                            void* user_data);
  int32_t (*AttachDataSource)(PP_Resource player,
                              PP_Resource data_source,
                              struct PP_CompletionCallback callback);
  int32_t (*Play)(PP_Resource player, struct PP_CompletionCallback callback);
  int32_t (*Pause)(PP_Resource player, struct PP_CompletionCallback callback);
  int32_t (*Stop)(PP_Resource player, struct PP_CompletionCallback callback);
  int32_t (*Seek)(PP_Resource player,
                  PP_TimeTicks time,
                  struct PP_CompletionCallback callback);
  int32_t (*SetPlaybackRate)(PP_Resource player,
                             double rate,
                             struct PP_CompletionCallback callback);
  int32_t (*GetDuration)(PP_Resource player,
                         PP_TimeDelta* duration,
                         struct PP_CompletionCallback callback);
  int32_t (*GetCurrentTime)(PP_Resource player,
                            PP_TimeTicks* time,
                            struct PP_CompletionCallback callback);
  int32_t (*GetPlayerState)(PP_Resource player,
                            PP_MediaPlayerState* state,
                            struct PP_CompletionCallback callback);
  int32_t (*GetCurrentVideoTrackInfo)(PP_Resource player,
                                      struct PP_VideoTrackInfo* track_info,
                                      struct PP_CompletionCallback callback);
  int32_t (*GetVideoTracksList)(PP_Resource player,
                                struct PP_ArrayOutput output,
                                struct PP_CompletionCallback callback);
  int32_t (*GetCurrentAudioTrackInfo)(PP_Resource player,
                                      struct PP_AudioTrackInfo* track_info,
                                      struct PP_CompletionCallback callback);
  int32_t (*GetAudioTracksList)(PP_Resource player,
                                struct PP_ArrayOutput output,
                                struct PP_CompletionCallback callback);
  int32_t (*GetCurrentTextTrackInfo)(PP_Resource player,
                                     struct PP_TextTrackInfo* track_info,
                                     struct PP_CompletionCallback callback);
  int32_t (*GetTextTracksList)(PP_Resource player,
                               struct PP_ArrayOutput output,
                               struct PP_CompletionCallback callback);
  int32_t (*SelectTrack)(PP_Resource player,
                         PP_ElementaryStream_Type_Samsung track_type,
                         uint32_t track_index,
                         struct PP_CompletionCallback callback);
  int32_t (*AddExternalSubtitles)(PP_Resource player,
                                  const char* file_path,
                                  const char* encoding,
                                  struct PP_TextTrackInfo* subtitles,
                                  struct PP_CompletionCallback callback);
  int32_t (*SetSubtitlesDelay)(PP_Resource player,
                               PP_TimeDelta delay,
                               struct PP_CompletionCallback callback);
  int32_t (*SetDisplayRect)(PP_Resource player,
                            const struct PP_Rect* rect,
                            struct PP_CompletionCallback callback);
  int32_t (*SetDRMSpecificData)(PP_Resource player,
                                PP_MediaPlayerDRMType drm_type,
                                PP_MediaPlayerDRMOperation drm_operation,
                                uint32_t drm_data_size,
                                const void* drm_data,
                                struct PP_CompletionCallback callback);
};

struct PPB_MediaPlayer_Samsung_1_1 {
  PP_Resource (*Create)(PP_Instance instance);
  PP_Resource (*CreateWithoutBindingToInstance)(PP_Instance instance);
  PP_Bool (*IsMediaPlayer)(PP_Resource controller);
  PP_Bool (*SetMediaEventsListener)(
      PP_Resource player,
      const struct PPP_MediaEventsListener_Samsung_1_0* listener,
      void* user_data);
  PP_Bool (*SetSubtitleListener)(
      PP_Resource player,
      const struct PPP_SubtitleListener_Samsung_1_0* listener,
      void* user_data);
  PP_Bool (*SetBufferingListener)(
      PP_Resource data_source,
      const struct PPP_BufferingListener_Samsung_1_0* listener,
      void* user_data);
  PP_Bool (*SetDRMListener)(PP_Resource data_source,
                            const struct PPP_DRMListener_Samsung_1_0* listener,
                            void* user_data);
  int32_t (*AttachDataSource)(PP_Resource player,
                              PP_Resource data_source,
                              struct PP_CompletionCallback callback);
  int32_t (*Play)(PP_Resource player, struct PP_CompletionCallback callback);
  int32_t (*Pause)(PP_Resource player, struct PP_CompletionCallback callback);
  int32_t (*Stop)(PP_Resource player, struct PP_CompletionCallback callback);
  int32_t (*Seek)(PP_Resource player,
                  PP_TimeTicks time,
                  struct PP_CompletionCallback callback);
  int32_t (*SetPlaybackRate)(PP_Resource player,
                             double rate,
                             struct PP_CompletionCallback callback);
  int32_t (*GetDuration)(PP_Resource player,
                         PP_TimeDelta* duration,
                         struct PP_CompletionCallback callback);
  int32_t (*GetCurrentTime)(PP_Resource player,
                            PP_TimeTicks* time,
                            struct PP_CompletionCallback callback);
  int32_t (*GetPlayerState)(PP_Resource player,
                            PP_MediaPlayerState* state,
                            struct PP_CompletionCallback callback);
  int32_t (*GetCurrentVideoTrackInfo)(PP_Resource player,
                                      struct PP_VideoTrackInfo* track_info,
                                      struct PP_CompletionCallback callback);
  int32_t (*GetVideoTracksList)(PP_Resource player,
                                struct PP_ArrayOutput output,
                                struct PP_CompletionCallback callback);
  int32_t (*GetCurrentAudioTrackInfo)(PP_Resource player,
                                      struct PP_AudioTrackInfo* track_info,
                                      struct PP_CompletionCallback callback);
  int32_t (*GetAudioTracksList)(PP_Resource player,
                                struct PP_ArrayOutput output,
                                struct PP_CompletionCallback callback);
  int32_t (*GetCurrentTextTrackInfo)(PP_Resource player,
                                     struct PP_TextTrackInfo* track_info,
                                     struct PP_CompletionCallback callback);
  int32_t (*GetTextTracksList)(PP_Resource player,
                               struct PP_ArrayOutput output,
                               struct PP_CompletionCallback callback);
  int32_t (*SelectTrack)(PP_Resource player,
                         PP_ElementaryStream_Type_Samsung track_type,
                         uint32_t track_index,
                         struct PP_CompletionCallback callback);
  int32_t (*AddExternalSubtitles)(PP_Resource player,
                                  const char* file_path,
                                  const char* encoding,
                                  struct PP_TextTrackInfo* subtitles,
                                  struct PP_CompletionCallback callback);
  int32_t (*SetSubtitlesDelay)(PP_Resource player,
                               PP_TimeDelta delay,
                               struct PP_CompletionCallback callback);
  int32_t (*SetDisplayRect)(PP_Resource player,
                            const struct PP_Rect* rect,
                            struct PP_CompletionCallback callback);
  int32_t (*SetDRMSpecificData)(PP_Resource player,
                                PP_MediaPlayerDRMType drm_type,
                                PP_MediaPlayerDRMOperation drm_operation,
                                uint32_t drm_data_size,
                                const void* drm_data,
                                struct PP_CompletionCallback callback);
};

struct PPB_MediaPlayer_Samsung_1_2 {
  PP_Resource (*Create)(PP_Instance instance);
  PP_Resource (*CreateWithOptions)(PP_Instance instance,
                                   PP_MediaPlayerMode player_mode,
                                   PP_BindToInstanceMode bind_mode);
  PP_Bool (*IsMediaPlayer)(PP_Resource controller);
  PP_Bool (*SetMediaEventsListener)(
      PP_Resource player,
      const struct PPP_MediaEventsListener_Samsung_1_0* listener,
      void* user_data);
  PP_Bool (*SetSubtitleListener)(
      PP_Resource player,
      const struct PPP_SubtitleListener_Samsung_1_0* listener,
      void* user_data);
  PP_Bool (*SetBufferingListener)(
      PP_Resource data_source,
      const struct PPP_BufferingListener_Samsung_1_0* listener,
      void* user_data);
  PP_Bool (*SetDRMListener)(PP_Resource data_source,
                            const struct PPP_DRMListener_Samsung_1_0* listener,
                            void* user_data);
  int32_t (*AttachDataSource)(PP_Resource player,
                              PP_Resource data_source,
                              struct PP_CompletionCallback callback);
  int32_t (*Play)(PP_Resource player, struct PP_CompletionCallback callback);
  int32_t (*Pause)(PP_Resource player, struct PP_CompletionCallback callback);
  int32_t (*Stop)(PP_Resource player, struct PP_CompletionCallback callback);
  int32_t (*Seek)(PP_Resource player,
                  PP_TimeTicks time,
                  struct PP_CompletionCallback callback);
  int32_t (*SetPlaybackRate)(PP_Resource player,
                             double rate,
                             struct PP_CompletionCallback callback);
  int32_t (*GetDuration)(PP_Resource player,
                         PP_TimeDelta* duration,
                         struct PP_CompletionCallback callback);
  int32_t (*GetCurrentTime)(PP_Resource player,
                            PP_TimeTicks* time,
                            struct PP_CompletionCallback callback);
  int32_t (*GetPlayerState)(PP_Resource player,
                            PP_MediaPlayerState* state,
                            struct PP_CompletionCallback callback);
  int32_t (*GetCurrentVideoTrackInfo)(PP_Resource player,
                                      struct PP_VideoTrackInfo* track_info,
                                      struct PP_CompletionCallback callback);
  int32_t (*GetVideoTracksList)(PP_Resource player,
                                struct PP_ArrayOutput output,
                                struct PP_CompletionCallback callback);
  int32_t (*GetCurrentAudioTrackInfo)(PP_Resource player,
                                      struct PP_AudioTrackInfo* track_info,
                                      struct PP_CompletionCallback callback);
  int32_t (*GetAudioTracksList)(PP_Resource player,
                                struct PP_ArrayOutput output,
                                struct PP_CompletionCallback callback);
  int32_t (*GetCurrentTextTrackInfo)(PP_Resource player,
                                     struct PP_TextTrackInfo* track_info,
                                     struct PP_CompletionCallback callback);
  int32_t (*GetTextTracksList)(PP_Resource player,
                               struct PP_ArrayOutput output,
                               struct PP_CompletionCallback callback);
  int32_t (*SelectTrack)(PP_Resource player,
                         PP_ElementaryStream_Type_Samsung track_type,
                         uint32_t track_index,
                         struct PP_CompletionCallback callback);
  int32_t (*AddExternalSubtitles)(PP_Resource player,
                                  const char* file_path,
                                  const char* encoding,
                                  struct PP_TextTrackInfo* subtitles,
                                  struct PP_CompletionCallback callback);
  int32_t (*SetSubtitlesDelay)(PP_Resource player,
                               PP_TimeDelta delay,
                               struct PP_CompletionCallback callback);
  int32_t (*SetDisplayRect)(PP_Resource player,
                            const struct PP_Rect* rect,
                            struct PP_CompletionCallback callback);
  int32_t (*SetDRMSpecificData)(PP_Resource player,
                                PP_MediaPlayerDRMType drm_type,
                                PP_MediaPlayerDRMOperation drm_operation,
                                uint32_t drm_data_size,
                                const void* drm_data,
                                struct PP_CompletionCallback callback);
};

struct PPB_MediaPlayer_Samsung_1_3 {
  PP_Resource (*Create)(PP_Instance instance);
  PP_Resource (*CreateWithOptions)(PP_Instance instance,
                                   PP_MediaPlayerMode player_mode,
                                   PP_BindToInstanceMode bind_mode);
  PP_Bool (*IsMediaPlayer)(PP_Resource controller);
  PP_Bool (*SetMediaEventsListener)(
      PP_Resource player,
      const struct PPP_MediaEventsListener_Samsung_1_0* listener,
      void* user_data);
  PP_Bool (*SetSubtitleListener)(
      PP_Resource player,
      const struct PPP_SubtitleListener_Samsung_1_0* listener,
      void* user_data);
  PP_Bool (*SetBufferingListener)(
      PP_Resource player,
      const struct PPP_BufferingListener_Samsung_1_0* listener,
      void* user_data);
  PP_Bool (*SetDRMListener)(PP_Resource player,
                            const struct PPP_DRMListener_Samsung_1_0* listener,
                            void* user_data);
  int32_t (*AttachDataSource)(PP_Resource player,
                              PP_Resource data_source,
                              struct PP_CompletionCallback callback);
  int32_t (*Play)(PP_Resource player, struct PP_CompletionCallback callback);
  int32_t (*Pause)(PP_Resource player, struct PP_CompletionCallback callback);
  int32_t (*Stop)(PP_Resource player, struct PP_CompletionCallback callback);
  int32_t (*Seek)(PP_Resource player,
                  PP_TimeTicks time,
                  struct PP_CompletionCallback callback);
  int32_t (*SetPlaybackRate)(PP_Resource player,
                             double rate,
                             struct PP_CompletionCallback callback);
  int32_t (*GetDuration)(PP_Resource player,
                         PP_TimeDelta* duration,
                         struct PP_CompletionCallback callback);
  int32_t (*GetCurrentTime)(PP_Resource player,
                            PP_TimeTicks* time,
                            struct PP_CompletionCallback callback);
  int32_t (*GetPlayerState)(PP_Resource player,
                            PP_MediaPlayerState* state,
                            struct PP_CompletionCallback callback);
  int32_t (*GetCurrentVideoTrackInfo)(PP_Resource player,
                                      struct PP_VideoTrackInfo* track_info,
                                      struct PP_CompletionCallback callback);
  int32_t (*GetVideoTracksList)(PP_Resource player,
                                struct PP_ArrayOutput output,
                                struct PP_CompletionCallback callback);
  int32_t (*GetCurrentAudioTrackInfo)(PP_Resource player,
                                      struct PP_AudioTrackInfo* track_info,
                                      struct PP_CompletionCallback callback);
  int32_t (*GetAudioTracksList)(PP_Resource player,
                                struct PP_ArrayOutput output,
                                struct PP_CompletionCallback callback);
  int32_t (*GetCurrentTextTrackInfo)(PP_Resource player,
                                     struct PP_TextTrackInfo* track_info,
                                     struct PP_CompletionCallback callback);
  int32_t (*GetTextTracksList)(PP_Resource player,
                               struct PP_ArrayOutput output,
                               struct PP_CompletionCallback callback);
  int32_t (*SelectTrack)(PP_Resource player,
                         PP_ElementaryStream_Type_Samsung track_type,
                         uint32_t track_index,
                         struct PP_CompletionCallback callback);
  int32_t (*AddExternalSubtitles)(PP_Resource player,
                                  const char* file_path,
                                  const char* encoding,
                                  struct PP_TextTrackInfo* subtitles,
                                  struct PP_CompletionCallback callback);
  int32_t (*SetSubtitlesDelay)(PP_Resource player,
                               PP_TimeDelta delay,
                               struct PP_CompletionCallback callback);
  int32_t (*SetDisplayRect)(PP_Resource player,
                            const struct PP_Rect* rect,
                            struct PP_CompletionCallback callback);
  int32_t (*SetDisplayMode)(PP_Resource player,
                            PP_MediaPlayerDisplayMode display_mode,
                            struct PP_CompletionCallback callback);
  int32_t (*SetDRMSpecificData)(PP_Resource player,
                                PP_MediaPlayerDRMType drm_type,
                                PP_MediaPlayerDRMOperation drm_operation,
                                uint32_t drm_data_size,
                                const void* drm_data,
                                struct PP_CompletionCallback callback);
};
/**
 * @}
 */

#endif  /* PPAPI_C_SAMSUNG_PPB_MEDIA_PLAYER_SAMSUNG_H_ */

