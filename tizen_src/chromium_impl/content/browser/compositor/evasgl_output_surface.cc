// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/compositor/evasgl_output_surface.h"
#include "components/viz/service/display/output_surface_frame.h"

namespace content {

EvasGLOutputSurface::EvasGLOutputSurface(
    const scoped_refptr<viz::ContextProvider>& context_provider)
    : OutputSurface(context_provider), fbo_id_(0) {}

EvasGLOutputSurface::~EvasGLOutputSurface() {}

void EvasGLOutputSurface::BindToClient(viz::OutputSurfaceClient* client) {}

void EvasGLOutputSurface::EnsureBackbuffer() {}

void EvasGLOutputSurface::DiscardBackbuffer() {}

void EvasGLOutputSurface::BindFramebuffer() {
  context_provider_->ContextGL()->BindFramebuffer(GL_FRAMEBUFFER, fbo_id_);
}

void EvasGLOutputSurface::SetDrawRectangle(const gfx::Rect& rect) {}

void EvasGLOutputSurface::Reshape(const gfx::Size& size,
                                  float device_scale_factor,
                                  const gfx::ColorSpace& color_space,
                                  bool has_alpha,
                                  bool use_stencil) {}

bool EvasGLOutputSurface::HasExternalStencilTest() const {
  return false;
}

void EvasGLOutputSurface::ApplyExternalStencil() {}

void EvasGLOutputSurface::SwapBuffers(viz::OutputSurfaceFrame frame) {}

viz::OverlayCandidateValidator*
EvasGLOutputSurface::GetOverlayCandidateValidator() const {
  return nullptr;
}

bool EvasGLOutputSurface::IsDisplayedAsOverlayPlane() const {
  return false;
}

unsigned EvasGLOutputSurface::GetOverlayTextureId() const {
  return 0;
}

gfx::BufferFormat EvasGLOutputSurface::GetOverlayBufferFormat() const {
  return gfx::BufferFormat::RGBX_8888;
}

bool EvasGLOutputSurface::SurfaceIsSuspendForRecycle() const {
  return false;
}

uint32_t EvasGLOutputSurface::GetFramebufferCopyTextureFormat() {
  return 0;
}

#if defined(OS_TIZEN)
void EvasGLOutputSurface::SetFrameBufferId(unsigned int fboID) {
  fbo_id_ = fboID;
}
#endif

#if defined(TIZEN_TBM_SUPPORT)
EvasGLTBMBuffer::EvasGLTBMBuffer()
    : image_(0),
      tbm_surface_(0),
      render_target_texture_(0),
      surface_(0){
}

EvasGLTBMBuffer::~EvasGLTBMBuffer() {
}

void EvasGLTBMBuffer::BindToSurface(EvasGLTBMOutputSurface* surface) {
  surface_ = surface;
}

void EvasGLTBMBuffer::Create(int width, int height) {
  if (!surface_) {
    LOG(ERROR) << "Surface is not bound.";
    return;
  }

  auto gl = surface_->ContextGLEfl();
  tbm_surface_ = tbm_surface_create(width, height, TBM_FORMAT_ARGB8888);
  if (tbm_surface_)
    image_ = gl->CreateImage(tbm_surface_);
  else
    LOG(ERROR) << "Failed to create tbm surface.";

  gl->GenTextures(1, &render_target_texture_);
  gl->BindTexture(GL_TEXTURE_2D, render_target_texture_);
  gl->ImageTargetTexture2DOES(image_);
}

void EvasGLTBMBuffer::Destory() {
  if (!surface_) {
    LOG(ERROR) << "Surface is not bound.";
    return;
  }

  auto gl = surface_->ContextGLEfl();
  if (image_) {
    gl->DestroyImage(image_);
    image_ = 0;
  }

  if (tbm_surface_) {
    if (tbm_surface_destroy(tbm_surface_) != TBM_SURFACE_ERROR_NONE)
      LOG(ERROR) << "Failed to destroy tbm surface.";
    tbm_surface_ = 0;
  }

  gl->DeleteTextures(1, &render_target_texture_);
  render_target_texture_ = 0;
}

void EvasGLTBMBuffer::BindBuffer() {
  auto gl = surface_->ContextGLEfl();
  gl->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           render_target_texture_, 0);
}

EvasGLTBMOutputSurface::EvasGLTBMOutputSurface(
    const scoped_refptr<viz::ContextProvider>& context_provider)
    : EvasGLOutputSurface(context_provider),
      offscreen_framebuffer_id_(0),
      current_buffer_(0),
      rendered_buffer_(0) {
  capabilities_.uses_default_gl_framebuffer = false;
  capabilities_.flipped_output_surface = true;

  CreateGLResource();
  for (int i = 0; i < TBM_BUFFER_SIZE; i++)
    tbm_buffer_[i].BindToSurface(this);
}

EvasGLTBMOutputSurface::~EvasGLTBMOutputSurface() {
  DestroySurface();
  DestroyGLResource();
  for (int i = 0; i < TBM_BUFFER_SIZE; i++)
    tbm_buffer_[i].BindToSurface(0);
}

gpu::gles2::GLES2ImplementationEfl* EvasGLTBMOutputSurface::ContextGLEfl() {
  return static_cast<gpu::gles2::GLES2ImplementationEfl*>(
      context_provider_->ContextGL());
}

void EvasGLTBMOutputSurface::CreateGLResource() {
  ContextGLEfl()->GenFramebuffers(1, &offscreen_framebuffer_id_);
}

void EvasGLTBMOutputSurface::DestroyGLResource() {
  ContextGLEfl()->DeleteFramebuffers(1, &offscreen_framebuffer_id_);
  offscreen_framebuffer_id_ = 0;
}

void EvasGLTBMOutputSurface::CreateSurface(int width, int height) {
  for (int i = 0; i < TBM_BUFFER_SIZE; i++)
    tbm_buffer_[i].Create(width, height);
}

void EvasGLTBMOutputSurface::DestroySurface() {
  for (int i = 0; i < TBM_BUFFER_SIZE; i++)
    tbm_buffer_[i].Destory();
}

void EvasGLTBMOutputSurface::Reshape(const gfx::Size& size,
                                     float device_scale_factor,
                                     const gfx::ColorSpace& color_space,
                                     bool has_alpha,
                                     bool use_stencil) {
  DestroySurface();
  CreateSurface(size.width(), size.height());
}

void EvasGLTBMOutputSurface::BindFramebuffer() {
  auto gl = ContextGLEfl();
  gl->BindFramebuffer(GL_FRAMEBUFFER, offscreen_framebuffer_id_);
  tbm_buffer_[current_buffer_].BindBuffer();
}

void EvasGLTBMOutputSurface::SwapBuffers(viz::OutputSurfaceFrame frame) {
  rendered_buffer_ = current_buffer_;
  current_buffer_++;
  if (current_buffer_ >= TBM_BUFFER_SIZE)
    current_buffer_ = 0;
}

void* EvasGLTBMOutputSurface::RenderedOffscreenBuffer() {
  return tbm_buffer_[rendered_buffer_].TBMSurface();
}
#endif
}  // namespace content
