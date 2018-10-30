// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if defined(OS_TIZEN)
#include "device/sensors/data_fetcher_impl_tizen.h"
#endif

#include "base/logging.h"
#include "device/sensors/data_fetcher_shared_memory.h"
#include "device/sensors/public/cpp/device_motion_hardware_buffer.h"
#include "device/sensors/public/cpp/device_orientation_hardware_buffer.h"
#include "tizen/system_info.h"

namespace device {

DataFetcherSharedMemory::DataFetcherSharedMemory() {
}

DataFetcherSharedMemory::~DataFetcherSharedMemory() {
}

bool DataFetcherSharedMemory::Start(ConsumerType consumer_type, void* buffer) {
  if (!(IsMobileProfile() || IsWearableProfile()))
    return false;

  DCHECK(buffer);

  switch (consumer_type) {
    case CONSUMER_TYPE_MOTION:
      return DataFetcherImplTizen::GetInstance()->
          StartFetchingDeviceMotionData(
              static_cast<DeviceMotionHardwareBuffer*>(buffer));
    case CONSUMER_TYPE_ORIENTATION:
      return DataFetcherImplTizen::GetInstance()->
          StartFetchingDeviceOrientationData(
              static_cast<DeviceOrientationHardwareBuffer*>(buffer));
    default:
      NOTREACHED();
  }
  return false;
}

bool DataFetcherSharedMemory::Stop(ConsumerType consumer_type) {
  if (!(IsMobileProfile() || IsWearableProfile()))
    return false;

  switch (consumer_type) {
    case CONSUMER_TYPE_MOTION:
      DataFetcherImplTizen::GetInstance()->StopFetchingDeviceMotionData();
      return true;
    case CONSUMER_TYPE_ORIENTATION:
      DataFetcherImplTizen::GetInstance()->StopFetchingDeviceOrientationData();
      return true;
    default:
      NOTREACHED();
  }
  return false;
}

#if !defined(EWK_BRINGUP)
DataFetcherSharedMemory::FetcherType DataFetcherSharedMemory::GetType() const {
  return FETCHER_TYPE_SEPARATE_THREAD;
}
#endif

}  // namespace content
