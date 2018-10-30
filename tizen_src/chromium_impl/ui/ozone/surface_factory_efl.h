// Copyright (c) 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OZONE_SURFACE_FACTORY_EFL
#define OZONE_SURFACE_FACTORY_EFL

#include "ui/ozone/public/surface_factory_ozone.h"

namespace ui {

// SurfaceFactoryOzone implementation for EFL.
class SurfaceFactoryEfl : public SurfaceFactoryOzone {
 public:
  SurfaceFactoryEfl();
  ~SurfaceFactoryEfl() override;

  // SurfaceFactoryOzone:
  std::vector<gl::GLImplementation> GetAllowedGLImplementations() override;
  GLOzone* GetGLOzone(gl::GLImplementation implementation) override;

 private:
  std::unique_ptr<GLOzone> egl_implementation_;

  DISALLOW_COPY_AND_ASSIGN(SurfaceFactoryEfl);
};

}  // namespace ui

#endif // OZONE_SURFACE_FACTORY_EFL
