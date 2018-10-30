// Copyright (c) 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/ozone_platform_efl.h"

#include "ui/display/types/native_display_delegate.h"
#include "ui/ozone/public/gpu_platform_support_host.h"
#include "ui/ozone/public/ozone_platform.h"
#include "ui/ozone/surface_factory_efl.h"
#include "ui/platform_window/platform_window.h"

namespace ui {

namespace {

// OzonePlatform implementation for EFL.
class OzonePlatformEfl : public OzonePlatform {
 public:
  OzonePlatformEfl() {}
  ~OzonePlatformEfl() override {}

  ui::SurfaceFactoryOzone* GetSurfaceFactoryOzone() override {
    return surface_factory_.get();
  }

  ui::OverlayManagerOzone* GetOverlayManager() override { return nullptr; }

  ui::CursorFactoryOzone* GetCursorFactoryOzone() override { return nullptr; }

  ui::InputController* GetInputController() override { return nullptr; }

  ui::GpuPlatformSupportHost* GetGpuPlatformSupportHost() override {
    return gpu_platform_host_.get();
  }

  std::unique_ptr<SystemInputInjector> CreateSystemInputInjector() override {
    return nullptr;
  }

  std::unique_ptr<PlatformWindow> CreatePlatformWindow(
      PlatformWindowDelegate* /*delegate*/,
      const gfx::Rect& /*bounds*/) override {
    return nullptr;
  }

  std::unique_ptr<display::NativeDisplayDelegate> CreateNativeDisplayDelegate()
      override {
    return nullptr;
  }

 private:
  void InitializeUI(const InitParams& /*params*/) override {
    gpu_platform_host_.reset(CreateStubGpuPlatformSupportHost());
  }

  void InitializeGPU(const InitParams& /*params*/) override {
    surface_factory_.reset(new SurfaceFactoryEfl());
  }

  std::unique_ptr<SurfaceFactoryEfl> surface_factory_;
  std::unique_ptr<GpuPlatformSupportHost> gpu_platform_host_;

  DISALLOW_COPY_AND_ASSIGN(OzonePlatformEfl);
};

}  // namespace

OzonePlatform* CreateOzonePlatformEfl() {
  return new OzonePlatformEfl;
}

}  // namespace ui
