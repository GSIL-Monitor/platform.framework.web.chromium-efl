// Copyright 2013 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef Vibration_Provider_Client_h
#define Vibration_Provider_Client_h

#include "base/macros.h"
#include "public/ewk_context_internal.h"
#include "services/device/vibration/vibration_provider_client.h"

class VibrationProviderClientEwk: public device::VibrationProviderClient {
 public:
  static VibrationProviderClientEwk* GetInstance();
  void Vibrate(uint64_t vibrationTime) override;
  void CancelVibration() override;
  void SetVibrationClientCallbacks(Ewk_Vibration_Client_Vibrate_Cb,
                                   Ewk_Vibration_Client_Vibration_Cancel_Cb,
                                   void*);

 private:
  VibrationProviderClientEwk()
    : vibrate_cb_(nullptr)
    , cancel_vibration_cb_(nullptr)
    , user_data_(nullptr) {}

  Ewk_Vibration_Client_Vibrate_Cb vibrate_cb_;
  Ewk_Vibration_Client_Vibration_Cancel_Cb cancel_vibration_cb_;
  void* user_data_;

  DISALLOW_COPY_AND_ASSIGN(VibrationProviderClientEwk);
};

#endif // Vibration_Provider_Client_h
