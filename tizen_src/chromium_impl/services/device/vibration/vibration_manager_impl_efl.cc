// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/vibration/vibration_manager_impl_efl.h"

#include "base/bind.h"

namespace device {

VibrationManagerImplEfl::VibrationManagerImplEfl() {}

VibrationManagerImplEfl::~VibrationManagerImplEfl() {}

std::unique_ptr<VibrationProviderClient>
    VibrationManagerImplEfl::provider_client_ =
        std::unique_ptr<VibrationProviderClient>();

void VibrationManagerImplEfl::Vibrate(int64_t milliseconds,
                                      VibrateCallback callback) {
  if (provider_client_.get())
    provider_client_->Vibrate(milliseconds);
  std::move(callback).Run();
}

void VibrationManagerImplEfl::Cancel(CancelCallback callback) {
  if (provider_client_.get())
    provider_client_->CancelVibration();
  std::move(callback).Run();
}

// static
void VibrationManagerImplEfl::RegisterProviderClient(
    VibrationProviderClient* provider_client) {
  provider_client_.reset(provider_client);
}

// static
void VibrationManagerImpl::Create(mojom::VibrationManagerRequest request) {
  mojo::MakeStrongBinding(base::MakeUnique<VibrationManagerImplEfl>(),
                          std::move(request));
}

}  // namespace device
