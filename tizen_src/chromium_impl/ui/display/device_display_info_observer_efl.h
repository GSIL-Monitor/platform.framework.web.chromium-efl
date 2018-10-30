// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_DISPLAY_DEVICE_DISPLAY_INFO_OBSERVER_EFL_H_
#define UI_DISPLAY_DEVICE_DISPLAY_INFO_OBSERVER_EFL_H_

#include "ui/display/display_export.h"

namespace display {

class DISPLAY_EXPORT DeviceDisplayInfoObserverEfl {
 public:
  virtual void OnDeviceDisplayInfoChanged() = 0;

 protected:
  virtual ~DeviceDisplayInfoObserverEfl() {}
};

}  // namespace display

#endif  // UI_DISPLAY_DEVICE_DISPLAY_INFO_OBSERVER_EFL_H_
