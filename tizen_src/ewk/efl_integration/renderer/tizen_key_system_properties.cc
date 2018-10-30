// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/tizen_key_system_properties.h"

namespace media {

TizenPlatformKeySystemProperties::TizenPlatformKeySystemProperties(
    const std::string& key_system_name)
    : key_system_name_(key_system_name) {}

std::string TizenPlatformKeySystemProperties::GetKeySystemName() const {
  return key_system_name_;
}

bool TizenPlatformKeySystemProperties::IsSupportedInitDataType(
    EmeInitDataType init_data_type) const {
  switch (init_data_type) {
    case EmeInitDataType::WEBM:
    case EmeInitDataType::CENC:
    case EmeInitDataType::KEYIDS:
      return true;
    default:
      return false;
  }
}

SupportedCodecs TizenPlatformKeySystemProperties::GetSupportedCodecs() const {
  return EmeCodec::EME_CODEC_WEBM_OPUS | EmeCodec::EME_CODEC_WEBM_VORBIS |
         EmeCodec::EME_CODEC_WEBM_VP9 |
#if BUILDFLAG(USE_PROPRIETARY_CODECS)
         EmeCodec::EME_CODEC_MP4_AAC | EmeCodec::EME_CODEC_MP4_AVC1 |
#endif
#if BUILDFLAG(ENABLE_AC3_EAC3_AUDIO_DEMUXING)
         EmeCodec::EME_CODEC_MP4_EAC3 |
#endif
#if BUILDFLAG(ENABLE_HEVC_DEMUXING)
         EmeCodec::EME_CODEC_MP4_HEVC |
#endif
         0;  // just to have right-hand-side argument for
  // either AVC1, or guarded flags...
}

EmeConfigRule TizenPlatformKeySystemProperties::GetRobustnessConfigRule(
    EmeMediaType,
    const std::string&) const {
  // TODO: check this one
  return EmeConfigRule::SUPPORTED;
}

EmeSessionTypeSupport
TizenPlatformKeySystemProperties::GetPersistentLicenseSessionSupport() const {
  return EmeSessionTypeSupport::SUPPORTED;
}

EmeSessionTypeSupport
TizenPlatformKeySystemProperties::GetPersistentReleaseMessageSessionSupport()
    const {
  return EmeSessionTypeSupport::SUPPORTED;
}

EmeFeatureSupport TizenPlatformKeySystemProperties::GetPersistentStateSupport()
    const {
  return EmeFeatureSupport::REQUESTABLE;
}

EmeFeatureSupport
TizenPlatformKeySystemProperties::GetDistinctiveIdentifierSupport() const {
  return EmeFeatureSupport::NOT_SUPPORTED;
}

}  // namespace media
