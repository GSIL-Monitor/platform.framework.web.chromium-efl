// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_TIZEN_MEDIA_STREAM_PLAYER_CAPI_H_
#define MEDIA_BASE_TIZEN_MEDIA_STREAM_PLAYER_CAPI_H_

#include "media/base/tizen/media_source_player_capi.h"

namespace media {
class MEDIA_EXPORT MediaStreamPlayerCapi : public MediaSourcePlayerCapi {
 public:
  using MediaSourcePlayerCapi::MediaSourcePlayerCapi;

  MediaStreamPlayerCapi(int player_id,
                        std::unique_ptr<DemuxerEfl> demuxer,
                        MediaPlayerManager* manager,
                        bool video_hole);

  void Seek(base::TimeDelta time) override;
  void Suspend() override;

  PlayerRoleFlags GetRoles() const noexcept override;

 protected:
  void OnMediaError(MediaError) override;
  void HandlePrepareComplete() override;
  void StartWaitingForData() override;

 private:
  bool ShouldFeed(DemuxerStream::Type type) const override;
  bool SetAudioStreamInfo() override;
  bool SetVideoStreamInfo() override;
  bool SetPlayerIniParam() override;
};
}  // namespace media

#endif  // MEDIA_BASE_TIZEN_MEDIA_STREAM_PLAYER_CAPI_H_
