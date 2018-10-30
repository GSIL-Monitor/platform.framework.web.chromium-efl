// Copyright (c) 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_CPP_SAMSUNG_MEDIA_PLAYER_SAMSUNG_H_
#define PPAPI_CPP_SAMSUNG_MEDIA_PLAYER_SAMSUNG_H_

#include <string>
#include <vector>

#include "ppapi/c/samsung/ppb_media_player_samsung.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/resource.h"

/// @file
/// This file defines the <code>MediaPlayer_Samsung</code> type, which provides
/// multimedia streaming and playback control capabilities.
///
/// Part of Pepper Media Player interfaces (Samsung's extension).
namespace pp {

class InstanceHandle;
class MediaDataSource_Samsung;

/// <code>MediaPlayer_Samsung</code> is type allowing application
/// to control playback state, inquire about playback state.
/// It's also responsible for assigning data source which will feed player
/// with media data.
///
///  Assumptions:
/// - Video is displayed in area of embed/object element of HTML page
///   in which plugin is embedded.
/// - Video and graphics from module can be displayed simultaneously.
///   (see <code>PPB_Instance.BindGraphics</code>). Application must
///   set proper CSS style for embed element with NaCl application
///   (mostly transparent background).
class MediaPlayer_Samsung : public Resource {
 public:
  /// An enum that controls binding Media Player to pp::Instance in the
  /// constructor. See descriptions of <code>MediaPlayer_Samsung(
  /// const InstanceHandle& instance)</code> and <code>MediaPlayer_Samsung(
  /// const InstanceHandle& instance, DoNotBindToInstance)</code>.
  enum DoNotBindToInstance { DO_NOT_BIND_TO_INSTANCE };

  MediaPlayer_Samsung();

  /// Creates a player bound to pp::Instance.
  explicit MediaPlayer_Samsung(const InstanceHandle& instance);

  /// Creates a player not bound to pp::Instance. It can be useful when the
  /// player is used with other graphic resources that need to be bound to
  /// pp::Instance. See <code>PPB_Instance::BindGraphics</code>
  /// and <code>MediaPlayer_Samsung::BindToInstance</code>.
  MediaPlayer_Samsung(const InstanceHandle& instance, DoNotBindToInstance);

  /// Creates a player with given modes. Default values create a player that can
  /// be used in the most typical scenarios.
  ///
  /// @param[in] player_mode Create a player that is either bound to
  /// instance or not. It can be useful when the player is used with other
  /// graphic resources that need to be bound to <code>pp::Instance</code>. See
  /// <code>PPB_Instance::BindGraphics</code> and
  /// <code>MediaPlayer_Samsung::BindToInstance</code> for more details.
  /// @param[in] bind_mode Create a player that uses a specified playback
  /// mode.
  MediaPlayer_Samsung(const InstanceHandle& instance,
                      PP_MediaPlayerMode player_mode,
                      PP_BindToInstanceMode bind_mode);

  MediaPlayer_Samsung(const MediaPlayer_Samsung& other);

  MediaPlayer_Samsung& operator=(const MediaPlayer_Samsung& other);

  virtual ~MediaPlayer_Samsung();

  /// Binds this resource to the pp::Instance as the current display surface.
  /// It is like <code>PPB_Instance::BindGraphics</code> for media
  /// player resource. Normally, when the constructor is called without
  /// DoNotBindToInstance, there is no need to call this method.
  bool BindToInstance(const InstanceHandle& instance);

  /// Attaches given <code>MediaDataSource_Samsung</code> to the player.
  ///
  /// You can pass a <code>NULL</code> resource as buffer to detach currently
  /// attached data source. Reattaching data source will return
  /// <code>PP_OK</code> and do nothing.
  ///
  /// Attaching data source to the player will cause:
  /// 1. Detaching currently attached data source
  /// 2. Performing initialization of newly bound data source (this step
  ///    is specific to data source which is being bound).
  ///
  /// @param[in] data_source A <code>MediaDataSource_Samsung</code>
  /// identifying data source to be attached to the player.
  /// @param[in] callback A <code>CompletionCallback</code> to be called upon
  /// completion.
  ///
  /// @return PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>.
  int32_t AttachDataSource(
      const MediaDataSource_Samsung& data_source,
      const CompletionCallback& callback);

  /* Playback control */

