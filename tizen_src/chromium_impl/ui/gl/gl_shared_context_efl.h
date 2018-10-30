// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GL_GL_SHARED_CONTEXT_EFL_H
#define GL_GL_SHARED_CONTEXT_EFL_H

#include <Evas.h>

#include "ui/gl/gl_export.h"

typedef struct _Evas_GL_Context Evas_GL_Context;
typedef struct _Evas_GL_Surface Evas_GL_Surface;

namespace gl {
  class GLContext;
  class GLShareGroup;
  class GLSurface;
}

struct GL_EXPORT GLSharedContextEfl {
  static void Initialize(Evas_Object* object);
  static gl::GLContext* GetInstance();
  static Evas_GL_Context* GetEvasGLContext();
  static gl::GLShareGroup* GetShareGroup();
};

struct GL_EXPORT GLSharedEvasGL {
  static void Initialize(Evas_Object* webview,
                         Evas_GL_Context* context,
                         Evas_GL_Surface* surface,
                         int w,
                         int h);
  static gl::GLSurface* GetSurface();
  static gl::GLContext* GetContext();
};

#endif
