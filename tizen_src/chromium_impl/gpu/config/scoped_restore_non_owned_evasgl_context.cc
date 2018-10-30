// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "gpu/config/scoped_restore_non_owned_evasgl_context.h"

ScopedRestoreNonOwnedEvasGLContext::ScopedRestoreNonOwnedEvasGLContext()
    : evas_gl_(NULL), evas_gl_context_(NULL), evas_gl_surface_(NULL) {
  evas_gl_ = evas_gl_current_evas_gl_get(NULL, NULL);
  if (evas_gl_) {
    evas_gl_context_ = evas_gl_current_context_get(evas_gl_);
    evas_gl_surface_ = evas_gl_current_surface_get(evas_gl_);
  }
}

ScopedRestoreNonOwnedEvasGLContext::~ScopedRestoreNonOwnedEvasGLContext() {
  if (!evas_gl_)
    return;

  if (!evas_gl_context_ || !evas_gl_surface_) {
    evas_gl_make_current(evas_gl_, NULL, NULL);
    return;
  }

  if (!evas_gl_make_current(evas_gl_, evas_gl_surface_, evas_gl_context_))
    LOG(WARNING) << "Failed to restore EvasGL context";
}

void ScopedRestoreNonOwnedEvasGLContext::WillDestorySurface(
  Evas_GL_Surface* evas_gl_surface) {
  if (evas_gl_surface == evas_gl_surface_) {
    LOG(WARNING) << "Surface is destroyed";
    evas_gl_surface_ = NULL;
  }
}
