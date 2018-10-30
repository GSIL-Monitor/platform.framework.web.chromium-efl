// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/compositor/evasgl_context_provider.h"

#include "gpu/skia_bindings/grcontext_for_gles2_interface.h"
#include "third_party/skia/include/gpu/GrContext.h"

namespace content {

EvasGLContextProvider::EvasGLContextProvider(Evas_GL_API* evas_gl_api,
                                             Evas_GL* evas_gl)
    : evasgl_implementation_(evas_gl_api, evas_gl) {
  cache_controller_.reset(
      new viz::ContextCacheController(&evasgl_implementation_, nullptr));
}

EvasGLContextProvider::~EvasGLContextProvider() {}

bool EvasGLContextProvider::BindToCurrentThread() {
  return true;
}

void EvasGLContextProvider::DetachFromThread() {}

const gpu::Capabilities& EvasGLContextProvider::ContextCapabilities() const {
  return capabilities_;
}

const gpu::GpuFeatureInfo& EvasGLContextProvider::GetGpuFeatureInfo() const {
  return gpu_feature_info_;
}

gpu::gles2::GLES2Interface* EvasGLContextProvider::ContextGL() {
  return &evasgl_implementation_;
}

gpu::ContextSupport* EvasGLContextProvider::ContextSupport() {
  return &evasgl_implementation_;
}

class GrContext* EvasGLContextProvider::GrContext() {
  if (gr_context_)
    return gr_context_->get();

  gr_context_.reset(new skia_bindings::GrContextForGLES2Interface(
      ContextGL(), ContextCapabilities()));
  cache_controller_->SetGrContext(gr_context_->get());

  // If GlContext is already lost, also abandon the new GrContext.
  if (gr_context_->get() &&
      ContextGL()->GetGraphicsResetStatusKHR() != GL_NO_ERROR) {
    gr_context_->get()->abandonContext();
  }

  return gr_context_->get();
}

viz::ContextCacheController* EvasGLContextProvider::CacheController() {
  return cache_controller_.get();
}

void EvasGLContextProvider::InvalidateGrContext(uint32_t state) {
  if (gr_context_)
    gr_context_->ResetContext(state);
}

base::Lock* EvasGLContextProvider::GetLock() {
  return nullptr;
}

void EvasGLContextProvider::SetLostContextCallback(
    const LostContextCallback& lost_context_callback) {}

}  // namespace content