  /// Requests to start playback of media from data source attached to the
  /// media player.
  ///
  /// @param[in] callback A <code>CompletionCallback</code> to be called upon
  /// completion.
  ///
  /// @return PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>. Meaning of errors:
  /// - <code>PP_ERROR_BADARGUMENT</code> - if no data source is connected
  ///   to the media player.
  /// - <code>PP_ERROR_NOTSUPPORTED</code> - if given operation is not
  ///   supported by attached data source.
  int32_t Play(const CompletionCallback& callback);

  /// Requests to pause playback of media from data source attached to the
  /// media player.
  ///
  /// @param[in] callback A <code>CompletionCallback</code> to be called upon
  /// completion.
  ///
  /// @return PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>. Meaning of errors:
  /// - <code>PP_ERROR_BADARGUMENT</code> - if no data source is connected
  ///   to the media player.
  /// - <code>PP_ERROR_NOTSUPPORTED</code> - if given operation is not
  ///   supported by attached data source.
  int32_t Pause(const CompletionCallback& callback);

  /// Requests to stop playback of media from data source attached to the
  /// media player.
  ///
  /// @param[in] callback A <code>CompletionCallback</code> to be called upon
  /// completion.
  ///
  /// @return PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>. Meaning of errors:
  /// - <code>PP_ERROR_BADARGUMENT</code> - if no data source is connected
  ///   to the media player.
  /// - <code>PP_ERROR_NOTSUPPORTED</code> - if given operation is not
  ///   supported by attached data source.
  int32_t Stop(const CompletionCallback& callback);

  /// Requests to seek media from attached data source to the given time stamp.
  /// After calling Seek new packets should be sent to the player. Only after
  /// receiving a number of packets the player can complete a seek operation and
  /// run a callback.
  ///
  /// @param[in] time A time stamp from begging of the clip to from which
  /// playback should be resumed.
  /// @param[in] callback A <code>CompletionCallback</code> to be called upon
  /// completion.
  ///
  /// @return PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>. Meaning of errors:
  /// - <code>PP_ERROR_BADARGUMENT</code> - if no data source is connected
  ///   to the media player.
  /// - <code>PP_ERROR_NOTSUPPORTED</code> - if given operation is not
  ///   supported by attached data source.
  int32_t Seek(PP_TimeTicks time, const CompletionCallback& callback);

  /// Sets playback rate, pass:
  /// |rate| == 1.0      to mark normal playback
  /// 0.0 < |rate| < 1.0 to mark speeds slower than normal
  /// |rate| > 1.0       to mark speeds faster than normal
  ///
  /// @param[in] rate A rate of the playback.
  /// @param[in] callback A <code>CompletionCallback</code> to be called upon
  /// completion.
  ///
  /// @return PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>. Meaning of errors:
  /// - <code>PP_ERROR_BADARGUMENT</code> - if no data source is connected
  ///   to the media player.
  /// - <code>PP_ERROR_NOTSUPPORTED</code> - if given operation is not
  ///   supported by attached data source.
  int32_t SetPlaybackRate(double rate, const CompletionCallback& callback);

  /* Playback time info */

  /// Retrieves duration of the media played from attached data source.
  ///
  /// @param[in] callback A <code>CompletionCallbackWitOutput</code>
  /// to be called upon completion with retrieved duration of the media.
  ///
  /// @return PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>. Meaning of errors:
  /// - <code>PP_ERROR_BADARGUMENT</code> - if no data source is connected
  ///   to the media player.
  /// - <code>PP_ERROR_NOTSUPPORTED</code> - if given operation is not
  ///   supported by attached data source (e.g. live content playback).
  int32_t GetDuration(
      const CompletionCallbackWithOutput<PP_TimeDelta>& callback);

  /// Retrieves current time/position of the media played from attached
  /// data source.
  ///
  /// This operation can be performed only for media player in
  /// <code>PP_MEDIAPLAYERSTATE_PLAYING</code> or
  /// <code>PP_MEDIAPLAYERSTATE_PAUSED</code> states.
  ///
  /// @param[in] callback A <code>CompletionCallbackWithOutput</code> to be
  /// called upon completion with retrieved current time.
  ///
  /// @return PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>. Meaning of errors:
  /// - <code>PP_ERROR_BADARGUMENT</code> - if no data source is connected
  ///   to the media player.
  /// - <code>PP_ERROR_NOTSUPPORTED</code> - if data can't be retrieved
  ///   due to invalid player state.
  int32_t GetCurrentTime(
      const CompletionCallbackWithOutput<PP_TimeTicks>& callback);

