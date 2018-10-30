// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_EFL_INTEGRATION_RENDERER_TIZEN_KEY_SYSTEM_PROPERTIES_H_
#define EWK_EFL_INTEGRATION_RENDERER_TIZEN_KEY_SYSTEM_PROPERTIES_H_

#include "media/base/key_system_properties.h"

namespace media {

class TizenPlatformKeySystemProperties : public KeySystemProperties {
 public:
  explicit TizenPlatformKeySystemProperties(const std::string& key_system_name);

  std::string GetKeySystemName() const override;

  bool IsSupportedInitDataType(EmeInitDataType init_data_type) const override;

  SupportedCodecs GetSupportedCodecs() const override;

  EmeConfigRule GetRobustnessConfigRule(EmeMediaType,
                                        const std::string&) const override;

  EmeSessionTypeSupport GetPersistentLicenseSessionSupport() const override;

  EmeSessionTypeSupport GetPersistentReleaseMessageSessionSupport()
      const override;

  EmeFeatureSupport GetPersistentStateSupport() const override;

  EmeFeatureSupport GetDistinctiveIdentifierSupport() const override;

 private:
  const std::string key_system_name_;
};
}  // namespace media

#endif  // EWK_EFL_INTEGRATION_RENDERER_TIZEN_KEY_SYSTEM_PROPERTIES_H_
