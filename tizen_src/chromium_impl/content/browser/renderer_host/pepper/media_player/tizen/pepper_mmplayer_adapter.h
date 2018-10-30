// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_MMPLAYER_ADAPTER_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_MMPLAYER_ADAPTER_H_

#include <player.h>
#include <player_internal.h>
#include <player_product.h>
#include <string>

#include <atomic>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_player_adapter_base.h"
#include "media/base/tizen/media_player_efl.h"

namespace content {

class BufferedPacket;
class MMPacketBuffer;

class MMPlayerMediaFormat
    : public base::RefCountedThreadSafe<MMPlayerMediaFormat> {
 public:
  virtual media_format_h GetMediaFormat() = 0;

 protected:
  friend class base::RefCountedThreadSafe<MMPlayerMediaFormat>;
  MMPlayerMediaFormat() = default;
  virtual ~MMPlayerMediaFormat() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(MMPlayerMediaFormat);
};

class MMPlayerAudioFormat : public MMPlayerMediaFormat {
 public:
  static scoped_refptr<MMPlayerAudioFormat> Create(
      const PepperAudioStreamInfo& stream_info);

  media_format_h GetMediaFormat() override;

 private:
  friend class base::RefCountedThreadSafe<MMPlayerAudioFormat>;
  MMPlayerAudioFormat(media_format_h handle,
                      const PepperAudioStreamInfo& stream_info);
  ~MMPlayerAudioFormat() override;

  std::vector<uint8_t> codec_extradata_;
  player_media_stream_audio_extra_info_s extra_info_;
  media::ScopedMediaFormat format_;
};

class MMPlayerVideoFormat : public MMPlayerMediaFormat {
 public:
  static scoped_refptr<MMPlayerVideoFormat> Create(
      const PepperVideoStreamInfo& stream_info);

  media_format_h GetMediaFormat() override;

  // Aspect ratio: width / height
  const PP_FloatSize& media_resolution() const { return media_resolution_; }

  float GetAspectRatio() const;

 private:
  friend class base::RefCountedThreadSafe<MMPlayerVideoFormat>;
  MMPlayerVideoFormat(media_format_h handle,
                      const PepperVideoStreamInfo& stream_info);
  ~MMPlayerVideoFormat() override;

  std::vector<uint8_t> codec_extradata_;
  player_media_stream_video_extra_info_s extra_info_;
  media::ScopedMediaFormat format_;
  PP_FloatSize media_resolution_;
};

class PepperMMPlayerAdapter : public PepperPlayerAdapterBase {
 public:
  PepperMMPlayerAdapter();
  ~PepperMMPlayerAdapter() override;

  // Player preparing
  int32_t Initialize() override;
  int32_t Reset() override;
  int32_t PrepareES() override;
  int32_t PrepareURL(const std::string& url) override;
  int32_t Unprepare() override;

  // Player controllers
  int32_t Play() override;
  int32_t Pause() override;
  int32_t Seek(PP_TimeTicks time,
               const base::Closure& seek_completed_cb) override;
  void SeekOnAppendThread(PP_TimeTicks time) override;
  int32_t SelectTrack(PP_ElementaryStream_Type_Samsung,
                      uint32_t track_index) override;
  int32_t SetPlaybackRate(float rate) override;
  int32_t SubmitEOSPacket(PP_ElementaryStream_Type_Samsung track_type) override;
  int32_t SubmitPacket(const ppapi::ESPacket* packet,
                       PP_ElementaryStream_Type_Samsung track_type) override;
  int32_t SubmitEncryptedPacket(
      const ppapi::ESPacket* packet,
      ScopedHandleAndSize handle,
      PP_ElementaryStream_Type_Samsung track_type) override;
  void Stop(const base::Callback<void(int32_t)>& callback) override;

  // Player setters
  int32_t SetApplicationID(const std::string& application_id) override;
  int32_t SetAudioStreamInfo(const PepperAudioStreamInfo& stream_info) override;
  int32_t SetDisplay(void* display, bool is_windowless) override;
  int32_t SetDisplayRect(const PP_Rect& viewport,
                         const PP_FloatRect& crop_ratio_rect) override;
  int32_t SetDisplayMode(PP_MediaPlayerDisplayMode display_mode) override;
  int32_t SetVr360Mode(PP_MediaPlayerVr360Mode vr360_mode) override;
  int32_t SetVr360Rotation(float horizontal_angle,
                           float vertical_angle) override;
  int32_t SetVr360ZoomLevel(uint32_t zoom_level) override;
  int32_t SetDuration(PP_TimeDelta duration) override;
  int32_t SetExternalSubtitlesPath(const std::string& file_path,
                                   const std::string& encoding) override;
  int32_t SetStreamingProperty(PP_StreamingProperty property,
                               const std::string& data) override;
  int32_t SetSubtitlesDelay(PP_TimeDelta delay) override;
  int32_t SetVideoStreamInfo(const PepperVideoStreamInfo& stream_info) override;