  /* Playback state info */

  /// Retrieves current state the media player.
  ///
  /// @param[in] callback A <code>CompletionCallbackWithOutput</code> to be
  /// called upon completion with retrieved player state.
  ///
  /// @return PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>.
  int32_t GetPlayerState(
      const CompletionCallbackWithOutput<PP_MediaPlayerState>& callback);

  /* Tracks info */

  /// Retrieves information of current video track from the media played from
  /// attached data source.
  ///
  /// @param[in] callback A <code>CompletionCallbackWithOutput</code> to be
  /// called upon completion with retrieved video track information.
  ///
  /// @return PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>. Meaning of errors:
  /// - <code>PP_ERROR_BADARGUMENT</code> - if no data source is connected
  ///   to the media player.
  /// - <code>PP_ERROR_NOTSUPPORTED</code> - if no video track is available
  ///   in media played from attached data source.
  int32_t GetCurrentVideoTrackInfo(
      const CompletionCallbackWithOutput<PP_VideoTrackInfo>& callback);

  typedef std::vector<PP_VideoTrackInfo> VideoTracksList;

  /// Retrieves information of all video tracks from the media played from
  /// attached data source.
  ///
  /// @param[in] callback A <code>CompletionCallbackWithOutput</code> to be
  /// called upon completion with list of video tracks available in currently
  /// attached data source. List will be empty if media played form attached
  /// data source doesn't have any video tracks.
  ///
  /// @return If >= 0, the number of the tracks is returned, otherwise an
  /// error code from <code>pp_errors.h</code>. Meaning of errors:
  /// - <code>PP_ERROR_BADARGUMENT</code> - if no data source is connected
  ///  to the media player.
  int32_t GetVideoTracksList(
      const CompletionCallbackWithOutput<VideoTracksList>& callback);

  /// Retrieves information of current audio track from the media played from
  /// attached data source.
  ///
  /// @param[in] callback A <code>CompletionCallbackWithOutput</code> to be
  /// called upon completion with retrieved audio track information.
  ///
  /// @return PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>. Meaning of errors:
  /// - <code>PP_ERROR_BADARGUMENT</code> - if no data source is connected
  ///   to the media player.
  /// - <code>PP_ERROR_NOTSUPPORTED</code> - if no video track is available
  ///   in media played from attached data source.
  int32_t GetCurrentAudioTrackInfo(
      const CompletionCallbackWithOutput<PP_AudioTrackInfo>& callback);

  typedef std::vector<PP_AudioTrackInfo> AudioTracksList;

  /// Retrieves information of all audio tracks from the media played from
  /// attached data source.
  ///
  /// @param[in] callback A <code>CompletionCallbackWithOutput</code> to be
  /// called upon completion with list of audio tracks available in currently
  /// attached data source. List will be empty if media played form attached
  /// data source doesn't have any audio tracks.
  ///
  /// @return If >= 0, the number of the tracks is returned, otherwise an
  /// error code from <code>pp_errors.h</code>. Meaning of errors:
  /// - <code>PP_ERROR_BADARGUMENT</code> - if no data source is connected
  ///  to the media player.
  int32_t GetAudioTracksList(
      const CompletionCallbackWithOutput<AudioTracksList>& callback);

  /// Retrieves information of current text/subtitles track from the media
  /// played from attached data source.
  ///
  /// @param[in] callback A <code>CompletionCallbackWithOutput</code> to be
  /// called upon completion with retrieved text/subtitles track information.
  ///
  /// @return PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>. Meaning of errors:
  /// - <code>PP_ERROR_BADARGUMENT</code> - if no data source is connected
  ///   to the media player.
  /// - <code>PP_ERROR_NOTSUPPORTED</code> - if no video track is available
  ///   in media played from attached data source.
  int32_t GetCurrentTextTrackInfo(
      const CompletionCallbackWithOutput<PP_TextTrackInfo>& callback);

  typedef std::vector<PP_TextTrackInfo> TextTracksList;

