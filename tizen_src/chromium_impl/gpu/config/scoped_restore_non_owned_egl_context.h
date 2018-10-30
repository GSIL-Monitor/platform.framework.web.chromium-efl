// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SCOPED_RESTORE_NON_OWNED_EGL_CONTEXT_H_
#define SCOPED_RESTORE_NON_OWNED_EGL_CONTEXT_H_

#include "ui/gl/gl_context.h"
#include "ui/gl/gl_surface.h"
#include "ui/gl/gl_surface_egl.h"

class ScopedRestoreNonOwnedEGLContext {
 public:
  ScopedRestoreNonOwnedEGLContext();
  ~ScopedRestoreNonOwnedEGLContext();

 private:
  EGLContext context_;
  EGLDisplay display_;
  EGLSurface draw_surface_;
  EGLSurface read_surface_;
};

#endif  // SCOPED_RESTORE_NON_OWNED_EGL_CONTEXT_H_