  // Player getters
  int32_t GetAudioTracksList(
      std::vector<PP_AudioTrackInfo>* track_list) override;
  int32_t GetAvailableBitrates(std::string* bitrates) override;
  int32_t GetCurrentTime(PP_TimeTicks* time) override;
  int32_t GetCurrentTrack(PP_ElementaryStream_Type_Samsung stream_type,
                          int32_t* track_index) override;
  int32_t GetDuration(PP_TimeDelta* duration) override;
  int32_t GetPlayerState(PP_MediaPlayerState* state) override;
  int32_t GetStreamingProperty(PP_StreamingProperty property,
                               std::string* property_value) override;
  int32_t GetTextTracksList(std::vector<PP_TextTrackInfo>* track_list) override;
  int32_t GetVideoTracksList(
      std::vector<PP_VideoTrackInfo>* track_list) override;

  bool IsDisplayModeSupported(PP_MediaPlayerDisplayMode display_mode) override;

  // Callback registering
  int32_t SetErrorCallback(
      const base::Callback<void(PP_MediaPlayerError)>& callback) override;

 private:
  void RegisterMediaCallbacks(PP_ElementaryStream_Type_Samsung type) override;
  void ClearMediaCallbacks();

  void RegisterToBufferingEvents(bool should_register) override;
  void RegisterToMediaEvents(bool should_register) override;
  void RegisterToSubtitleEvents(bool should_register) override;

  void OnBufferingProgress(int percent);

  // Must be called when player starts preparing.
  void OnPlayerPreparing();
  // Must be called when player finishes preparing.
  void OnPlayerPrepared(int32_t result);

  void OnBufferUnderrun(PP_ElementaryStream_Type_Samsung stream,
                        unsigned long long bytes);  // NOLINT(runtime/int)
  void OnBufferOverflow(PP_ElementaryStream_Type_Samsung stream);
  void OnInternalBufferFull(PP_ElementaryStream_Type_Samsung stream);

  void SeekCompleted();

  int32_t UnpreparePlayer();
  void StopPlayer();

  static void OnStreamBufferStatus(
      PP_ElementaryStream_Type_Samsung stream,
      player_media_stream_buffer_status_e status,
      unsigned long long bytes,  // NOLINT(runtime/int)
      void* user_data);

  static void OnStreamSeekData(
      PP_ElementaryStream_Type_Samsung stream,
      unsigned long long offset,  // NOLINT(runtime/int)
      void* user_data);

  static void OnAudioBufferSatus(
      player_media_stream_buffer_status_e status,
      unsigned long long bytes,  // NOLINT(runtime/int)
      void* user_data);

  int32_t ValidateSubtitleEncoding(const std::string& encoding);
  int32_t SetSubtitlesEncoding(const std::string& encoding);

  // NOLINTNEXTLINE(runtime/int)
  static void OnAudioSeekData(unsigned long long offset, void* user_data);

  static void OnVideoBufferStatus(
      player_media_stream_buffer_status_e status,
      unsigned long long bytes,  // NOLINT(runtime/int)
      void* user_data);

  // NOLINTNEXTLINE(runtime/int)
  static void OnVideoSeekData(unsigned long long offset, void* user_data);

  // NOLINTNEXTLINE(runtime/int)
  static void OnErrorEvent(int error_code, void* user_data);

  static void OnEventEnded(void* user_data);

  // NOLINTNEXTLINE(runtime/int)
  static void OnSubtitleEvent(unsigned long duration,
                              void* sub,
                              unsigned type,
                              unsigned attri_count,
                              MMSubAttribute** sub_attribute,
                              void* thiz);

  static void OnSeekCompleted(void* user_data);
  static void OnInitializeDone(void* user_data);
  static void OnBufferingEvent(int percent, void* user_data);

  scoped_refptr<MMPlayerMediaFormat> GetMediaFormat(
      PP_ElementaryStream_Type_Samsung stream_type);
  MMPacketBuffer* GetPacketsBuffer(
      PP_ElementaryStream_Type_Samsung stream_type);
  int32_t PushMediaStream(PP_ElementaryStream_Type_Samsung stream_type,
                          std::unique_ptr<BufferedPacket> pkt);
  void AppendBufferedPackets(PP_ElementaryStream_Type_Samsung stream_type,
                             int64_t bytes);
  void FlushPacketsOnAppendThread();

  void FlushPacketsAndUpdateConfigs();

  player_h player_;
  scoped_refptr<MMPlayerAudioFormat> audio_format_;
  scoped_refptr<MMPlayerVideoFormat> video_format_;

  std::unique_ptr<MMPacketBuffer> audio_packets_;
  std::unique_ptr<MMPacketBuffer> video_packets_;

  scoped_refptr<base::SingleThreadTaskRunner> append_packet_runner_;

  // MM Player is not able to unprepare itself if it did not completed
  // prepare operation. In pull mode complete operation is completed only after
  // sending some packets. So when data source is detached before sending
  // packets we destroy and recreate player instead of unpreparing it.
  //
  // Don't change this value explictly; use Set/UnsetPlayerPreparing instead.
  enum class PlayerPreparingState { kNone, kPreparing, kPrepared };
  PlayerPreparingState player_preparing_;

  base::Callback<void(int32_t)> stop_callback_;

  // Sometimes mmplayer won't generate buffering(0), which is translated to
  // buffering started event. This flag helps make sure buffering started will
  // always be sent.
  bool buffering_started_;

  bool audio_media_callback_set_;
  bool video_media_callback_set_;

  std::atomic<bool> packets_flush_requested_;
  player_state_e target_state_after_seek_;
  bool es_mode_;

  PP_MediaPlayerDisplayMode display_mode_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_MMPLAYER_ADAPTER_H_
