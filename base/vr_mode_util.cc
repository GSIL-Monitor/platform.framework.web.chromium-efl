// Copy-right 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/vr_mode_util.h"

namespace base {

bool VrModeUtil::daydream_mode_enabled_ = false;
bool VrModeUtil::gear_vr_mode_enabled_ = false;
bool VrModeUtil::webgl_enabled_ = true;

bool VrModeUtil::isVrModeEnabled() {
  return (daydream_mode_enabled_ || gear_vr_mode_enabled_);
}

bool VrModeUtil::isDayDreamModeEnabled() {
  return daydream_mode_enabled_;
}

bool VrModeUtil::isGearVrModeEnabled() {
  return gear_vr_mode_enabled_;
}

void VrModeUtil::setDayDreamModeEnabled(bool enabled) {
  daydream_mode_enabled_ = enabled;
}

void VrModeUtil::setGearVrModeEnabled(bool enabled) {
  gear_vr_mode_enabled_ = enabled;
}

bool VrModeUtil::isWebGLEnabled() {
  return webgl_enabled_;
}

void VrModeUtil::setWebGLEnabled(bool enabled) {
  webgl_enabled_ = enabled;
}

}  // namespace base
