// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_COMPOSITOR_EVASGL_CONTEXT_PROVIDER_H_
#define CONTENT_BROWSER_COMPOSITOR_EVASGL_CONTEXT_PROVIDER_H_

#include "gpu/command_buffer/client/gles2_implementation_efl.h"

#include "base/compiler_specific.h"
#include "components/viz/common/gpu/context_provider.h"
#include "content/common/content_export.h"
#include "gpu/command_buffer/common/capabilities.h"
#include "gpu/config/gpu_feature_info.h"

namespace skia_bindings {
class GrContextForGLES2Interface;
}

namespace content {

class CONTENT_EXPORT EvasGLContextProvider : public viz::ContextProvider {
 public:
  EvasGLContextProvider(Evas_GL_API* evas_gl_api, Evas_GL* evas_gl);

  // viz::ContextProvider implementation.
  bool BindToCurrentThread() override;
  void DetachFromThread() override;
  const gpu::Capabilities& ContextCapabilities() const override;
  const gpu::GpuFeatureInfo& GetGpuFeatureInfo() const override;
  gpu::gles2::GLES2Interface* ContextGL() override;
  gpu::ContextSupport* ContextSupport() override;
  class GrContext* GrContext() override;
  viz::ContextCacheController* CacheController() override;
  void InvalidateGrContext(uint32_t state) override;
  base::Lock* GetLock() override;
  void SetLostContextCallback(
      const LostContextCallback& lost_context_callback) override;

 private:
  ~EvasGLContextProvider() override;

  gpu::Capabilities capabilities_;
  gpu::GpuFeatureInfo gpu_feature_info_;
  gpu::gles2::GLES2ImplementationEfl evasgl_implementation_;
  std::unique_ptr<viz::ContextCacheController> cache_controller_;
  std::unique_ptr<skia_bindings::GrContextForGLES2Interface> gr_context_;

  DISALLOW_COPY_AND_ASSIGN(EvasGLContextProvider);
};

}  // namespace content

#endif  // CONTENT_BROWSER_COMPOSITOR_EVASGL_CONTEXT_PROVIDER_H_
