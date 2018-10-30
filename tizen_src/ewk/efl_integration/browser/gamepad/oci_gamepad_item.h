// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TIZEN_SRC_EWK_EFL_INTEGRATION_BROWSER_GAMEPAD_OCI_GAMEPAD_ITEM_H_
#define TIZEN_SRC_EWK_EFL_INTEGRATION_BROWSER_GAMEPAD_OCI_GAMEPAD_ITEM_H_

#include <accessory/GamepadEntry.h>
#include <accessory/IGamepad.h>
#include <accessory/OCICommon.h>
#include <memory>
#include "base/macros.h"
#include "device/gamepad/public/cpp/gamepad.h"

namespace device {

class OCIGamepadItem : public gamepad::CGamepadCallback {
 public:
  ~OCIGamepadItem() override;

  // OCIGamepadItem implementation.
  const char* GetUID();
  const char* GetGamepadName();
  size_t GetItemIndex();

  // change the device data.
  void RefreshOCIGamepadData();

  // static function for create OCIGamepadItem, used by
  // GamepadPlatformDataFetcherTizenTV
  static std::unique_ptr<OCIGamepadItem> Create(IGamepadManager* manager,
                                                OCIDevInfo* dev_info,
                                                Gamepad* pad,
                                                int index);

 protected:
  // Cannot follow google naming style, because it's override from
  // CGamepadCallback
  void t_OnCallback(int type,
                    void* pparam1,
                    void* pparam2,
                    void* pparam3) override;

 private:
  float range_tizen_axis_value_abs_z_;
  float range_tizen_axis_value_abs_rz_;

  float range_tizen_axis_value_abs_x_;
  float range_tizen_axis_value_abs_y_;
  float range_tizen_axis_value_abs_rx_;
  float range_tizen_axis_value_abs_ry_;

  static const size_t kOCIGamepadItemsLength = 16;
  OCIControllerEvent events_[kOCIGamepadItemsLength];

  explicit OCIGamepadItem(IGamepadManager* manager,
                          OCIDevInfo* info,
                          Gamepad* pad);
  bool CreateDevice();
  void DestroyDevice();
  void SetOCIGamepadItemIndex(size_t index);

  void InitAxisRangeTizen();
  void InitAxisRangeTizen(int code, float* range);
  void RefreshOCIGamepadDataEachEvent(const OCIControllerEvent& event,
                                      const int index);
  static size_t s_oci_gamepaditem_num_;
  static const float kRangeTizenAxisValueAbsZRz;
  static const float kRangeTizenAxisValueAbsXYRxRy;

  IGamepadManager* manager_;
  OCIDevInfo info_;
  IGamepad* gamepad_;
  Gamepad* pad_;
  size_t index_;

  DISALLOW_COPY_AND_ASSIGN(OCIGamepadItem);
};

}  // namespace device

#endif  // TIZEN_SRC_EWK_EFL_INTEGRATION_BROWSER_GAMEPAD_OCI_GAMEPAD_ITEM_H_
