// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/display/screen_efl.h"

#include "ecore_x_wayland_wrapper.h"

#include "content/browser/renderer_host/dip_util.h"
#include "ui/base/x/x11_util.h"
#include "ui/display/device_display_info_efl.h"
#include "ui/display/device_display_info_observer_efl.h"
#include "ui/display/display.h"
#include "ui/display/display_observer.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/dip_util.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/x/x11_types.h"

using namespace display;

namespace ui {

class ScreenEfl : public display::Screen,
                  public display::DeviceDisplayInfoObserverEfl {
 public:
  ScreenEfl() {
    display::DeviceDisplayInfoEfl display_info;
    display_info.AddObserver(this);
    UpdateDisplays();
  }

  ~ScreenEfl() override {
    display::DeviceDisplayInfoEfl display_info;
    display_info.RemoveObserver(this);
  }

  // display::Screen overrides
  gfx::Point GetCursorScreenPoint() override {
    NOTIMPLEMENTED();
    return gfx::Point();
  }

  bool IsWindowUnderCursor(gfx::NativeWindow window) override {
    NOTIMPLEMENTED();
    return false;
  }

  gfx::NativeWindow GetWindowAtScreenPoint(const gfx::Point& point) override {
    return NULL;
  }

  int GetNumDisplays() const override { return 1; }

  const std::vector<display::Display>& GetAllDisplays() const override {
    return displays_;
  }

  display::Display GetDisplayNearestWindow(
      gfx::NativeView /*view*/) const override {
    return GetPrimaryDisplay();
  }

  display::Display GetDisplayNearestPoint(
      const gfx::Point& /*point*/) const override {
    return GetPrimaryDisplay();
  }

  display::Display GetDisplayMatching(
      const gfx::Rect& /*match_rect*/) const override {
    return GetPrimaryDisplay();
  }

  display::Display GetPrimaryDisplay() const override {
    DCHECK(!displays_.empty());
    return displays_[0];
  }

  void AddObserver(display::DisplayObserver* /*observer*/) override {
    NOTIMPLEMENTED();
  }

  void RemoveObserver(display::DisplayObserver* /*observer*/) override {
    NOTIMPLEMENTED();
  }

  // display::DeviceDisplayInfoObserverEfl
  void OnDeviceDisplayInfoChanged() override {
    UpdateDisplays();
  }

 private:
  void UpdateDisplays() {
    display::DeviceDisplayInfoEfl display_info;
    const float device_scale_factor =
        display::Display::HasForceDeviceScaleFactor()
            ? display::Display::GetForcedDeviceScaleFactor()
            : display_info.GetDIPScale();

    display::Display display(0);
    display.SetRotationAsDegree(display_info.GetRotationDegrees());

    int width = display_info.GetDisplayWidth();
    int height = display_info.GetDisplayHeight();

    const gfx::Rect bounds_in_pixels = gfx::Rect(width, height);
    const gfx::Rect bounds_in_dip =
        gfx::ConvertRectToDIP(device_scale_factor, bounds_in_pixels);

    display.set_device_scale_factor(device_scale_factor);
    display.set_bounds(bounds_in_dip);
    display.set_work_area(bounds_in_dip);

    std::vector<display::Display> displays (1, display);
    displays_.swap(displays);
  }

  std::vector<display::Display> displays_;

  DISALLOW_COPY_AND_ASSIGN(ScreenEfl);
};

void InstallScreenInstance() {
  static bool installed = false;
  if (!installed) {
    installed = true;
    display::Screen::SetScreenInstance(new ui::ScreenEfl());
  }
}

}  // namespace ui

namespace display {
// static
gfx::NativeWindow Screen::GetWindowForView(gfx::NativeView view) {
  return view;
}
}  // namespace display
