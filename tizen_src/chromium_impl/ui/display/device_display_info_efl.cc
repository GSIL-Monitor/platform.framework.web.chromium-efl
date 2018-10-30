// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/display/device_display_info_efl.h"

#include "base/logging.h"
#include "base/observer_list.h"
#include "tizen/system_info.h"
#include "ui/display/device_display_info_observer_efl.h"

#if defined(OS_TIZEN)
#include <system_info.h>
#endif

namespace display {

namespace {

const double kBaselineDPIDensity = 160.0;
const int kInvalidRotationDegrees = -1;

#if defined(OS_TIZEN)
const char kScreenShapeCircle[] =
    "http://tizen.org/feature/screen.shape.circle";
#endif

int GetDensityRange(int dpi) {
  if (IsMobileProfile() || IsWearableProfile()) {
    // Copied from Android platform and extended to support UHD displays,
    // (http://developer.android.com/reference/android/util/DisplayMetrics.html).
    const int density_range[] = {
      0,    // NODPI, ANYDPI
      120,  // LOW
      160,  // MEDIUM
      240,  // HIGH
      280,  // DPI_280
      320,  // XHIGH
      400,  // DPI_400
      480,  // XXHIGH
      560,  // DPI_560
      640,  // XXXHIGH
      720,  // DPI_720
      800,  // ULTRAHIGH
      880,  // DPI_880
      960,  // ULTRAXHIGH
      1040, // DPI_1040
      1120, // ULTRAXXHIGH
      1200, // DPI_1200
      1280  // ULTRAXXXHIGH
    };

    const int range_size = sizeof(density_range) / sizeof(*density_range);
    int upper_bound = density_range[range_size - 1];
    for (int i = range_size - 2; i >= 0 && dpi <= density_range[i]; --i) {
      upper_bound = density_range[i];
    }

    return upper_bound;
  } else {
    // For platforms other than mobile, the user agents expect 1.0 as DIP scale
    // for the contents to be delivered.
    return 1.0;
  }
}

}  // namespace

// DeviceDisplayInfoEflImpl
class DISPLAY_EXPORT DeviceDisplayInfoEflImpl {
 public:
  static DeviceDisplayInfoEflImpl* GetInstance();

  void Update(int display_width, int display_height, double dip_scale,
              int rotation_degrees);
  void SetRotationDegrees(int rotation_degrees);
  int GetDisplayWidth() const;
  int GetDisplayHeight() const;
  double GetDIPScale() const;
  int GetRotationDegrees() const;
  bool IsScreenShapeCircle() const;

  void AddObserver(DeviceDisplayInfoObserverEfl* observer);
  void RemoveObserver(DeviceDisplayInfoObserverEfl* observer);

 private:
  DeviceDisplayInfoEflImpl();

  void NotifyDeviceDisplayInfoChanged();

  friend struct base::DefaultSingletonTraits<DeviceDisplayInfoEflImpl>;

  mutable base::Lock display_info_accessor_;
  mutable base::Lock rotation_accessor_;

  int display_width_;
  int display_height_;
  double dip_scale_;
  int rotation_degrees_;
  bool circle_shape_;

  base::ObserverList<DeviceDisplayInfoObserverEfl> observer_list_;

