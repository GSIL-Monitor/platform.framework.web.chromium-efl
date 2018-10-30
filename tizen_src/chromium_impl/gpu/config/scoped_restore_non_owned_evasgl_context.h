// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SCOPED_RESTORE_NON_OWNED_EVASGL_CONTEXT_H_
#define SCOPED_RESTORE_NON_OWNED_EVASGL_CONTEXT_H_

#include <Evas_GL.h>

class ScopedRestoreNonOwnedEvasGLContext {
 public:
  ScopedRestoreNonOwnedEvasGLContext();
  ~ScopedRestoreNonOwnedEvasGLContext();

  void WillDestorySurface(Evas_GL_Surface* evas_gl_surface);

 private:
  Evas_GL* evas_gl_;
  Evas_GL_Context* evas_gl_context_;
  Evas_GL_Surface* evas_gl_surface_;
};

#endif  // SCOPED_RESTORE_NON_OWNED_EVASGL_CONTEXT_H_
