// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/video/video_decode_accelerator.h"

#include <GLES2/gl2.h>
#include "base/logging.h"

namespace media {

VideoDecodeAccelerator::Config::Config() = default;
VideoDecodeAccelerator::Config::Config(const Config& config) = default;

VideoDecodeAccelerator::Config::Config(VideoCodecProfile video_codec_profile)
    : profile(video_codec_profile) {}

VideoDecodeAccelerator::Config::~Config() = default;

std::string VideoDecodeAccelerator::Config::AsHumanReadableString() const {
  std::ostringstream s;
  s << "profile: " << GetProfileName(profile) << " encrypted? "
    << (is_encrypted() ? "true" : "false");
  return s.str();
}

void VideoDecodeAccelerator::Client::NotifyInitializationComplete(
    bool success) {
  NOTREACHED() << "By default deferred initialization is not supported.";
}

VideoDecodeAccelerator::~VideoDecodeAccelerator() {}

bool VideoDecodeAccelerator::TryToSetupDecodeOnSeparateThread(
    const base::WeakPtr<Client>& decode_client,
    const scoped_refptr<base::SingleThreadTaskRunner>& decode_task_runner) {
  // Implementations in the process that VDA runs in must override this.
  LOG(FATAL) << "This may only be called in the same process as VDA impl.";
  return false;
}

void VideoDecodeAccelerator::ImportBufferForPicture(
    int32_t picture_buffer_id,
    const gfx::GpuMemoryBufferHandle& gpu_memory_buffer_handle) {
  NOTREACHED() << "Buffer import not supported.";
}

void VideoDecodeAccelerator::SetOverlayInfo(const OverlayInfo& overlay_info) {
  NOTREACHED() << "Overlays are not supported.";
}

GLenum VideoDecodeAccelerator::GetSurfaceInternalFormat() const {
  return GL_RGBA;
}

VideoDecodeAccelerator::SupportedProfile::SupportedProfile()
    : profile(media::VIDEO_CODEC_PROFILE_UNKNOWN), encrypted_only(false) {}

VideoDecodeAccelerator::SupportedProfile::~SupportedProfile() {}

VideoDecodeAccelerator::Capabilities::Capabilities() : flags(NO_FLAGS) {}

VideoDecodeAccelerator::Capabilities::Capabilities(const Capabilities& other) =
    default;

VideoDecodeAccelerator::Capabilities::~Capabilities() {}

std::string VideoDecodeAccelerator::Capabilities::AsHumanReadableString()
    const {
  std::ostringstream s;
  s << "[";
  for (const SupportedProfile& sp : supported_profiles) {
    s << " " << GetProfileName(sp.profile) << ": " << sp.min_resolution.width()
      << "x" << sp.min_resolution.height() << "->" << sp.max_resolution.width()
      << "x" << sp.max_resolution.height();
  }
  s << "]";
  return s.str();
}

#if defined(TIZEN_RESOURCE_MANAGER)
void VideoDecodeAccelerator::SetResourceManager(
    content::TizenResourceManager* mgr) {
  resource_manager_ = mgr;
}
#endif

} // namespace media

namespace std {

void default_delete<media::VideoDecodeAccelerator>::operator()(
    media::VideoDecodeAccelerator* vda) const {
  vda->Destroy();
}

}  // namespace std
