// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gl/gl_shared_context_efl.h"

#include <EGL/egl.h>
#include <Ecore_Evas.h>
#include <Evas_GL.h>

#include "base/command_line.h"
#include "base/lazy_instance.h"
#include "base/threading/thread_restrictions.h"
#include "gpu/command_buffer/service/mailbox_manager_impl.h"
#include "gpu/config/scoped_restore_non_owned_evasgl_context.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/gl_share_group.h"
#include "ui/gl/gl_surface.h"
#include "ui/gl/gl_switches.h"
#include "ui/gl/gpu_timing.h"

// Defined in gl_current_context_efl.cc because of conflict in chromium
// and efl gl includes.
extern void* GLGetCurrentContext();
#if defined(DIRECT_CANVAS)
extern void* GLGetCurrentSurface();
#endif
struct GLSharedContextEflPrivate : public gl::GLContext {
  GLSharedContextEflPrivate(Evas_Object* object)
      : GLContext(GLSharedContextEfl::GetShareGroup()) {
#if !defined(TIZEN_VD_DISABLE_GPU)
    Evas* evas = evas_object_evas_get(object);
    evas_gl_config_ = evas_gl_config_new();
    evas_gl_config_->options_bits = EVAS_GL_OPTIONS_NONE;
    evas_gl_config_->color_format = EVAS_GL_RGBA_8888;
    evas_gl_config_->depth_bits = EVAS_GL_DEPTH_BIT_24;
    evas_gl_config_->stencil_bits = EVAS_GL_STENCIL_BIT_8;

    evas_gl_ = evas_gl_new(evas);

    if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
            switches::kDisableES3GLContext)) {
      evas_gl_context_ =
          evas_gl_context_version_create(evas_gl_, 0, EVAS_GL_GLES_3_X);
      if (evas_gl_context_) {
        GLSharedContextEfl::GetShareGroup()->SetUseGLES3(true);
        LOG(INFO) << "GLSharedContextEflPrivate(): Create GLES 3.0 Context";
      }
    }

    if (!evas_gl_context_)
      evas_gl_context_ =
          evas_gl_context_version_create(evas_gl_, 0, EVAS_GL_GLES_2_X);

    if (!evas_gl_context_)
      LOG(FATAL) << "GLSharedContextEflPrivate(): Create evas gl context Fail";

    evas_gl_surface_ = evas_gl_surface_create(evas_gl_, evas_gl_config_, 1, 1);
    if (!evas_gl_surface_)
      LOG(FATAL) << "GLSharedContextEflPrivate(): Create evas gl Surface Fail";

    ScopedRestoreNonOwnedEvasGLContext restore_context;
    evas_gl_make_current(evas_gl_, evas_gl_surface_, evas_gl_context_);
    handle_ = GLGetCurrentContext();
    CHECK(handle_ != EGL_NO_CONTEXT);
#endif
  }

  bool Initialize(gl::GLSurface*, const gl::GLContextAttribs&) override {
    NOTREACHED();
    return false;
  }

  bool MakeCurrent(gl::GLSurface*) override {
    NOTREACHED();
    return false;
  }

  void ReleaseCurrent(gl::GLSurface*) override { NOTREACHED(); }

  bool IsCurrent(gl::GLSurface*) override {
    NOTREACHED();
    return false;
  }

  void* GetHandle() override { return handle_; }

  scoped_refptr<gl::GPUTimingClient> CreateGPUTimingClient() override {
    return 0;
  }

  const gl::ExtensionSet& GetExtensions() override {
    return GLSharedContextEfl::GetInstance()->GetExtensions();
  }

  void OnSetSwapInterval(int interval) override { NOTREACHED(); }
#if defined(DIRECT_CANVAS)
  Evas_GL* GetEvasGL() { return evas_gl_; }
#endif
  void ResetExtensions() override { NOTREACHED(); }

 private:
  friend struct GLSharedContextEfl;

  ~GLSharedContextEflPrivate() override {
    evas_gl_surface_destroy(evas_gl_, evas_gl_surface_);
    evas_gl_context_destroy(evas_gl_, evas_gl_context_);
    evas_gl_config_free(evas_gl_config_);
    evas_gl_free(evas_gl_);
  }

  void* handle_;
  Evas_GL* evas_gl_ = nullptr;
  Evas_GL_Context* evas_gl_context_ = nullptr;
  Evas_GL_Surface* evas_gl_surface_ = nullptr;
  Evas_GL_Config* evas_gl_config_ = nullptr;
};
#if defined(DIRECT_CANVAS)
class GLSurfaceEvasGL : public gl::GLSurface {
 public:
  GLSurfaceEvasGL()
      : ecore_evas_(nullptr),
        evas_(nullptr),
        webview_(nullptr),
        evas_gl_surface_(nullptr),
        initialized_(false) {}

  ~GLSurfaceEvasGL() {}

  void Initialize(Evas_Object* webview,
                  Evas_GL_Surface* surface,
                  int w,
                  int h) {
    webview_ = webview;
    evas_gl_surface_ = surface;
    size_.SetSize(w, h);
    evas_ = evas_object_evas_get(webview_);
    ecore_evas_ = ecore_evas_ecore_evas_get(evas_);
  }

