
// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_src/ewk/efl_integration/browser/gamepad/gamepad_platform_data_fetcher_tizen_tv.h"
#include <string.h>
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/trace_event/trace_event.h"

namespace device {

void GamepadPlatformDataFetcherTizenTV::GetPidVid(const char* uid,
                                                  std::string* pid,
                                                  std::string* vid) {
  // * @brief    just get pid and vid from uid information
  // * @param    [in]     - uid information  char[]
  // * @param    [out]    - pid
  // * @param    [out]    - vid
  //
  // eg. id
  // Microsoft Corporation Controller (STANDARD GAMEPAD Vendor: 045e Product:
  // 028e)
  // GetUID() :          USB\VID_045e\PID_028e\-/75
  //                     vid: 045e,  pid: 028e
  // GetGamepadName() :  Microsoft Corporation Controller

  int position_vid = 0;
  int position_pid = 0;
  int position_second = 0;
  int position_third = 0;

  std::string str_vp(uid);

  position_vid = str_vp.find("VID_");
  position_pid = str_vp.find("PID_");

  position_second = str_vp.find("\\", position_vid);
  position_third = str_vp.find("\\", position_pid);

  (*vid) = str_vp.substr(position_vid + 4, position_second - position_vid - 4);
  (*pid) = str_vp.substr(position_pid + 4, position_third - position_pid - 4);
}

void GamepadPlatformDataFetcherTizenTV::UTF8toUTF16(UChar utf_16_str[],
                                                    size_t utf_16_str_size,
                                                    std::string str,
                                                    size_t str_length) {
  base::TruncateUTF8ToByteSize(str, str_length - 1, &str);
  base::string16 tmp16 = base::UTF8ToUTF16(str);
  memset(utf_16_str, 0, utf_16_str_size);
  tmp16.copy(utf_16_str, utf_16_str_size - 1);
}

void GamepadPlatformDataFetcherTizenTV::MapperTizenStyleGamepad(
    const Gamepad& input,
    Gamepad* mapped) {
  *mapped = input;
  mapped->buttons[BUTTON_INDEX_LEFT_TRIGGER] = AxisToButton(input.axes[2]);
  mapped->buttons[BUTTON_INDEX_RIGHT_TRIGGER] = AxisToButton(input.axes[5]);
  mapped->buttons[BUTTON_INDEX_BACK_SELECT] = input.buttons[12];
  mapped->buttons[BUTTON_INDEX_START] = input.buttons[13];
  mapped->buttons[BUTTON_INDEX_LEFT_THUMBSTICK] = input.buttons[10];
  mapped->buttons[BUTTON_INDEX_RIGHT_THUMBSTICK] = input.buttons[11];

  mapped->buttons[BUTTON_INDEX_DPAD_UP] = AxisNegativeAsButton(input.axes[7]);
  mapped->buttons[BUTTON_INDEX_DPAD_DOWN] = AxisPositiveAsButton(input.axes[7]);
  mapped->buttons[BUTTON_INDEX_DPAD_LEFT] = AxisNegativeAsButton(input.axes[6]);
  mapped->buttons[BUTTON_INDEX_DPAD_RIGHT] =
      AxisPositiveAsButton(input.axes[6]);

  mapped->buttons[BUTTON_INDEX_META] = input.buttons[14];
  mapped->axes[AXIS_INDEX_RIGHT_STICK_X] = input.axes[3];
  mapped->axes[AXIS_INDEX_RIGHT_STICK_Y] = input.axes[4];
  mapped->buttons_length = BUTTON_INDEX_COUNT;
  mapped->axes_length = AXIS_INDEX_COUNT;
}

GamepadPlatformDataFetcherTizenTV::GamepadPlatformDataFetcherTizenTV() {
  gamepad_manager_ = nullptr;
  if (Gamepad_Create()) {
    Initialize();
  } else {
    LOG(ERROR) << "[Gamepad_LOG] "
                  "GamepadPlatformDataFetcherTizenTV::"
                  "GamepadPlatformDataFetcherTizenTV() Gamepad_Create() ERROR "
                  "!!!";
  }
}

GamepadPlatformDataFetcherTizenTV::~GamepadPlatformDataFetcherTizenTV() {
  CHECK(gamepad_manager_ != nullptr);
  gamepad_manager_->UnregisterCallback(this);

  // Need set gamepad_items_ to null before Gamepad_Destroy, caused by business
  // logic
  for (size_t i = 0; i < Gamepads::kItemsLengthCap; i++) {
    if (gamepad_items_[i] != nullptr) {
      gamepad_items_[i] = nullptr;
    }
  }
  Gamepad_Destroy();
}

void GamepadPlatformDataFetcherTizenTV::Initialize() {
  gamepad_manager_ = Gamepad_GetManager();
  if (gamepad_manager_ == nullptr) {
    LOG(ERROR) << "[Gamepad_LOG] "
                  "GamepadPlatformDataFetcherTizenTV::Initialize() "
                  "Gamepad_GetManager() handle get fail !!! ";
    return;
  }

  InitTizenDevice();
  EnumerateTizenDevices();
  gamepad_manager_->RegisterCallback(this);
}

void GamepadPlatformDataFetcherTizenTV::InitTizenDevice() {
  LOG(INFO) << "InitTizenDevice  Gamepads::kItemsLengthCap "
            << Gamepads::kItemsLengthCap;

  for (size_t i = 0; i < Gamepads::kItemsLengthCap; ++i) {
    Gamepad& pad = data_.items[i];
    pad.connected = false;
  }
}

void GamepadPlatformDataFetcherTizenTV::EnumerateTizenDevices() {
  CHECK(gamepad_manager_ != nullptr);

  for (size_t i = 0; i < Gamepads::kItemsLengthCap; i++) {
    OCIDevInfo dev_info;
    bool bIsFree;

    if (gamepad_manager_->GetConnectedDeviceInfo(dev_info, bIsFree, i) !=
        OCI_OK) {
      continue;
    }
    LOG(INFO) << " dev_info.deviceType " << dev_info.deviceType
              << " dev_info.eventTyp " << dev_info.eventType
              << " dev_info.profileType " << dev_info.profileType
              << " dev_info.UID " << dev_info.UID
              << " dev_info.name " << dev_info.name
              << " bIsFree " << bIsFree;
    if (bIsFree == false) {
      continue;
    }

    if (gamepad_items_[i] == nullptr) {
      gamepad_items_[i] = OCIGamepadItem::Create(gamepad_manager_, &dev_info,
                                                 &data_.items[i], i);
      if (gamepad_items_[i]) {
        InitMapping(i);
      }
    }
  }
}

void GamepadPlatformDataFetcherTizenTV::GetGamepadData(
    bool devices_changed_hint) {
  TRACE_EVENT0("GAMEPAD", "GetGamepadData");
  // Update our internal state.
  for (size_t i = 0; i < Gamepads::kItemsLengthCap; ++i) {
    if (gamepad_items_[i] != nullptr) {
      ReadTizenDeviceData(i);
    }
  }
  for (size_t i = 0; i < Gamepads::kItemsLengthCap; ++i) {
    PadState* state = GetPadState(i);
    if (!state)
      return;

    Gamepad& pad = state->data;
    MapperTizenStyleGamepad(data_.items[i], &pad);
  }
}

GamepadSource GamepadPlatformDataFetcherTizenTV::source() {
  return Factory::static_source();
}

void GamepadPlatformDataFetcherTizenTV::ReadTizenDeviceData(size_t index) {
  // Linker does not like CHECK_LT(index, Gamepads::kItemsLengthCap). =/
  if (index >= Gamepads::kItemsLengthCap) {
    LOG(ERROR) << "[Gamepad_LOG] "
                  "GamepadPlatformDataFetcherTizenTV::ReadTizenDeviceData() "
               << "index >= Gamepads::kItemsLengthCap";
    return;
  }
  CHECK(gamepad_items_[index] != nullptr);
  gamepad_items_[index]->RefreshOCIGamepadData();
}

void GamepadPlatformDataFetcherTizenTV::InitMapping(size_t index) {
  if (index >= Gamepads::kItemsLengthCap) {
    LOG(ERROR)
        << "[Gamepad_LOG] GamepadPlatformDataFetcherTizenTV::InitMapping() "
        << "index >= Gamepads::kItemsLengthCap";
    return;
  }

  Gamepad& pad = data_.items[index];

  // reset memory to zero
  memset(pad.buttons, 0, (sizeof(pad.buttons[0])) * Gamepad::kButtonsLengthCap);
  memset(pad.axes, 0, (sizeof(pad.axes[0])) * Gamepad::kAxesLengthCap);
  pad.axes[2] = -1.0;
  pad.axes[5] = -1.0;
  pad.buttons_length = 0;
  pad.axes_length = 0;
  pad.timestamp = 0;

  std::string mapping = "standard";
  UTF8toUTF16(pad.mapping, sizeof(pad.mapping), mapping,
              Gamepad::kMappingLengthCap);

  CHECK(gamepad_items_[index] != nullptr);

  std::string vid = "";
  std::string pid = "";

  GetPidVid(gamepad_items_[index]->GetUID(), &pid, &vid);

  std::string name_string(gamepad_items_[index]->GetGamepadName());

  std::string id =
      name_string + base::StringPrintf(" (%sVendor: %s Product: %s)",
                                       "STANDARD GAMEPAD ", vid.c_str(),
                                       pid.c_str());

  UTF8toUTF16(pad.id, sizeof(pad.id), id, Gamepad::kIdLengthCap);
}

void GamepadPlatformDataFetcherTizenTV::t_OnCallback(int type,
                                                     void* pparam1,
                                                     void* pparam2,
                                                     void* pparam3) {
  int evType = 0;
  OCIDevInfo dev_info;
  std::string sType = "";

  if (type == OCI_EVENT_DEV_CONNECT)
    sType = " CONNECT ";
  else if (type == OCI_EVENT_DEV_DISCONNECT)
    sType = " DISCONNECT ";

  LOG(INFO) << sType << " pparam1 " << reinterpret_cast<char*>(pparam1);

  if (t_GetDevManagerEventData(evType, dev_info,
                               reinterpret_cast<char*>(pparam1)) != OCI_OK) {
    LOG(ERROR) << "[Gamepad_LOG] "
                  "GamepadPlatformDataFetcherTizenTV::t_OnCallback() "
               << sType << "error != OCI_OK";

    return;
  }
  LOG(INFO) << " dev_info.deviceType " << dev_info.deviceType
            << " dev_info.eventTyp " << dev_info.eventType
            << " dev_info.profileType " << dev_info.profileType
            << " dev_info.UID " << dev_info.UID
            << " dev_info.name " << dev_info.name;

  if (dev_info.deviceType != OCI_DEVICE_JOYSTICK) {
    LOG(ERROR) << "[Gamepad_LOG] "
                  "GamepadPlatformDataFetcherTizenTV::t_OnCallback() "
               << sType << "error != OCI_DEVICE_JOYSTICK ";
    return;
  }

  switch (type) {
    case OCI_EVENT_DEV_CONNECT: {
      for (size_t i = 0; i < Gamepads::kItemsLengthCap; i++) {
        if (gamepad_items_[i] == nullptr) {
          LOG(INFO) << " connected";
          gamepad_items_[i] = OCIGamepadItem::Create(
              gamepad_manager_, &dev_info, &data_.items[i], i);
          if (gamepad_items_[i]) {
            InitMapping(i);
            break;
          }
        } else {
          continue;
        }
      }
      break;
    }

    case OCI_EVENT_DEV_DISCONNECT: {
      for (size_t i = 0; i < Gamepads::kItemsLengthCap; i++) {
        if (gamepad_items_[i] != nullptr &&
            (strcmp(gamepad_items_[i]->GetUID(), dev_info.UID) == 0)) {
          LOG(INFO) << " gamepad_items_[" << i << "]->GetUID() "
                    << gamepad_items_[i]->GetUID();
          LOG(INFO) << " disconnected";
          gamepad_items_[i] = nullptr;
          break;
        }
      }
      break;
    }
    case OCI_EVENT_DEV_STATUS: {
      break;
    }
    default: { break; }
  }
}

}  // namespace device