  /// Retrieves information of all text/subtitles tracks from the media played
  /// from attached data source.
  ///
  /// @param[in] callback A <code>CompletionCallbackWithOutput</code> to be
  /// called upon completion with list of text/subtitles tracks available
  /// in currently attached data source. List will be empty if media played
  /// form attached data source doesn't have any text/subtitles tracks.
  ///
  /// @return If >= 0, the number of the tracks is returned, otherwise an
  /// error code from <code>pp_errors.h</code>. Meaning of errors:
  /// - <code>PP_ERROR_BADARGUMENT</code> - if no data source is connected
  ///  to the media player.
  int32_t GetTextTracksList(
      const CompletionCallbackWithOutput<TextTracksList>& callback);

  /// Selects a track for the given stream type to be activated for media
  /// played from the attached data source.
  ///
  /// Remarks:
  /// If activated track is a text track, it will be automatically activated and
  /// therefore it's subtitles will be delivered as events to the
  /// <code>SubtitleListener_Samsung</code>.
  ///
  /// Constraints:
  /// An ability to handle multiple video tracks at a time is not guaranteed to
  /// be supported on all platforms, therefore calling this method with a
  /// <code>PP_ElementaryStream_Type_Samsung_VIDEO</code> as a stream type may
  /// result with a <code>PP_ERROR_NOTSUPPORTED</code> error code.
  ///
  /// @param[in] track_type A type of the stream for which activate track.
  /// @param[in] track_index An index of the track which has to be activated.
  /// Valid track index can be obtained from one of PP_*TrackInfo structures
  /// returned by corresponding call to Get*TracksInfo.
  /// @param[in] callback A <code>CompletionCallback</code> to be called upon
  /// completion.
  ///
  /// @return  PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>. Meaning of errors:
  /// - <code>PP_ERROR_BADARGUMENT</code> - if no data source is connected
  ///   to the media player or passed |track_type| or |track_index| are
  ///   not valid.
  /// - <code>PP_ERROR_NOTSUPPORTED</code> - if called with
  ///   <code>PP_ElementaryStream_Type_Samsung_VIDEO</code> on a platform that
  ///   supports only one video track at a time.
  int32_t SelectTrack(
      PP_ElementaryStream_Type_Samsung track_type,
      uint32_t track_index,
      const CompletionCallback& callback);

  /* Subtitles control */

  /// Adds external subtitles.
  /// Returns <code>PP_OK/code> in case of success and writes added text track
  /// information to |subtitles| param. After that newly added subtitles will be
  /// activated and <code>SubtitleListener_Samsung</code> will be notified about
  /// it's texts at the time those texts should be shown.
  ///
  /// Please note that player is responsible only for subtitle file parsing. No
  /// subtitles are displayed by the player. Application can use
  /// <code>SubtitleListener_Samsung</code> to get subtitle texts at correct
  /// playback times and display them manually.
  ///
  /// Constraints:
  /// Ability to add external subtitles after attaching data source is
  /// implementation dependent. If it is impossible to add external subtitles
  /// after data source is attached, this method will fail with
  /// <code>PP_ERROR_NOTSUPPORTED</code> error code. Therefore this method is
  /// guaranteed to succeed only before data source is attached.
  ///
  /// @param[in] file_path A path of the subtitles.
  /// @param[in] encoding Subtitle encoding. May be empty string, which will
  /// cause subtitles to be interpreted as UTF-8 text.
  /// @param[in] callback A <code>CompletionCallbackWithOutput</code> to be
  /// called upon completion with added subtitles information.
  ///
  /// @return PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>. Meaning of errors:
  /// - <code>PP_ERROR_FILENOTFOUND</code> - if provided file_path is invalid.
  /// - <code>PP_ERROR_BADARGUMENT</code> - if provided encoding is invalid.
  /// - <code>PP_ERROR_NOTSUPPORTED</code> - if method was called after
  ///   attaching a data source and such operation is not supported on the
  ///   current implementation.
  int32_t AddExternalSubtitles(
      const std::string& file_path,
      const std::string& encoding,
      const CompletionCallbackWithOutput<PP_TextTrackInfo>& callback);

