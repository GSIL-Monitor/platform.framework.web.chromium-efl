// Copyright 2013 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/vibration/vibration_provider_client.h"

#include "services/device/vibration/vibration_manager_impl_efl.h"

VibrationProviderClientEwk* VibrationProviderClientEwk::GetInstance() {
  static VibrationProviderClientEwk* vibration_provider_client = NULL;

  if (!vibration_provider_client) {
    vibration_provider_client = new VibrationProviderClientEwk;
    device::VibrationManagerImplEfl::RegisterProviderClient(vibration_provider_client);
  }
  return vibration_provider_client;
}

void VibrationProviderClientEwk::Vibrate(uint64_t vibration_time) {
  if (vibrate_cb_)
    vibrate_cb_(vibration_time, user_data_);
}

void VibrationProviderClientEwk::CancelVibration() {
  if (cancel_vibration_cb_)
    cancel_vibration_cb_(user_data_);
}

void VibrationProviderClientEwk::SetVibrationClientCallbacks(Ewk_Vibration_Client_Vibrate_Cb vibrate,
                                                          Ewk_Vibration_Client_Vibration_Cancel_Cb cancelVibration,
                                                          void* data) {
  vibrate_cb_ = vibrate;
  cancel_vibration_cb_ = cancelVibration;
  user_data_ = data;
}
