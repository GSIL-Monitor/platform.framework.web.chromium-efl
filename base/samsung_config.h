// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_SAMSUNG_CONFIG_H_
#define BASE_SAMSUNG_CONFIG_H_

#include "base/base_export.h"

namespace base {

class BASE_EXPORT SamsungConfig {
 public:
  // Returns true if SamsungDex is enabled.
  static bool IsDexEnabled();

 private:
  SamsungConfig() {}
  ~SamsungConfig() {}
};

}  // namespace base

#endif  // BASE_SAMSUNG_CONFIG_H_