  void Destroy() override {}

  bool IsOffscreen() override { return false; }

  gfx::SwapResult SwapBuffers() override {
    if (!initialized_) {
      initialized_ = true;
      ecore_evas_manual_render_set(ecore_evas_, EINA_TRUE);
    }
    evas_object_image_pixels_dirty_set(webview_, true);

    // Call manual render to force swapbuffer directly.
    ecore_evas_manual_render(ecore_evas_);
    return gfx::SwapResult::SWAP_ACK;
  }

  gfx::Size GetSize() override { return size_; }

  void* GetHandle() override {
    NOTREACHED();
    return nullptr;
  }

  gl::GLSurfaceFormat GetFormat() override { return gl::GLSurfaceFormat(); }

  Evas_GL_Surface* GetEvasGLSurface() { return evas_gl_surface_; }

 private:
  Ecore_Evas* ecore_evas_;
  Evas* evas_;
  Evas_Object* webview_;
  Evas_GL_Surface* evas_gl_surface_;
  gfx::Size size_;
  bool initialized_;
};

class GLContextEvasGL : public gl::GLContextReal {
 public:
  GLContextEvasGL(Evas_GL_Context* context)
      : GLContextReal(nullptr),
        evas_gl_(nullptr),
        evas_gl_context_(nullptr),
        evas_gl_api_(nullptr) {}

  bool Initialize(gl::GLSurface* compatible_surface,
                  const gl::GLContextAttribs& attribs) override {
    evas_gl_ = static_cast<GLSharedContextEflPrivate*>(
                   GLSharedContextEfl::GetInstance())
                   ->GetEvasGL();
    evas_gl_api_ = evas_gl_api_get(evas_gl_);
    return true;
  }

  // Makes the GL context and a surface current on the current thread.
  bool MakeCurrent(gl::GLSurface* surface) override {
    GLSurfaceEvasGL* evas_gl_surface = static_cast<GLSurfaceEvasGL*>(surface);
    bool result = evas_gl_make_current(
        evas_gl_, evas_gl_surface->GetEvasGLSurface(), evas_gl_context_);
    // Set this as soon as the context is current, since we might call into GL.
    BindGLApi();
    GLContext::SetCurrent(surface);
    return result;
  }

  // Release this GL Context and surface as current on the current thread.
  void ReleaseCurrent(gl::GLSurface* surface) override {}

  // Returns true if this context and surface are current.
  // Pass the null surface if the current surface is not important.
  bool IsCurrent(gl::GLSurface* surface) override {
    return false;
  }

  // Get the underlying platform specific GL context "handle".
  void* GetHandle() override {
    NOTREACHED();
    return nullptr;
  }

  void OnSetSwapInterval(int interval) override {}

 private:
  Evas_GL* evas_gl_;
  Evas_GL_Context* evas_gl_context_;
  Evas_GL_API* evas_gl_api_;
};
#endif
namespace {
static GLSharedContextEflPrivate* g_private_part = NULL;
#if defined(DIRECT_CANVAS)
class SharedEvasGL {
 public:
  SharedEvasGL() {}
  ~SharedEvasGL() {}

  scoped_refptr<GLSurfaceEvasGL> surface;
  scoped_refptr<GLContextEvasGL> context;
};

base::LazyInstance<SharedEvasGL>::DestructorAtExit g_shared_evas_gl =
    LAZY_INSTANCE_INITIALIZER;
#endif
}

// static
void GLSharedContextEfl::Initialize(Evas_Object* object) {
  if (!g_private_part) {
    base::ThreadRestrictions::ScopedAllowIO allow_io;
    g_private_part = new GLSharedContextEflPrivate(object);
  }
}

// static
gl::GLContext* GLSharedContextEfl::GetInstance() {
  return g_private_part;
}

// static
Evas_GL_Context* GLSharedContextEfl::GetEvasGLContext() {
  return g_private_part->evas_gl_context_;
}

// static
gl::GLShareGroup* GLSharedContextEfl::GetShareGroup() {
  static scoped_refptr<gl::GLShareGroup> share_group_ = new gl::GLShareGroup();
  return share_group_.get();
}
#if defined(DIRECT_CANVAS)
void GLSharedEvasGL::Initialize(Evas_Object* webview,
                                Evas_GL_Context* context,
                                Evas_GL_Surface* surface,
                                int w,
                                int h) {
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  if (!g_shared_evas_gl.Get().surface)
    g_shared_evas_gl.Get().surface = new GLSurfaceEvasGL();
  if (!g_shared_evas_gl.Get().context)
    g_shared_evas_gl.Get().context = new GLContextEvasGL(context);
  g_shared_evas_gl.Get().surface->Initialize(webview, surface, w, h);
  g_shared_evas_gl.Get().context->Initialize(nullptr, gl::GLContextAttribs());
}

gl::GLSurface* GLSharedEvasGL::GetSurface() {
  return g_shared_evas_gl.Get().surface.get();
}

gl::GLContext* GLSharedEvasGL::GetContext() {
  return g_shared_evas_gl.Get().context.get();
}
#endif
