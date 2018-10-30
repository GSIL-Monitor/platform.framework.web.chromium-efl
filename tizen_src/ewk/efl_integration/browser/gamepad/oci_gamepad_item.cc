// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_src/ewk/efl_integration/browser/gamepad/oci_gamepad_item.h"
#include <utility>
#include "base/logging.h"

namespace device {

size_t OCIGamepadItem::s_oci_gamepaditem_num_ = 0;

const float OCIGamepadItem::kRangeTizenAxisValueAbsZRz = 127.5;
const float OCIGamepadItem::kRangeTizenAxisValueAbsXYRxRy = 32767.0;

// static function
std::unique_ptr<OCIGamepadItem> OCIGamepadItem::Create(IGamepadManager* manager,
                                                       OCIDevInfo* dev_info,
                                                       Gamepad* pad,
                                                       int index) {
  if (manager == nullptr) {
    return nullptr;
  }
  auto gamepad_item = std::unique_ptr<OCIGamepadItem>(
      new OCIGamepadItem(manager, dev_info, pad));
  if (!gamepad_item->CreateDevice()) {
    return nullptr;
  }
  gamepad_item->SetOCIGamepadItemIndex(index);
  return gamepad_item;
}

void OCIGamepadItem::InitAxisRangeTizen(int code, float* range) {
  if (gamepad_ == nullptr) {
    LOG(ERROR) << "[Gamepad_LOG] OCIGamepadItem::InitAxisRangeTizen() "
                  "gamepad_ == nullptr return";
    return;
  }

  // Do not need to modify, because GetABSValueRange(int, long&, long&) used by
  // accessory.
  long min = 0l;  // NOLINT(runtime/int)
  long max = 0l;  // NOLINT(runtime/int)

  if (gamepad_->GetABSValueRange(code, max, min) == OCI_OK) {
    (*range) = static_cast<float>(max - min) / 2.0;
  } else {
    LOG(ERROR) << "[Gamepad_LOG] OCIGamepadItem::InitAxisRangeTizen(int "
                  "code, float* range) init error, code = "
               << code;
  }
}

void OCIGamepadItem::InitAxisRangeTizen() {
  // OCI_ABS_X, OCI_ABS_Y, OCI_ABS_RX, OCI_ABS_RY   maybe 32767.0
  // OCI_ABS_Z, OCI_ABS_RZ   maybe 127.5
  InitAxisRangeTizen(OCI_ABS_X, &range_tizen_axis_value_abs_x_);
  InitAxisRangeTizen(OCI_ABS_Y, &range_tizen_axis_value_abs_y_);
  InitAxisRangeTizen(OCI_ABS_RX, &range_tizen_axis_value_abs_rx_);
  InitAxisRangeTizen(OCI_ABS_RY, &range_tizen_axis_value_abs_ry_);
  InitAxisRangeTizen(OCI_ABS_Z, &range_tizen_axis_value_abs_z_);
  InitAxisRangeTizen(OCI_ABS_RZ, &range_tizen_axis_value_abs_rz_);
}

OCIGamepadItem::OCIGamepadItem(IGamepadManager* manager,
                               OCIDevInfo* info,
                               Gamepad* pad) {
  CHECK(info != nullptr);
  memcpy(&info_, info, sizeof(OCIDevInfo));
  CHECK(manager != nullptr);
  manager_ = manager;
  gamepad_ = nullptr;
  pad_ = pad;

  range_tizen_axis_value_abs_x_ = kRangeTizenAxisValueAbsXYRxRy;
  range_tizen_axis_value_abs_y_ = kRangeTizenAxisValueAbsXYRxRy;
  range_tizen_axis_value_abs_rx_ = kRangeTizenAxisValueAbsXYRxRy;
  range_tizen_axis_value_abs_ry_ = kRangeTizenAxisValueAbsXYRxRy;

  range_tizen_axis_value_abs_z_ = kRangeTizenAxisValueAbsZRz;
  range_tizen_axis_value_abs_rz_ = kRangeTizenAxisValueAbsZRz;

  s_oci_gamepaditem_num_++;
}

OCIGamepadItem::~OCIGamepadItem() {
  this->DestroyDevice();
  s_oci_gamepaditem_num_--;
}

bool OCIGamepadItem::CreateDevice() {
  CHECK(manager_ != nullptr);
  gamepad_ = manager_->CreateDevice(info_.UID);
  if (gamepad_ == nullptr) {
    LOG(ERROR)
        << "[Gamepad_LOG] OCIGamepadItem::Create() gamepad_ == nullptr return";
    return false;
  }

  CHECK(pad_ != nullptr);
  pad_->connected = true;

  InitAxisRangeTizen();
  return true;
}

void OCIGamepadItem::DestroyDevice() {
  CHECK(manager_ != nullptr);
  CHECK(gamepad_ != nullptr);
  CHECK(pad_ != nullptr);

  manager_->DestroyDevice(gamepad_->GetID());
  gamepad_ = nullptr;
  pad_->connected = false;

  return;
}

void OCIGamepadItem::t_OnCallback(int type,
                                  void* pparam1,
                                  void* pparam2,
                                  void* pparam3) {
  // used by parent class
}

const char* OCIGamepadItem::GetUID() {
  LOG(INFO) << "GamePad " << info_.UID;
  return info_.UID;
}

const char* OCIGamepadItem::GetGamepadName() {
  LOG(INFO) << "GamePad " << info_.name;
  return info_.name;
}

void OCIGamepadItem::SetOCIGamepadItemIndex(size_t index) {
  index_ = index;
}

size_t OCIGamepadItem::GetItemIndex() {
  return index_;
}

void OCIGamepadItem::RefreshOCIGamepadDataEachEvent(
    const OCIControllerEvent& event,
    const int index) {
  Gamepad& pad = (*pad_);
  size_t item = event.code;
  if (event.type == OCI_EV_ABS) {
    if (item >= Gamepad::kAxesLengthCap) {
      // change the keycode index
      // OCI_ABS_HAT0X & OCI_ABS_HAT0Y  maybe [-1, 0, 1]

      if (event.code == OCI_ABS_HAT0X) {
        item = 6;
        pad.axes[item] = event.value;
        if (item >= pad.axes_length) {
          pad.axes_length = item + 1;
        }
      } else if (event.code == OCI_ABS_HAT0Y) {
        item = 7;
        pad.axes[item] = event.value;
        if (item >= pad.axes_length) {
          pad.axes_length = item + 1;
        }
      } else {
        LOG(ERROR)
            << "[Gamepad_LOG] OCIGamepadItem::RefreshOCIGamepadDataEachEvent() "
            << " set the keycode index Gamepad::axes Error 001 !!!";
        return;
      }
    } else {
      // OCI_ABS_Z, OCI_ABS_RZ  [0, 255] -> [-1, 1]  init -1
      // OCI_ABS_X&OCI_ABS_Y, OCI_ABS_RX&OCI_ABS_RY    [-32768.32767] -> [-1, 1]

      if (event.code == OCI_ABS_Z) {
        pad.axes[item] = (event.value - range_tizen_axis_value_abs_z_) /
                         range_tizen_axis_value_abs_z_;
      } else if (event.code == OCI_ABS_RZ) {
        pad.axes[item] = (event.value - range_tizen_axis_value_abs_rz_) /
                         range_tizen_axis_value_abs_rz_;
      } else if (event.code == OCI_ABS_X) {
        pad.axes[item] = event.value / range_tizen_axis_value_abs_x_;
      } else if (event.code == OCI_ABS_Y) {
        pad.axes[item] = event.value / range_tizen_axis_value_abs_y_;
      } else if (event.code == OCI_ABS_RX) {
        pad.axes[item] = event.value / range_tizen_axis_value_abs_rx_;
      } else if (event.code == OCI_ABS_RY) {
        pad.axes[item] = event.value / range_tizen_axis_value_abs_ry_;
      } else {
        LOG(ERROR)
            << "[Gamepad_LOG] OCIGamepadItem::RefreshOCIGamepadDataEachEvent() "
            << " set the keycode index Gamepad::axes Error 002 !!!";
      }
      if (item >= pad.axes_length) {
        pad.axes_length = item + 1;
      }
    }
  } else if (event.type == OCI_EV_KEY) {
    if (item >= Gamepad::kButtonsLengthCap) {
      LOG(ERROR)
          << "[Gamepad_LOG] OCIGamepadItem::RefreshOCIGamepadDataEachEvent() "
          << " set the keycode index Gamepad::buttons Error !!!";
      return;
    }
    pad.buttons[item].pressed = event.value;
    pad.buttons[item].value = event.value ? 1.0 : 0.0;
    if (item >= pad.buttons_length) {
      pad.buttons_length = item + 1;
    }
  }
  pad.timestamp = event.time;

  return;
}

void OCIGamepadItem::RefreshOCIGamepadData() {
  if (gamepad_ == nullptr) {
    return;
  }

  CHECK(OCIGamepadItem::kOCIGamepadItemsLength >= s_oci_gamepaditem_num_);

  int count = OCIGamepadItem::kOCIGamepadItemsLength;
  memset(events_, 0, sizeof(OCIControllerEvent) * count);
  if (OCI_OK == gamepad_->GetInputEventEx(events_, count)) {
    for (int i = 0; i < count; i++) {
      RefreshOCIGamepadDataEachEvent(events_[i], count);
    }
  }
}

}  // namespace device
