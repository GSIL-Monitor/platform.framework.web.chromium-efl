// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/battery/battery_status_manager_tizen.h"

#include <device/battery.h>
#include <device/callback.h>
#include <vconf/vconf.h>

namespace device {

namespace {

void OnChargingStatusChanged(keynode_t* key, void* data) {
  BatteryStatusManagerTizen* bsm =
      static_cast<BatteryStatusManagerTizen*>(data);
  if (!bsm)
    return;

  int charging = 0;
  if (vconf_get_int(VCONFKEY_SYSMAN_BATTERY_CHARGE_NOW, &charging) < 0)
    return;

  mojom::BatteryStatus current_status = bsm->GetCurrentStatus();
  current_status.charging = charging > 0;
  current_status.charging_time = 0.0;
  current_status.discharging_time = std::numeric_limits<double>::infinity();

  int left_time = 0;
  if (current_status.charging) {
    bsm->GetRemainingTimeUntilFullyCharged(&left_time);
    if (left_time > 0)
      current_status.charging_time = left_time;
  } else {
    bsm->GetRemainingTimeUntilDischarged(&left_time);
    if (left_time > 0)
      current_status.discharging_time = left_time;
  }

  bsm->SetCurrentStatus(current_status);
  bsm->GetBatteryUpdateCallback().Run(current_status);
}

void OnLevelChanged(device_callback_e device_type, void* value, void* data) {
  intptr_t percent = (intptr_t)value;

  BatteryStatusManagerTizen* bsm =
      static_cast<BatteryStatusManagerTizen*>(data);
  if (!bsm)
    return;

  mojom::BatteryStatus current_status = bsm->GetCurrentStatus();
  current_status.level = percent / 100.0;
  bsm->SetCurrentStatus(current_status);
  bsm->GetBatteryUpdateCallback().Run(current_status);
}

}  // namespace

std::unique_ptr<BatteryStatusManager> BatteryStatusManager::Create(
    const BatteryStatusService::BatteryUpdateCallback& callback) {
  return base::MakeUnique<BatteryStatusManagerTizen>(callback);
}

BatteryStatusManagerTizen::BatteryStatusManagerTizen(
    const BatteryStatusService::BatteryUpdateCallback& callback)
    : callback_(callback) {
}

BatteryStatusManagerTizen::~BatteryStatusManagerTizen() {
  StopListeningBatteryChange();
}

int BatteryStatusManagerTizen::GetRemainingTimeUntilFullyCharged(
    int* time) const {
  DCHECK(time != NULL);
  bool charging = false;
  if (device_battery_is_charging(&charging) != DEVICE_ERROR_NONE ||
      charging == false)
    return DEVICE_ERROR_OPERATION_FAILED;

  int temp_time = 0;
  if (vconf_get_int(VCONFKEY_PM_BATTERY_TIMETOFULL, &temp_time) < 0 ||
      temp_time < 0)
    return DEVICE_ERROR_OPERATION_FAILED;

  *time = temp_time;
  return DEVICE_ERROR_NONE;
}

int BatteryStatusManagerTizen::GetRemainingTimeUntilDischarged(
    int* time) const {
  DCHECK(time != NULL);
  bool charging = false;
  if (device_battery_is_charging(&charging) != DEVICE_ERROR_NONE ||
      charging == true)
    return DEVICE_ERROR_OPERATION_FAILED;

  int temp_time = 0;
  if (vconf_get_int(VCONFKEY_PM_BATTERY_TIMETOEMPTY, &temp_time) < 0 ||
      temp_time < 0)
    return DEVICE_ERROR_OPERATION_FAILED;

  *time = temp_time;
  return DEVICE_ERROR_NONE;
}

bool BatteryStatusManagerTizen::StartListeningBatteryChange() {
  if (vconf_notify_key_changed(VCONFKEY_SYSMAN_BATTERY_CHARGE_NOW,
                               OnChargingStatusChanged, this) ||
      device_add_callback(DEVICE_CALLBACK_BATTERY_CAPACITY, OnLevelChanged,
                          this) != DEVICE_ERROR_NONE) {
    StopListeningBatteryChange();
    return false;
  }

  int charging = 0;
  if (vconf_get_int(VCONFKEY_SYSMAN_BATTERY_CHARGE_NOW, &charging) < 0)
    return false;

  int level = 0, charging_time = 0, discharging_time = 0;
  device_battery_get_percent(&level);
  GetRemainingTimeUntilFullyCharged(&charging_time);
  GetRemainingTimeUntilDischarged(&discharging_time);

  current_status_.charging = charging > 0;
  current_status_.level = level / 100.0;
  current_status_.charging_time = charging_time > 0 ? charging_time : 0.0;
  current_status_.discharging_time = discharging_time > 0 ? discharging_time :
      std::numeric_limits<double>::infinity();

  callback_.Run(current_status_);
  return true;
}

void BatteryStatusManagerTizen::StopListeningBatteryChange() {
  vconf_ignore_key_changed(VCONFKEY_SYSMAN_BATTERY_CHARGE_NOW,
      OnChargingStatusChanged);
  device_remove_callback(DEVICE_CALLBACK_BATTERY_CAPACITY, OnLevelChanged);
}

}  // namespace device
