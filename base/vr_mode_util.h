// Copy-right 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_VR_MODE_UTIL_H_
#define BASE_VR_MODE_UTIL_H_

#include "base/base_export.h"

namespace base {

class BASE_EXPORT VrModeUtil {
 public:
  static bool isVrModeEnabled();
  static bool isDayDreamModeEnabled();
  static bool isGearVrModeEnabled();
  static void setDayDreamModeEnabled(bool enabled);
  static void setGearVrModeEnabled(bool enabled);
  static bool isWebGLEnabled();
  static void setWebGLEnabled(bool enabled);

 private:
  static bool daydream_mode_enabled_;
  static bool gear_vr_mode_enabled_;
  static bool webgl_enabled_;
};

}  // namespace base

#endif  // BASE_VR_MODE_UTIL_H_
