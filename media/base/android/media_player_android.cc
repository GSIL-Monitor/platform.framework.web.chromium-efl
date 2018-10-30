// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/android/media_player_android.h"

#include <algorithm>

#include "base/android/scoped_java_ref.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "media/base/android/media_drm_bridge.h"
#include "media/base/android/media_player_manager.h"

using base::android::JavaRef;

namespace {

const double kDefaultVolume = 1.0;

}  // namespace

namespace media {

const double MediaPlayerAndroid::kDefaultVolumeMultiplier = 1.0;

MediaPlayerAndroid::MediaPlayerAndroid(
    int player_id,
    MediaPlayerManager* manager,
    const OnDecoderResourcesReleasedCB& on_decoder_resources_released_cb,
    const GURL& frame_url)
    : on_decoder_resources_released_cb_(on_decoder_resources_released_cb),
      player_id_(player_id),
      volume_(kDefaultVolume),
      volume_multiplier_(kDefaultVolumeMultiplier),
      manager_(manager),
      frame_url_(frame_url),
#if defined(S_TERRACE_SUPPORT)
      is_media_control_displayed_(false),
      weak_factory_(this) {
#else
      weak_factory_(this) {
#endif
  listener_.reset(new MediaPlayerListener(base::ThreadTaskRunnerHandle::Get(),
                                          weak_factory_.GetWeakPtr()));

#if defined(S_NATIVE_SUPPORT)
  played_ = false;
#endif
}

MediaPlayerAndroid::~MediaPlayerAndroid() {}

void MediaPlayerAndroid::SetVolume(double volume) {
  volume_ = std::max(0.0, std::min(volume, 1.0));
  UpdateEffectiveVolume();
}

void MediaPlayerAndroid::SetVolumeMultiplier(double volume_multiplier) {
  volume_multiplier_ = std::max(0.0, std::min(volume_multiplier, 1.0));
  UpdateEffectiveVolume();
}

double MediaPlayerAndroid::GetEffectiveVolume() const {
  return volume_ * volume_multiplier_;
}

void MediaPlayerAndroid::UpdateEffectiveVolume() {
  UpdateEffectiveVolumeInternal(GetEffectiveVolume());
}

// For most subclasses we can delete on the caller thread.
void MediaPlayerAndroid::DeleteOnCorrectThread() {
  delete this;
}

GURL MediaPlayerAndroid::GetUrl() {
  return GURL();
}

GURL MediaPlayerAndroid::GetSiteForCookies() {
  return GURL();
}

void MediaPlayerAndroid::SetRate(double rate) {}

#if defined(S_TERRACE_SUPPORT)
const std::string MediaPlayerAndroid::GetCookies() {
  return std::string();
}
#endif

void MediaPlayerAndroid::SetCdm(
    const scoped_refptr<ContentDecryptionModule>& /* cdm */) {
  // Players that support EME should override this.
  LOG(ERROR) << "EME not supported on base MediaPlayerAndroid class.";
  return;
}

void MediaPlayerAndroid::OnVideoSizeChanged(int width, int height) {
  manager_->OnVideoSizeChanged(player_id(), width, height);
}

void MediaPlayerAndroid::OnMediaError(int error_type) {
  manager_->OnError(player_id(), error_type);
}

void MediaPlayerAndroid::OnBufferingUpdate(int percent) {
  manager_->OnBufferingUpdate(player_id(), percent);
}

void MediaPlayerAndroid::OnPlaybackComplete() {
  manager_->OnPlaybackComplete(player_id());
}

void MediaPlayerAndroid::OnMediaInterrupted() {
  manager_->OnMediaInterrupted(player_id());
}

void MediaPlayerAndroid::OnSeekComplete() {
  manager_->OnSeekComplete(player_id(), GetCurrentTime());
}

void MediaPlayerAndroid::OnMediaPrepared() {}

void MediaPlayerAndroid::AttachListener(
    const JavaRef<jobject>& j_media_player) {
  listener_->CreateMediaPlayerListener(j_media_player);
}

void MediaPlayerAndroid::DetachListener() {
  listener_->ReleaseMediaPlayerListenerResources();
}

void MediaPlayerAndroid::DestroyListenerOnUIThread() {
  weak_factory_.InvalidateWeakPtrs();
  listener_.reset();
}

base::WeakPtr<MediaPlayerAndroid> MediaPlayerAndroid::WeakPtrForUIThread() {
  return weak_factory_.GetWeakPtr();
}

#if defined(S_TERRACE_SUPPORT)
void MediaPlayerAndroid::PlayMedia() {
  manager_->PlayMedia(player_id());
}

void MediaPlayerAndroid::PauseMedia() {
  manager_->PauseMedia(player_id());
}

void MediaPlayerAndroid::SetClosedCaptionUrl(const std::string& url) {
  closed_caption_url_ = url;
}

void MediaPlayerAndroid::SetClosedCaptionStatus(int status) {
  switch (status) {
    case 0:
      closed_caption_status_ = CLOSED_CAPTION_DISABLED;
      break;
    case 1:
      closed_caption_status_ = CLOSED_CAPTION_VISIBLE;
      break;
    case 2:
      closed_caption_status_ = CLOSED_CAPTION_INVISIBLE;
      break;
    default:
      NOTREACHED();
      break;
  }
}

int MediaPlayerAndroid::GetClosedCaptionStatus() {
  return closed_caption_status_;
}

bool MediaPlayerAndroid::HasClosedCaption() {
  return closed_caption_status_ != CLOSED_CAPTION_DISABLED;
}

void MediaPlayerAndroid::SetIsMediaControlDisplayed(bool is_displayed) {
  is_media_control_displayed_ = is_displayed;
}
#endif

}  // namespace media
