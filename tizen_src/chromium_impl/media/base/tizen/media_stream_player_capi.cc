// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "media/base/tizen/media_stream_player_capi.h"

#include "media/base/tizen/media_player_manager_efl.h"

namespace media {

// Buffering logic is not supported in MM player low latency mode.
// This class ignores all calls regarding buffering logic.
class DummyBufferObserver : public BufferObserver {
 public:
  void SetBufferSize(DemuxerStream::Type, player_buffer_size_t) override {}
  void SetBufferingCallback(const BufferingCallback&) override {}
  void EOSReached(DemuxerStream::Type) override {}
  void UpdateBufferFill(DemuxerStream::Type, player_buffer_size_t) override {}
  void ResetBufferStatus() override {}
  int SetMediaStreamStatusAudioCallback(player_h) override {
    return PLAYER_ERROR_NONE;
  }
  int SetMediaStreamStatusVideoCallback(player_h) override {
    return PLAYER_ERROR_NONE;
  }
  MediaSourcePlayerCapi::BufferStatus AudioStatus() const override {
    return MediaSourcePlayerCapi::BUFFER_NONE;
  }
  MediaSourcePlayerCapi::BufferStatus VideoStatus() const override {
    return MediaSourcePlayerCapi::BUFFER_NONE;
  }
};

MediaPlayerEfl* MediaPlayerEfl::CreateStreamPlayer(
    int player_id,
    std::unique_ptr<DemuxerEfl> demuxer,
    MediaPlayerManager* manager,
    bool video_hole) {
  LOG(INFO) << "MediaStreamElement is using |CAPI| to play media";
  return new MediaStreamPlayerCapi(player_id, std::move(demuxer), manager,
                                   video_hole);
}

MediaStreamPlayerCapi::MediaStreamPlayerCapi(
    int player_id,
    std::unique_ptr<DemuxerEfl> demuxer,
    MediaPlayerManager* manager,
    bool video_hole)
    : MediaSourcePlayerCapi(player_id,
                            std::move(demuxer),
                            manager,
                            new DummyBufferObserver(),
#if defined(OS_TIZEN_TV_PRODUCT)
                            PLAYER_DRM_TYPE_NONE,
#endif
                            video_hole) {
}

void MediaStreamPlayerCapi::Seek(base::TimeDelta) {
  LOG(INFO) << "Ignoring seek on media stream player";
}

void MediaStreamPlayerCapi::Suspend() {
  manager()->OnPlayerDestroyed(GetPlayerId());
  MediaSourcePlayerCapi::Suspend();
}

void MediaStreamPlayerCapi::HandlePrepareComplete() {
  MediaSourcePlayerCapi::HandlePrepareComplete();
  PlayInternal();
}

void MediaStreamPlayerCapi::OnMediaError(MediaError) {
  Suspend();
}

void MediaStreamPlayerCapi::StartWaitingForData() {
  // Player should not be stalled if there isn't enough data
}

bool MediaStreamPlayerCapi::ShouldFeed(DemuxerStream::Type) const {
  // Player should be fed only after being prepared, otherwise MM
  // player in low latency mode has problems with a proper initialization.
  return player_prepared_;
}

bool MediaStreamPlayerCapi::SetAudioStreamInfo() {
  if (!MediaSourcePlayerCapi::SetAudioStreamInfo())
    return false;
#if defined(OS_TIZEN_TV_PRODUCT)
  auto ret = player_set_audio_latency_mode(player_, AUDIO_LATENCY_MODE_LOW);
  if (ret != PLAYER_ERROR_NONE) {
    LOG(ERROR) << "player_set_audio_latency_mode Error: " << ret;
    return false;
  }
#endif
  return true;
}

bool MediaStreamPlayerCapi::SetVideoStreamInfo() {
  if (!MediaSourcePlayerCapi::SetVideoStreamInfo())
    return false;
#if defined(OS_TIZEN_TV_PRODUCT)
  auto ret = player_set_video_latency_mode(player_, VIDEO_LATENCY_MODE_LOW);
  if (ret != PLAYER_ERROR_NONE) {
    LOG(ERROR) << "player_set_video_latency_mode Error: " << ret;
    return false;
  }
#endif
  return true;
}

bool MediaStreamPlayerCapi::SetPlayerIniParam() {
#if defined(OS_TIZEN_TV_PRODUCT)
  auto ret = player_set_ini_param(player_, "free run = yes");
  if (ret != PLAYER_ERROR_NONE) {
    LOG(ERROR) << "player_set_ini_param Error: " << ret;
    return false;
  }
#endif
  return true;
}

PlayerRoleFlags MediaStreamPlayerCapi::GetRoles() const noexcept {
  return PlayerRole::ElementaryStreamBased | PlayerRole::RemotePeerStream;
}

}  // namespace media
