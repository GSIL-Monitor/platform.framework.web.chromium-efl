// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_COMPOSITOR_EVASGL_OUTPUT_SURFACE_H_
#define CONTENT_BROWSER_COMPOSITOR_EVASGL_OUTPUT_SURFACE_H_

#include "base/memory/ref_counted.h"
#include "components/viz/service/display/output_surface.h"
#include "content/browser/compositor/evasgl_context_provider.h"
#include "content/common/content_export.h"
#include "ui/gfx/geometry/size.h"

namespace content {
class CONTENT_EXPORT EvasGLOutputSurface : public viz::OutputSurface {
 public:
  EvasGLOutputSurface(
      const scoped_refptr<viz::ContextProvider>& context_provider);
  ~EvasGLOutputSurface() override;

  // viz::OutputSurface implementation.
  void BindToClient(viz::OutputSurfaceClient* client) override;
  void EnsureBackbuffer() override;
  void DiscardBackbuffer() override;
  void BindFramebuffer() override;
  void SetDrawRectangle(const gfx::Rect& rect) override;
  void Reshape(const gfx::Size& size,
               float device_scale_factor,
               const gfx::ColorSpace& color_space,
               bool has_alpha,
               bool use_stencil) override;
  bool HasExternalStencilTest() const override;
  void ApplyExternalStencil() override;
  void SwapBuffers(viz::OutputSurfaceFrame frame) override;
  viz::OverlayCandidateValidator* GetOverlayCandidateValidator() const override;
  bool IsDisplayedAsOverlayPlane() const override;
  unsigned GetOverlayTextureId() const override;
  gfx::BufferFormat GetOverlayBufferFormat() const override;
  bool SurfaceIsSuspendForRecycle() const override;
  uint32_t GetFramebufferCopyTextureFormat() override;
#if defined(OS_TIZEN)
  void SetFrameBufferId(unsigned int fboID) override;
#endif

 private:
  unsigned int fbo_id_;
};

#if defined(TIZEN_TBM_SUPPORT)
#define TBM_BUFFER_SIZE 1
class EvasGLTBMOutputSurface;

class EvasGLTBMBuffer {
 public:
  EvasGLTBMBuffer();
  ~EvasGLTBMBuffer();

  void BindToSurface(EvasGLTBMOutputSurface* surface);
  void Create(int width, int height);
  void Destory();
  void BindBuffer();

  void* TBMSurface() { return tbm_surface_; }
 private:
  EvasGLImage image_;
  tbm_surface_h tbm_surface_;
  GLuint render_target_texture_;

  EvasGLTBMOutputSurface* surface_;
};

class CONTENT_EXPORT EvasGLTBMOutputSurface : public EvasGLOutputSurface {
 public:
  EvasGLTBMOutputSurface(
      const scoped_refptr<viz::ContextProvider>& context_provider);
  ~EvasGLTBMOutputSurface() override;

  void BindFramebuffer() override;
  void Reshape(const gfx::Size& size,
               float device_scale_factor,
               const gfx::ColorSpace& color_space,
               bool has_alpha,
               bool use_stencil) override;
  void SwapBuffers(viz::OutputSurfaceFrame frame) override;

  void* RenderedOffscreenBuffer() override;
  gpu::gles2::GLES2ImplementationEfl* ContextGLEfl();

 private:
  void CreateGLResource();
  void DestroyGLResource();

  void CreateSurface(int width, int height);
  void DestroySurface();

  EvasGLTBMBuffer tbm_buffer_[TBM_BUFFER_SIZE];
  unsigned offscreen_framebuffer_id_;
  int current_buffer_;
  int rendered_buffer_;
};
#endif
}  // namespace content

#endif  // CONTENT_RENDERER_GPU_OUTPUT_SURFACE_Efl_H_
