// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TIZEN_SRC_EWK_EFL_INTEGRATION_BROWSER_GAMEPAD_GAMEPAD_PLATFORM_DATA_FETCHER_TIZEN_TV_H_
#define TIZEN_SRC_EWK_EFL_INTEGRATION_BROWSER_GAMEPAD_GAMEPAD_PLATFORM_DATA_FETCHER_TIZEN_TV_H_

#include <accessory/GamepadCallback.h>
#include <accessory/GamepadEntry.h>
#include <accessory/IGamepad.h>
#include <accessory/OCICommon.h>
#include <memory>
#include <string>
#include "device/gamepad/gamepad_data_fetcher.h"
#include "device/gamepad/gamepad_standard_mappings.h"
#include "device/gamepad/public/cpp/gamepad.h"
#include "tizen_src/ewk/efl_integration/browser/gamepad/oci_gamepad_item.h"

namespace device {

class GamepadPlatformDataFetcherTizenTV
    : public device::GamepadDataFetcher,
      public gamepad::CGamepadManagerCallback {
 public:
  typedef device::GamepadDataFetcherFactoryImpl<
      GamepadPlatformDataFetcherTizenTV,
      device::GAMEPAD_SOURCE_TIZEN_TV>
      Factory;
  GamepadPlatformDataFetcherTizenTV();
  ~GamepadPlatformDataFetcherTizenTV() override;

  // GamepadDataFetcher implementation.
  void GetGamepadData(bool devices_changed_hint) override;
  GamepadSource source() override;

 protected:
  virtual void t_OnCallback(int type,
                            void* pparam1,
                            void* pparam2,
                            void* pparam3);

 private:
  void ReadTizenDeviceData(size_t index);
  void EnumerateTizenDevices();

  void InitTizenDevice();
  void Initialize();
  void InitMapping(size_t index);

  // Notice: [in] uid, [out] pid, [out] vid
  void GetPidVid(const char* uid, std::string* pid, std::string* vid);
  void UTF8toUTF16(UChar utf_16_str[],
                   size_t utf_16_str_size,
                   std::string str,
                   size_t str_length);

  // Functions to map from device data to standard layout.
  static void MapperTizenStyleGamepad(const Gamepad& input, Gamepad* mapped);

  // Data that's returned to the consumer.
  Gamepads data_;

  gamepad::IGamepadManager* gamepad_manager_;
  std::array<std::unique_ptr<OCIGamepadItem>, Gamepads::kItemsLengthCap>
      gamepad_items_;

  DISALLOW_COPY_AND_ASSIGN(GamepadPlatformDataFetcherTizenTV);
};

}  // namespace device

#endif  // TIZEN_SRC_EWK_EFL_INTEGRATION_BROWSER_GAMEPAD_GAMEPAD_PLATFORM_DATA_FETCHER_TIZEN_TV_H_
