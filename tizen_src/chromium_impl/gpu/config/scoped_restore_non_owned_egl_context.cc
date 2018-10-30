// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/config/scoped_restore_non_owned_egl_context.h"

ScopedRestoreNonOwnedEGLContext::ScopedRestoreNonOwnedEGLContext()
    : context_(EGL_NO_CONTEXT),
      display_(EGL_NO_DISPLAY),
      draw_surface_(EGL_NO_SURFACE),
      read_surface_(EGL_NO_SURFACE) {
#if !defined(TIZEN_VD_DISABLE_GPU)
  // This should only used to restore a context that is not created or owned by
  // Chromium native code, but created by Tizen system itself.
  DCHECK(!gl::GLContext::GetCurrent());

  context_ = eglGetCurrentContext();
  display_ = eglGetCurrentDisplay();
  draw_surface_ = eglGetCurrentSurface(EGL_DRAW);
  read_surface_ = eglGetCurrentSurface(EGL_READ);
#endif
}

ScopedRestoreNonOwnedEGLContext::~ScopedRestoreNonOwnedEGLContext() {
  if (context_ == EGL_NO_CONTEXT || display_ == EGL_NO_DISPLAY ||
      draw_surface_ == EGL_NO_SURFACE || read_surface_ == EGL_NO_SURFACE)
    return;

  if (!eglMakeCurrent(display_, draw_surface_, read_surface_, context_))
    LOG(WARNING) << "Failed to restore EGL context";
}

