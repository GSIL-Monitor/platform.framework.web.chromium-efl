// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <KHR/khrplatform.h>

// GL types are forward declared to avoid including the GL headers. The problem
// is determining which GL headers to include from code that is common to the
// client and service sides (GLES2 or one of several GL implementations).
typedef unsigned int GLenum;
typedef unsigned int GLbitfield;
typedef unsigned int GLuint;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLboolean;
typedef signed char GLbyte;
typedef short GLshort;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned long GLulong;
typedef float GLfloat;
typedef float GLclampf;
typedef double GLdouble;
typedef double GLclampd;
typedef void GLvoid;
typedef khronos_intptr_t GLintptr;
typedef khronos_ssize_t GLsizeiptr;
typedef struct __GLsync* GLsync;
// For desktop build using efl-1.9.x ~ efl-1.11.x
// GLsync, GL*64, EvasGL*64 are not defined by Evas_GL.h
#if defined(OS_TIZEN) || defined(OS_NACL)
typedef khronos_int64_t GLint64;
typedef khronos_uint64_t GLuint64;
#else
typedef int64_t GLint64;
typedef uint64_t GLuint64;
#endif
