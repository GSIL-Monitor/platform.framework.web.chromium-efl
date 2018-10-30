// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VIBRATION_MANAGER_IMPL_EFL_H
#define VIBRATION_MANAGER_IMPL_EFL_H

#include "services/device/vibration/vibration_manager_impl.h"

#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/device/vibration/vibration_provider_client.h"

namespace device {

class VibrationManagerImplEfl : public mojom::VibrationManager {
 public:
  VibrationManagerImplEfl();
  ~VibrationManagerImplEfl() override;

  void Vibrate(int64_t milliseconds, VibrateCallback callback) override;
  void Cancel(CancelCallback callback) override;

  static void RegisterProviderClient(VibrationProviderClient* provider_client);

 private:
  static std::unique_ptr<VibrationProviderClient> provider_client_;
};

}  // namespace device

#endif  // VIBRATION_MANAGER_IMPL_EFL_H
