// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gl/egl_util.h"

#include "build/build_config.h"

#if defined(OS_ANDROID)
#include <EGL/egl.h>
#else
#include "third_party/khronos/EGL/egl.h"
#endif

// This needs to be after the EGL includes
#include "ui/gl/gl_bindings.h"

#if defined(TIZEN_TBM_SUPPORT) && defined(OS_TIZEN_TV_PRODUCT)
#if !defined(TIZEN_VD_DISABLE_GPU)
#include <libvr360/vr360_tz.h>
#endif
#endif

namespace ui {

const char* GetEGLErrorString(uint32_t error) {
  switch (error) {
    case EGL_SUCCESS:
      return "EGL_SUCCESS";
    case EGL_NOT_INITIALIZED:
      return "EGL_NOT_INITIALIZED";
    case EGL_BAD_ACCESS:
      return "EGL_BAD_ACCESS";
    case EGL_BAD_ALLOC:
      return "EGL_BAD_ALLOC";
    case EGL_BAD_ATTRIBUTE:
      return "EGL_BAD_ATTRIBUTE";
    case EGL_BAD_CONFIG:
      return "EGL_BAD_CONFIG";
    case EGL_BAD_CONTEXT:
      return "EGL_BAD_CONTEXT";
    case EGL_BAD_CURRENT_SURFACE:
      return "EGL_BAD_CURRENT_SURFACE";
    case EGL_BAD_DISPLAY:
      return "EGL_BAD_DISPLAY";
    case EGL_BAD_MATCH:
      return "EGL_BAD_MATCH";
    case EGL_BAD_NATIVE_PIXMAP:
      return "EGL_BAD_NATIVE_PIXMAP";
    case EGL_BAD_NATIVE_WINDOW:
      return "EGL_BAD_NATIVE_WINDOW";
    case EGL_BAD_PARAMETER:
      return "EGL_BAD_PARAMETER";
    case EGL_BAD_SURFACE:
      return "EGL_BAD_SURFACE";
    case EGL_CONTEXT_LOST:
      return "EGL_CONTEXT_LOST";
    default:
      return "UNKNOWN";
  }
}

// Returns the last EGL error as a string.
const char* GetLastEGLErrorString() {
  return GetEGLErrorString(eglGetError());
}

#if defined(TIZEN_TBM_SUPPORT) && defined(OS_TIZEN_TV_PRODUCT)
void SetSecureModeGPU(TzGPUmode gpu_mode) {
#if !defined(TIZEN_VD_DISABLE_GPU)
  vr360_tz_handle tz_handle = nullptr;
  static int current_gpu_mode = VR360_TZ_GPU_MODE_NONSECURE;
  int set_gpu_mode;

  vr360_tz_initialize(&tz_handle);

  if (gpu_mode == TzGPUmode::SECURE_MODE)
    set_gpu_mode = VR360_TZ_GPU_MODE_SECURE;
  else
    set_gpu_mode = VR360_TZ_GPU_MODE_NONSECURE;

  if (set_gpu_mode == current_gpu_mode) {
    LOG(INFO) << "[TZ] Already current mode: " << gpu_mode;
  } else {
    vr360_tz_set_gpu_mode(tz_handle, set_gpu_mode);
    current_gpu_mode = set_gpu_mode;
    LOG(INFO) << "[TZ] gpu secure mode : "
              << (VR360_TZ_GPU_MODE_SECURE == current_gpu_mode);
  }

  vr360_tz_finalize(tz_handle);
#endif
}
#endif

}  // namespace ui
