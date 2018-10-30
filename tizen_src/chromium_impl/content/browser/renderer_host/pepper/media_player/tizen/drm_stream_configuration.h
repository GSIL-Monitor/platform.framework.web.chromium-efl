// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_DRM_STREAM_CONFIGURATION_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_DRM_STREAM_CONFIGURATION_H_

#include <cstdint>
#include <memory>
#include <vector>

#include "base/logging.h"
#include "base/macros.h"
#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_tizen_player_dispatcher.h"

namespace content {

class ByteStreamReader;

class DRMStreamConfiguration {
 public:
  static std::unique_ptr<DRMStreamConfiguration> Create() {
    return std::unique_ptr<DRMStreamConfiguration>{new DRMStreamConfiguration};
  }

  PP_MediaPlayerDRMType drm_type() const { return drm_type_; }

  const std::string& drm_specific_data() const { return drm_specific_data_; }

  int32_t ParseDRMInitData(const std::vector<uint8_t>& type,
                           const std::vector<uint8_t>& init_data);

 private:
  DRMStreamConfiguration() : drm_type_(PP_MEDIAPLAYERDRMTYPE_UNKNOWN) {}

  int32_t ParsePSSHBox(const std::vector<uint8_t>& pssh_box);

  // Microsoft PlayReady Header (PRO) parsing function
  int32_t ParsePROHeader(const std::vector<uint8_t>& pro_header);
  int32_t ReadPlayReadyRightsManagementHeader(ByteStreamReader&,
                                              uint16_t length);
  int32_t ReadPlayReadyEmbeddedLicense(ByteStreamReader&, uint16_t length);

  // Content protection system specific data.
  // It should be treated as vector<uint8_t>, however std::string is used
  // for compatibility with underlying IEME interface and to avoid unnecesary
  // data copying.
  std::string drm_specific_data_;

  PP_MediaPlayerDRMType drm_type_;

  DISALLOW_COPY_AND_ASSIGN(DRMStreamConfiguration);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_DRM_STREAM_CONFIGURATION_H_
