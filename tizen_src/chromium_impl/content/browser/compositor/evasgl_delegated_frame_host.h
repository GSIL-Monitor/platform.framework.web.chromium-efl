// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_COMPOSITOR_EVASGL_DELEGATED_FRAME_HOST_H_
#define CONTENT_BROWSER_COMPOSITOR_EVASGL_DELEGATED_FRAME_HOST_H_

#include <Evas_GL.h>

#include "base/memory/weak_ptr.h"
#include "components/viz/common/quads/compositor_frame.h"
#include "components/viz/service/display/direct_renderer.h"
#include "content/common/content_export.h"

// TODO(prashant.n): Include gl_renderer.h in evasgl_delegated_frame_host.cc
// file and forward declare required classes here.

namespace content {

class EvasGLContextProvider;
class EvasGLOutputSurfaceClient;

class CONTENT_EXPORT EvasGLDelegatedFrameHostClient {
 public:
  // TODO(prashant.n): Remove GetEvasGLAPI() after decoupling context provider
  // and surface from EvasGLDelegatedFrameHost.
  virtual Evas_GL_API* GetEvasGLAPI() = 0;
  virtual Evas_GL* GetEvasGL() = 0;
  virtual void DelegatedFrameHostSendReclaimCompositorResources(
      const viz::LocalSurfaceId& local_surface_id,
      const std::vector<viz::ReturnedResource>& resources) = 0;
  virtual bool MakeCurrent() = 0;
  virtual void ClearBrowserFrame(SkColor) = 0;
#if defined(TIZEN_TBM_SUPPORT)
  virtual bool OffscreenRenderingEnabled() = 0;
#endif
};

class CONTENT_EXPORT EvasGLDelegatedFrameHost
    : public base::SupportsWeakPtr<EvasGLDelegatedFrameHost> {
 public:
  explicit EvasGLDelegatedFrameHost(EvasGLDelegatedFrameHostClient* client);
  ~EvasGLDelegatedFrameHost();
  void Initialize();
  void SwapDelegatedFrame(const viz::LocalSurfaceId& local_surface_id,
                          viz::CompositorFrame frame);
  void RenderDelegatedFrame(const gfx::Rect& bounds);
  void SetFBOId(GLuint id);
#if defined(OS_TIZEN_TV_PRODUCT)
  void ClearAllTilesResources();
#endif

#if defined(TIZEN_TBM_SUPPORT)
  void* RenderedOffscreenBuffer();
#endif

 private:
  void UnrefResources(const std::vector<viz::ReturnedResource>& resources);
  bool ShouldTransformRootRenderPass(int rotation);
  void TransformRootRenderPass(viz::RenderPass* root_render_pass,
                               const gfx::Rect& bounds,
                               int rotation);
  void ClearRenderPasses(
      const viz::ResourceIdSet& resources_in_frame = viz::ResourceIdSet());
  void ClearChildId();

  EvasGLDelegatedFrameHostClient* client_;
  bool is_initialized_;
  // TODO(prashant.n): Decouple context provider and surface from this class.
  scoped_refptr<EvasGLContextProvider> context_provider_;
  std::unique_ptr<EvasGLOutputSurfaceClient> output_surface_client_;
  std::unique_ptr<viz::OutputSurface> output_surface_;
  viz::RendererSettings settings_;
  std::unique_ptr<cc::DisplayResourceProvider> resource_provider_;
  int child_id_;
#if defined(OS_TIZEN_TV_PRODUCT)
  viz::LocalSurfaceId local_surface_id_;
#endif
  viz::RenderPassList render_pass_list_;
  viz::ResourceIdSet resources_in_frame_;
  std::vector<viz::ReturnedResource> returned_resources_;
  std::unique_ptr<viz::DirectRenderer> renderer_;
  SkColor background_color_ = SK_ColorTRANSPARENT;

  base::WeakPtrFactory<EvasGLDelegatedFrameHost> weak_ptr_factory_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_COMPOSITOR_EVASGL_DELEGATED_FRAME_HOST_H_
