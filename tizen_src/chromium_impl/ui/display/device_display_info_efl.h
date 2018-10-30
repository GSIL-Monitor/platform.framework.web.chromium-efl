// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_DISPLAY_DEVICE_DISPLAY_INFO_EFL_H_
#define UI_DISPLAY_DEVICE_DISPLAY_INFO_EFL_H_

#include "base/memory/singleton.h"
#include "base/synchronization/lock.h"
#include "ui/display/display_export.h"

namespace display {

class DeviceDisplayInfoObserverEfl;

// Facilitates access to device information in browser or renderer.
class DISPLAY_EXPORT DeviceDisplayInfoEfl {
 public:
  explicit DeviceDisplayInfoEfl();

  // Update all info.
  void Update(int display_width, int display_height, double dip_scale,
              int rotation_degrees);

  // Sets rotation degrees. Expected values are one of { 0, 90, 180, 270 }.
  void SetRotationDegrees(int rotation_degrees);

  // Returns display width in physical pixels.
  int GetDisplayWidth() const;

  // Returns display height in physical pixels.
  int GetDisplayHeight() const;

  // Computes the DIP scale from the given dpi.
  double ComputeDIPScale(int dpi) const;

  // Returns a scaling factor for Density Independent Pixel unit
  // (1.0 is 160dpi, 0.75 is 120dpi, 2.0 is 320dpi).
  double GetDIPScale() const;

  // Returns the display rotation angle from its natural orientation. Expected
  // values are one of { 0, 90, 180, 270 }.
  int GetRotationDegrees() const;

  // Returns true, if the devive supports circle-shaped screen.
  bool IsScreenShapeCircle() const;

  void AddObserver(DeviceDisplayInfoObserverEfl* observer);
  void RemoveObserver(DeviceDisplayInfoObserverEfl* observer);

 private:
  DISALLOW_COPY_AND_ASSIGN(DeviceDisplayInfoEfl);
};

}  // namespace display

#endif  // UI_DISPLAY_DEVICE_DISPLAY_INFO_EFL_H_