  DISALLOW_COPY_AND_ASSIGN(DeviceDisplayInfoEflImpl);
};

// static
DeviceDisplayInfoEflImpl* DeviceDisplayInfoEflImpl::GetInstance() {
  return base::Singleton<DeviceDisplayInfoEflImpl>::get();
}

void DeviceDisplayInfoEflImpl::Update(int display_width, int display_height,
                                      double dip_scale, int rotation_degrees) {
  {
    base::AutoLock autolock(display_info_accessor_);
    display_width_ = display_width;
    display_height_ = display_height;
    dip_scale_ = dip_scale;

    DCHECK_NE(0, display_width_);
    DCHECK_NE(0, display_height_);
    DCHECK_NE(0, dip_scale_);
  }

  {
    base::AutoLock autolock(rotation_accessor_);
    rotation_degrees_ = rotation_degrees;
    DCHECK_NE(kInvalidRotationDegrees, rotation_degrees_);
  }

  NotifyDeviceDisplayInfoChanged();
}

void DeviceDisplayInfoEflImpl::SetRotationDegrees(int rotation_degrees) {
  {
    base::AutoLock autolock(rotation_accessor_);
    rotation_degrees_ = rotation_degrees;
  }

  NotifyDeviceDisplayInfoChanged();
}

int DeviceDisplayInfoEflImpl::GetDisplayWidth()  const {
  base::AutoLock autolock(display_info_accessor_);
  DCHECK_NE(0, display_width_);
  return display_width_;
}

int DeviceDisplayInfoEflImpl::GetDisplayHeight() const {
  base::AutoLock autolock(display_info_accessor_);
  DCHECK_NE(0, display_height_);
  return display_height_;
}

double DeviceDisplayInfoEflImpl::GetDIPScale() const {
  base::AutoLock autolock(display_info_accessor_);
  DCHECK_NE(0, dip_scale_);
  return dip_scale_;
}

int DeviceDisplayInfoEflImpl::GetRotationDegrees() const {
  base::AutoLock autolock(rotation_accessor_);
  DCHECK_NE(kInvalidRotationDegrees, rotation_degrees_);
  return rotation_degrees_;
}

bool DeviceDisplayInfoEflImpl::IsScreenShapeCircle() const {
  return circle_shape_;
}

void DeviceDisplayInfoEflImpl::AddObserver(
    DeviceDisplayInfoObserverEfl* observer) {
  observer_list_.AddObserver(observer);
}

void DeviceDisplayInfoEflImpl::RemoveObserver(
    DeviceDisplayInfoObserverEfl* observer) {
  observer_list_.RemoveObserver(observer);
}

DeviceDisplayInfoEflImpl::DeviceDisplayInfoEflImpl()
  : display_width_(0),
    display_height_(0),
    dip_scale_(1.0),
    rotation_degrees_(kInvalidRotationDegrees),
    circle_shape_(false) {
#if defined(OS_TIZEN)
  system_info_get_platform_bool(kScreenShapeCircle, &circle_shape_);
#endif
}

void DeviceDisplayInfoEflImpl::NotifyDeviceDisplayInfoChanged() {
  for (auto& observer : observer_list_)
    observer.OnDeviceDisplayInfoChanged();
}

// DeviceDisplayInfoEfl
DeviceDisplayInfoEfl::DeviceDisplayInfoEfl() {
  DCHECK(DeviceDisplayInfoEflImpl::GetInstance());
}

void DeviceDisplayInfoEfl::Update(int display_width, int display_height,
                                  double dip_scale, int rotation_degrees) {
  DeviceDisplayInfoEflImpl::GetInstance()->Update(
      display_width, display_height, dip_scale, rotation_degrees);
}

void DeviceDisplayInfoEfl::SetRotationDegrees(int rotation_degrees) {
  DeviceDisplayInfoEflImpl::GetInstance()->SetRotationDegrees(rotation_degrees);
}

int DeviceDisplayInfoEfl::GetDisplayWidth() const {
  return DeviceDisplayInfoEflImpl::GetInstance()->GetDisplayWidth();
}

int DeviceDisplayInfoEfl::GetDisplayHeight() const {
  return DeviceDisplayInfoEflImpl::GetInstance()->GetDisplayHeight();
}

double DeviceDisplayInfoEfl::ComputeDIPScale(int dpi) const {
  if (IsMobileProfile()) {
    double dip_scale = static_cast<double>(GetDensityRange(dpi));
    DCHECK(dip_scale);
    dip_scale /= kBaselineDPIDensity;
    return dip_scale;
  } else {
    // For platforms other than mobile, the user agents expect 1.0 as DIP scale
    // for the contents to be delivered.
    return 1.0;
  }
}

double DeviceDisplayInfoEfl::GetDIPScale() const {
  return DeviceDisplayInfoEflImpl::GetInstance()->GetDIPScale();
}

int DeviceDisplayInfoEfl::GetRotationDegrees() const {
  return DeviceDisplayInfoEflImpl::GetInstance()->GetRotationDegrees();
}

bool DeviceDisplayInfoEfl::IsScreenShapeCircle() const {
  return DeviceDisplayInfoEflImpl::GetInstance()->IsScreenShapeCircle();
}

void DeviceDisplayInfoEfl::AddObserver(DeviceDisplayInfoObserverEfl* observer) {
  DeviceDisplayInfoEflImpl::GetInstance()->AddObserver(observer);
}

void DeviceDisplayInfoEfl::RemoveObserver(
    DeviceDisplayInfoObserverEfl* observer) {
  DeviceDisplayInfoEflImpl::GetInstance()->RemoveObserver(observer);
}

}  // namespace display

