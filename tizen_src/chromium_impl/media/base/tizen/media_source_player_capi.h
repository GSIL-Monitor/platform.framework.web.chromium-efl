// Copyright 2015 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_TIZEN_MEDIA_SOURCE_PLAYER_CAPI_H_
#define MEDIA_BASE_TIZEN_MEDIA_SOURCE_PLAYER_CAPI_H_

#include <player.h>

#include "base/callback.h"
#include "base/cancelable_callback.h"
#include "base/location.h"
#include "base/memory/shared_memory_handle.h"
#include "base/static_map.h"
#include "base/threading/thread.h"
#include "base/time/default_tick_clock.h"
#include "base/timer/timer.h"
#include "media/base/decoder_buffer.h"
#include "media/base/tizen/demuxer_efl.h"
#include "media/base/tizen/media_packet_impl.h"
#include "media/base/tizen/media_player_efl.h"
#include "media/base/tizen/media_player_util_efl.h"
#include "third_party/WebKit/public/platform/WebMediaPlayer.h"

#if defined(TIZEN_VIDEO_HOLE)
#include "ui/gfx/geometry/rect_f.h"
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
#include <player_product.h>
#endif

namespace media {

using player_buffer_size_t = unsigned long long;
const size_t kCapiPlayerChannelCount = 2;

class BufferObserver;

// This class handles media source extensions for CAPI port.
class MEDIA_EXPORT MediaSourcePlayerCapi : public MediaPlayerEfl,
                                           public DemuxerEflClient {
 public:
  typedef base::Callback<void(const bool)> CompleteCB;
  typedef std::queue<scoped_refptr<DecoderBuffer>> BufferQueue;

  // Constructs a player with the given ID and demuxer. |manager| must outlive
  // the lifetime of this object.

  // Player Buffer:          0% ---------------- 100%
  // Buffer Status: UNDERRUN | MIN | NORMAL | MAX | OVERFLOW
  // . BUFFER_NONE: Initial state.
  // . BUFFER_EOS: EOS reached.
  enum BufferStatus {
    BUFFER_NONE,
    BUFFER_UNDERRUN,
    BUFFER_MIN_THRESHOLD,
    BUFFER_NORMAL,
    BUFFER_MAX_THRESHOLD,
    BUFFER_OVERFLOW,
    BUFFER_EOS,
  };

  enum SendMediaPacketResult {
    SUCCESS,
    FAILURE,
    BUFFER_SPACE,
  };

  MediaSourcePlayerCapi(int player_id,
                        std::unique_ptr<DemuxerEfl> demuxer,
                        MediaPlayerManager* manager,
                        bool video_hole);

  MediaSourcePlayerCapi(int player_id,
                        std::unique_ptr<DemuxerEfl> demuxer,
                        MediaPlayerManager* manager,
                        BufferObserver* buffer_observer,
#if defined(OS_TIZEN_TV_PRODUCT)
                        player_drm_type_e drm_type,
#endif
                        bool video_hole);

  ~MediaSourcePlayerCapi() override;

  // MediaPlayerEfl implementation.
  void Play() override;
  void Pause(bool is_media_related_action) override;
  void SetRate(double rate) override;
  void Seek(base::TimeDelta time) override;
  void SetVolume(double volume) override;
  base::TimeDelta GetCurrentTime() override;
  void Initialize() override;

  // DemuxerEflClient implementation.
  void OnDemuxerConfigsAvailable(const DemuxerConfigs& params) override;
#if defined(OS_TIZEN_TV_PRODUCT)
  void OnDemuxerStateChanged(const DemuxerConfigs& params) override;
  unsigned GetDroppedFrameCount() override;
  unsigned GetDecodedFrameCount() const override;
  void SetDecryptor(eme::eme_decryptor_t) override;
#endif
  void OnDemuxerDataAvailable(
      base::SharedMemoryHandle foreign_memory_handle,
      const media::DemuxedBufferMetaData& meta_data) override;
  void OnDemuxerSeekDone(const base::TimeDelta& actual_browser_seek_time,
                         const base::TimeDelta& video_key_frame) override;
  void OnDemuxerDurationChanged(base::TimeDelta duration) override;
#if defined(TIZEN_VIDEO_HOLE)
  void SetGeometry(const gfx::RectF&) override;
  void OnWebViewMoved() override;
#endif
  void ConfirmStateChange();

  void EnteredFullscreen() override;
  void ExitedFullscreen() override;

  // To handle CAPI callbacks.
  void OnPlaybackComplete();
  void OnPrepareComplete();
  void OnHandlePlayerError(int player_error_code, const base::Location& loc);
  void OnMediaPacketUpdated(media_packet_h packet);
  bool HasConfigs() const override;

  void Suspend() override;
  void Resume() override;

  PlayerRoleFlags GetRoles() const noexcept override;

 protected:
  void Release() override;
  void OnMediaError(MediaError error_type) override;
  virtual void HandlePrepareComplete();
  virtual bool SetAudioStreamInfo();
  virtual bool SetVideoStreamInfo();
  void PlayInternal();

 private:
  struct ChannelData {
    bool is_eos_ = false;
    bool should_feed_ = false;
    ScopedMediaFormat media_format_;
    BufferQueue buffer_queue_;
  };
  player_state_e GetPlayerState();

  // For internal seeks.
  void SeekInternal();
  void UpdateSeekState(MediaSeekState state);

  void ReadDemuxedDataIfNeed(media::DemuxerStream::Type type);

  void HandleBufferingStatusChanged(media::DemuxerStream::Type type,
                                    BufferStatus status);

  void HandlePlaybackComplete();
  void HandleSeekComplete();
  void SeekDone();

  void OnResumeComplete(bool success);
  void OnInitComplete(bool success);

  void PlaybackComplete();

  void HandlePlayerError(int player_error_code, const base::Location& loc);
  void SendEosToCapi(const media::DemuxedBufferMetaData& meta_data);

  // |current_time_update_timer_| related
  void OnCurrentTimeUpdateTimerFired();
  void StartCurrentTimeUpdateTimer();
  void StopCurrentTimeUpdateTimer();

  ChannelData& GetChannelData(DemuxerStream::Type type);
  const ChannelData& GetChannelData(DemuxerStream::Type type) const;
  bool IsEos(DemuxerStream::Type type) const;
  void SetEos(DemuxerStream::Type type, bool value);
  virtual bool ShouldFeed(DemuxerStream::Type type) const;
  void SetShouldFeed(DemuxerStream::Type type, bool value);
  media_format_s* GetMediaFormat(DemuxerStream::Type type) const;
  void SetMediaFormat(media_format_s* format, DemuxerStream::Type type);
  BufferQueue& GetBufferQueue(DemuxerStream::Type type);
  void ClearBufferQueue(DemuxerStream::Type type);

  void ReadFromQueueIfNeeded(DemuxerStream::Type type);
  bool ReadFromQueue(DemuxerStream::Type type);

  std::unique_ptr<MediaPacket> CreateEosMediaPacket(DemuxerStream::Type type);
  std::unique_ptr<MediaPacket> CreateMediaPacket(
      scoped_refptr<DecoderBuffer> buffer,
      DemuxerStream::Type type);
  std::unique_ptr<MediaPacket> CreateMediaPacket(
      base::SharedMemoryHandle foreign_memory_handle,
      const media::DemuxedBufferMetaData& meta_data);
  SendMediaPacketResult PushMediaPacket(MediaPacket& media_packet);

  void SaveDecoderBuffer(base::SharedMemoryHandle foreign_memory_handle,
                         const media::DemuxedBufferMetaData& meta_data);

  bool Prepare(CompleteCB cb);

  void RunCompleteCB(bool success, const base::Location& from);

  // Returns Max buffer size for audio / video
  player_buffer_size_t GetMaxBufferSize(media::DemuxerStream::Type type,
                                        const DemuxerConfigs& configs);

  void UpdateVideoBufferSize(player_buffer_size_t buffer_size_in_bytes);

  // Set |media_type_| based on Demuxer configs
  void SetMediaType(const DemuxerConfigs& configs);

  static void OnPrepareComplete(void* user_data);
  static void OnSeekComplete(void* user_data);
  static void OnMediaPacketUpdated(media_packet_h packet, void* user_data);
  static void OnPlaybackComplete(void* user_data);
  static void OnPlayerError(int error_code, void* user_data);
  static void OnInterrupted(player_interrupted_code_e code, void* user_data);

#if defined(OS_TIZEN_TV_PRODUCT)
  void UpdateResolution(int width, int height, int media_type);
  void OnPlayerResourceConflict();
#endif

  virtual void StartWaitingForData();
  void FinishWaitingForData();
  void OnDemuxerBufferedChanged(
      const media::Ranges<base::TimeDelta>& ranges) override;
  std::pair<base::TimeDelta, base::TimeDelta> FindBufferedRange(
      base::TimeDelta start,
      base::TimeDelta end) const;
  void UncheckedUpdateReadyState();
  void UpdateReadyState();
  base::TimeDelta CalculateLastFrameDuration() const;
  virtual bool SetPlayerIniParam();

  // CAPI player state handling APIs.
  void ExecuteDelayedPlayerState();
  void SetDelayedPlayerState(player_state_e state);
  bool ChangePlayerState(player_state_e state);

  std::unique_ptr<DemuxerEfl> demuxer_;
  std::unique_ptr<BufferObserver> buffer_observer_;

  player_buffer_size_t max_audio_buffer_size_;
  player_buffer_size_t max_video_buffer_size_;
  const scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  base::TimeDelta duration_;

  bool paused_by_buffering_logic_;

  bool pending_seek_;
  bool pending_seek_on_player_;
  bool seeking_video_pushed_;
  base::TimeDelta pending_demuxer_seek_position_;
  base::TimeDelta pending_seek_offset_;

  // To track CAPI player state change. Will be reset once player state
  // change is success.
  player_state_e player_new_state_;

  // To handle state change requests that has to be executed after completing
  // current task.
  player_state_e delayed_player_state_;

  base::TimeDelta seek_offset_;
  MediaSeekState seek_state_;
#if defined(TIZEN_VIDEO_HOLE)
  std::unique_ptr<VideoPlaneManagerTizen> video_plane_manager_;
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
  player_media_stream_audio_extra_info_s audio_extra_info_;
  player_media_stream_video_extra_info_s video_extra_info_;
  std::vector<uint8_t> video_codec_extra_data_;
  std::vector<uint8_t> audio_codec_extra_data_;
  std::string hdr_info_;
  unsigned decoded_frame_count_;
  base::TimeDelta cached_current_time_;
  int64_t position_before_conflict_;
  player_drm_type_e drm_type_;
#endif

  bool is_demuxer_state_reported_;
  media::Ranges<base::TimeDelta> demuxer_buffered_ranges_;
  base::TimeDelta official_playback_position_;
  blink::WebMediaPlayer::ReadyState reported_ready_state_;

  // first -> pts, second -> duration
  base::StaticMap<DemuxerStream::Type,
                  std::pair<base::TimeDelta, base::TimeDelta>,
                  base::StaticKeys<DemuxerStream::Type,
                                   DemuxerStream::AUDIO,
                                   DemuxerStream::VIDEO>>
      last_frames_;

  std::array<ChannelData, kCapiPlayerChannelCount> channel_data_;
  CompleteCB complete_callback_;

  base::RepeatingTimer current_time_update_timer_;
  base::WeakPtrFactory<MediaSourcePlayerCapi> weak_factory_;

  base::TimeDelta video_key_frame_;
  DISALLOW_COPY_AND_ASSIGN(MediaSourcePlayerCapi);
};

class BufferObserver {
 public:
  using BufferingCallback =
      base::Callback<void(DemuxerStream::Type,
                          MediaSourcePlayerCapi::BufferStatus)>;

  virtual ~BufferObserver() = default;

  virtual void SetBufferSize(DemuxerStream::Type, player_buffer_size_t) = 0;
  virtual void SetBufferingCallback(const BufferingCallback&) = 0;
  virtual void EOSReached(DemuxerStream::Type) = 0;
  virtual void UpdateBufferFill(DemuxerStream::Type, player_buffer_size_t) = 0;
  virtual void ResetBufferStatus() = 0;
  virtual int SetMediaStreamStatusAudioCallback(player_h) = 0;
  virtual int SetMediaStreamStatusVideoCallback(player_h) = 0;
  virtual MediaSourcePlayerCapi::BufferStatus AudioStatus() const = 0;
  virtual MediaSourcePlayerCapi::BufferStatus VideoStatus() const = 0;
};

}  // namespace media

#endif  // MEDIA_BASE_TIZEN_MEDIA_SOURCE_PLAYER_CAPI_H_
