// Copyright (c) 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromium_impl/build/tizen_version.h"
#include "ui/ozone/surface_factory_efl.h"

#include "ecore_x_wayland_wrapper.h"

#include "ui/gl/gl_surface_egl.h"
#include "ui/ozone/common/egl_util.h"
#include "ui/ozone/common/gl_ozone_egl.h"

namespace ui {

namespace {

class GLOzoneEGLEfl : public GLOzoneEGL {
 public:
  GLOzoneEGLEfl() {}
  ~GLOzoneEGLEfl() override {}

  scoped_refptr<gl::GLSurface> CreateViewGLSurface(
      gfx::AcceleratedWidget window) override {
    // We use offscreen surface for EFL port.
    return nullptr;
  }

  scoped_refptr<gl::GLSurface> CreateOffscreenGLSurface(
      const gfx::Size& size) override {
    if (gl::GLSurfaceEGL::IsEGLSurfacelessContextSupported() &&
        size.width() == 0 && size.height() == 0) {
      return gl::InitializeGLSurface(new gl::SurfacelessEGL(size));
    } else {
      return gl::InitializeGLSurface(new gl::PbufferGLSurfaceEGL(size));
    }
  }

 protected:
  intptr_t GetNativeDisplay() override {
#if defined(USE_WAYLAND)
#if TIZEN_VERSION_AT_LEAST(5, 0, 0)
    Ecore_Wl2_Display *wl2_display = ecore_wl2_connected_display_get(NULL);
    return reinterpret_cast<intptr_t>(ecore_wl2_display_get(wl2_display));
#else
    return reinterpret_cast<intptr_t>(ecore_wl_display_get());
#endif  // TIZEN_VERSION_AT_LEAST(5, 0, 0)
#else
    return reinterpret_cast<intptr_t>(ecore_x_display_get());
#endif
  }

  bool LoadGLES2Bindings(gl::GLImplementation implementation) override {
    return LoadDefaultEGLGLES2Bindings(implementation);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(GLOzoneEGLEfl);
};

}  // namespace

SurfaceFactoryEfl::SurfaceFactoryEfl() {
  egl_implementation_.reset(new GLOzoneEGLEfl());
}

SurfaceFactoryEfl::~SurfaceFactoryEfl() {}

std::vector<gl::GLImplementation>
SurfaceFactoryEfl::GetAllowedGLImplementations() {
  std::vector<gl::GLImplementation> impls;
  impls.push_back(gl::kGLImplementationEGLGLES2);
  return impls;
}

GLOzone* SurfaceFactoryEfl::GetGLOzone(gl::GLImplementation implementation) {
  switch (implementation) {
    case gl::kGLImplementationEGLGLES2:
      return egl_implementation_.get();
    default:
      return nullptr;
  }
}

}  // namespace ui