  /// Sets subtitles (text stream) <code>SubtitleListener</code>
  /// event emission delay regarding to the current media time.
  ///
  /// @param[in] delay A delay to be set.
  /// @param[in] callback A <code>CompletionCallback</code> to be called upon
  /// completion.
  ///
  /// @return PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>. Meaning of errors:
  /// - <code>PP_ERROR_BADARGUMENT</code> - if no data source is connected
  ///   to the media player.
  int32_t SetSubtitlesDelay(
      PP_TimeDelta delay,
      const CompletionCallback& callback);

  /* Other - player control */

  /// Sets display region in which video will be displayed. Passed position
  /// is relative to the embed/object element of WebPage associated with given
  /// plugin.
  ///
  /// @param[in] rect Video region in which video will be displayed.
  /// @param[in] callback A <code>CompletionCallback</code> to be called upon
  /// completion.
  ///
  /// @return PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>.
  int32_t SetDisplayRect(
      const PP_Rect& rect,
      const CompletionCallback& callback);

  /// Sets a display mode which will be used by player. If no display mode is
  /// set, <code>PP_MEDIAPLAYERDISPLAYMODE_STRETCH</code> is used.
  ///
  /// @param[in] display_mode A display mode to be used. See <code>
  /// PP_MediaPlayerDisplayMode</code> enum for possible options.
  /// @param[in] callback A <code>CompletionCallback</code> to be called upon
  /// completion.
  ///
  /// @return PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>.
  int32_t SetDisplayMode(
      PP_MediaPlayerDisplayMode display_mode,
      const CompletionCallback& callback);

  /* DRM Related */

  /// Calls DRM system specific operation.
  ///
  /// @param[in] drm_type A DRM system to be used
  /// @param[in] drm_operation A DRM specific operation to be performed.
  /// @param[in] drm_data_size A size of data buffer passed to DRM system.
  /// @param[in] drm_data A data buffer passed to DRM system.
  /// @param[in] callback A <code>CompletionCallback</code> to be called upon
  /// completion.
  ///
  /// @return PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>. Meaning of errors:
  /// - <code>PP_ERROR_BADARGUMENT</code> - if no data source is connected
  ///   to the media player.
  int32_t SetDRMSpecificData(
      PP_MediaPlayerDRMType drm_type,
      PP_MediaPlayerDRMOperation drm_operation,
      uint32_t drm_data_size,
      const void* drm_data,
      const CompletionCallback& callback);

  /* Vr360 Control */

  /// Set Vr360 mode.
  /// This function should be called in player
  /// <code>PP_MEDIAPLAYERSTATE_UNINITIALIZED</code> - when data source was not
  /// attached yet.
  ///
  /// @param[in] player A <code>PP_Resource</code> identifying the media player.
  /// @param[in] vr360_mode <code>PP_MediaPlayerVr360Mode</code>
  /// @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
  /// completion.
  ///
  /// @return PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>. Meaning of errors:
  /// - <code>PP_ERROR_BADARGUMENT</code> - if parameter was wrong.
  /// - <code>PP_ERROR_FAILED</code> - if player was is invalid state.
  int32_t SetVr360Mode(PP_MediaPlayerVr360Mode display_mode,
                       const CompletionCallback& callback);

  /// Set Vr360 rotation.
  ///
  /// @param[in] player A <code>PP_Resource</code> identifying the media player.
  /// @param[in] horizontal_angle - horizontal angle of rotation in degree
  /// (0.0f to 360.0f),
  /// @param[in] vertical_angle - vertical angle of rotation in degree
  /// (0.0f to 360.0f),
  /// @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
  /// completion.
  ///
  /// @return PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>. Meaning of errors:
  /// - <code>PP_ERROR_BADARGUMENT</code> - if parameter was wrong.
  int32_t SetVr360Rotation(float horizontal_angle,
                           float vertical_angle,
                           const CompletionCallback& callback);

  /// Set Vr360 zoom.
  ///
  /// @param[in] player A <code>PP_Resource</code> identifying the media player.
  /// @param[in] zoom - zoom level (0 to 100) 0 means max zoom-out,
  /// 100 means max zoom-in.
  /// @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
  /// completion.
  ///
  /// @return PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>. Meaning of errors:
  /// - <code>PP_ERROR_BADARGUMENT</code> - if parameter was wrong.
  int32_t SetVr360ZoomLevel(uint32_t zoom_level,
                            const CompletionCallback& callback);
};

}  // namespace pp

#endif  // PPAPI_CPP_SAMSUNG_MEDIA_PLAYER_SAMSUNG_H_
