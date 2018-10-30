// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VIBRATION_PROVIDER_CLIENT_H
#define VIBRATION_PROVIDER_CLIENT_H

#include <stdint.h>

namespace device {

class VibrationProviderClient {
 public:
  virtual void Vibrate(uint64_t vibrationTime) = 0;
  virtual void CancelVibration() = 0;

  virtual ~VibrationProviderClient() {}
};

}  // namespace device

#endif  // VIBRATION_PROVIDER_CLIENT_H
