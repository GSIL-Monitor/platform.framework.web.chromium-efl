// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DATA_FETCHER_IMPL_TIZEN_H_
#define DATA_FETCHER_IMPL_TIZEN_H_

#include <sensor/sensor.h>

#include "base/synchronization/lock.h"
#include "device/sensors/device_sensors_consts.h"
#include "device/sensors/public/cpp/device_motion_hardware_buffer.h"
#include "device/sensors/public/cpp/device_orientation_hardware_buffer.h"

namespace base {
template<typename T> struct DefaultSingletonTraits;
}

namespace device {

// FIXME: This class should be moved to device namespace.
class DataFetcherImplTizen {
 public:
  static DataFetcherImplTizen* GetInstance();

  // Shared memory related methods.
  bool StartFetchingDeviceMotionData(DeviceMotionHardwareBuffer* buffer);
  void StopFetchingDeviceMotionData();

  bool StartFetchingDeviceOrientationData(
      DeviceOrientationHardwareBuffer* buffer);
  void StopFetchingDeviceOrientationData();

  virtual bool Start(ConsumerType);
  virtual void Stop(ConsumerType);

 protected:
  DataFetcherImplTizen();
  virtual ~DataFetcherImplTizen();
  static void onOrientationChanged(sensor_h sensor, sensor_event_s *event, void* userData);
  static void onAccelerationChanged(sensor_h sensor, sensor_event_s *event, void* userData);

 private:
  friend struct base::DefaultSingletonTraits<DataFetcherImplTizen>;

  void SetOrientationBufferReadyStatus(bool ready);

  DeviceMotionHardwareBuffer* device_motion_buffer_;
  DeviceOrientationHardwareBuffer* device_orientation_buffer_;
  DeviceMotionHardwareBuffer* last_motion_data_;
  bool has_last_motion_data_;
  unsigned long long last_motion_timestamp_;
  bool is_orientation_buffer_ready_;

  base::Lock motion_buffer_lock_;
  base::Lock orientation_buffer_lock_;

  DISALLOW_COPY_AND_ASSIGN(DataFetcherImplTizen);
};

} // namespace content

#endif  // DATA_FETCHER_IMPL_TIZEN_H_
