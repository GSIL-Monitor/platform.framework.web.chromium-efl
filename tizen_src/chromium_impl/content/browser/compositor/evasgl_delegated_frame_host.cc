// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/compositor/evasgl_delegated_frame_host.h"

#include "base/command_line.h"
#include "components/viz/common/quads/draw_quad.h"
#include "components/viz/common/quads/render_pass.h"
#include "components/viz/service/display/gl_renderer.h"
#include "components/viz/service/display/skia_renderer.h"
#include "content/browser/compositor/evasgl_output_surface.h"
#include "content/browser/compositor/evasgl_output_surface_client.h"
#include "gpu/config/scoped_restore_non_owned_evasgl_context.h"
#include "ui/base/ui_base_switches.h"
#include "ui/display/device_display_info_efl.h"

namespace content {

EvasGLDelegatedFrameHost::EvasGLDelegatedFrameHost(
    EvasGLDelegatedFrameHostClient* client)
    : client_(client),
      is_initialized_(false),
      child_id_(0),
      weak_ptr_factory_(this) {}

EvasGLDelegatedFrameHost::~EvasGLDelegatedFrameHost() {
  if (!is_initialized_)
    return;
  ClearRenderPasses();
  ClearChildId();
}

void EvasGLDelegatedFrameHost::Initialize() {
  DCHECK(!renderer_);
  context_provider_ = scoped_refptr<EvasGLContextProvider>(
      new EvasGLContextProvider(client_->GetEvasGLAPI(),
                                client_->GetEvasGL()));

  output_surface_client_.reset(new EvasGLOutputSurfaceClient);
  output_surface_ = std::unique_ptr<viz::OutputSurface>(
      new EvasGLOutputSurface(context_provider_));
#if defined(TIZEN_TBM_SUPPORT)
  if (client_->OffscreenRenderingEnabled()) {
    output_surface_ = std::unique_ptr<viz::OutputSurface>(
        new EvasGLTBMOutputSurface(context_provider_));
  }
#endif

  output_surface_->BindToClient(output_surface_client_.get());

  settings_.resource_settings.texture_id_allocation_chunk_size = 1;
  // TODO: Check if this has to be true.
  constexpr bool delegated_sync_points_required = false;
  resource_provider_ = base::MakeUnique<cc::DisplayResourceProvider>(
      output_surface_->context_provider(), nullptr, nullptr,
      delegated_sync_points_required, settings_.resource_settings);

  child_id_ = resource_provider_->CreateChild(
      base::Bind(&EvasGLDelegatedFrameHost::UnrefResources,
                 weak_ptr_factory_.GetWeakPtr()));

  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(switches::kUseSkiaRenderer)) {
    renderer_ = base::MakeUnique<viz::SkiaRenderer>(
        &settings_, output_surface_.get(), resource_provider_.get());
  } else {
    renderer_ = base::MakeUnique<viz::GLRenderer>(
        &settings_, output_surface_.get(), resource_provider_.get(), nullptr);
  }
  renderer_->Initialize();
  renderer_->SetVisible(true);
  is_initialized_ = true;
}

#if defined(OS_TIZEN)
void EvasGLDelegatedFrameHost::SetFBOId(GLuint fboID) {
  if (output_surface_)
    output_surface_->SetFrameBufferId(fboID);
}
#endif

void EvasGLDelegatedFrameHost::SwapDelegatedFrame(
    const viz::LocalSurfaceId& local_surface_id,
    viz::CompositorFrame frame) {
#if defined(TIZEN_VD_DISABLE_GPU)
  LOG(INFO) << "Disable evas gl for non gpu model";
  return;
#endif

  if (!is_initialized_)
    return;

  DCHECK(renderer_);
  DCHECK(resource_provider_);
  DCHECK(!frame.render_pass_list.empty());
#if defined(OS_TIZEN_TV_PRODUCT)
  local_surface_id_ = local_surface_id;
#endif

  // Remap resource ids for the new resource provider.
  const cc::ResourceProvider::ResourceIdMap& resource_map =
      resource_provider_->GetChildToParentMap(child_id_);

  resource_provider_->ReceiveFromChild(child_id_, frame.resource_list);

  bool invalid_frame = false;
  viz::ResourceIdSet resources_in_frame;
  size_t reserve_size = frame.resource_list.size();
  resources_in_frame.reserve(reserve_size);
  for (const auto& pass : frame.render_pass_list) {
    for (const auto& quad : pass->quad_list) {
      for (viz::ResourceId& resource_id : quad->resources) {
        cc::ResourceProvider::ResourceIdMap::const_iterator it =
            resource_map.find(resource_id);
        if (it == resource_map.end()) {
          LOG(ERROR) << "EvasGLDelegatedFrameHost > Resource is invalid!";
          invalid_frame = true;
          break;
        }
        DCHECK_EQ(it->first, resource_id);
        viz::ResourceId remapped_id = it->second;
        resources_in_frame.insert(resource_id);
        resource_id = remapped_id;
      }
    }
  }

  // Drop new frame data if resource is invalid. Keep the previous frame.
  // [TODO] it does not need to invoke evas_object_image_pixels_dirty_set()
  // when |invalid_frame| is true.
  if (!invalid_frame)
    render_pass_list_ = std::move(frame.render_pass_list);

  ClearRenderPasses(resources_in_frame);

  background_color_ = frame.metadata.root_background_color;

  DCHECK(!invalid_frame);
  client_->DelegatedFrameHostSendReclaimCompositorResources(
      local_surface_id, returned_resources_);
  returned_resources_.clear();
}

void EvasGLDelegatedFrameHost::RenderDelegatedFrame(const gfx::Rect& bounds) {
  if (!is_initialized_)
    return;

  DCHECK(renderer_);

  if (render_pass_list_.empty()) {
    client_->ClearBrowserFrame(SK_ColorTRANSPARENT);
    return;
  }

  client_->ClearBrowserFrame(background_color_);

  display::DeviceDisplayInfoEfl display_info;
  viz::RenderPassList render_pass_list;
  viz::RenderPass::CopyAll(render_pass_list_, &render_pass_list);

  gfx::Size device_viewport_size = gfx::Size(bounds.width(), bounds.height());

  viz::RenderPass* root_render_pass = render_pass_list.back().get();
  root_render_pass->output_rect.set_width(bounds.width());
  root_render_pass->output_rect.set_height(bounds.height());

  int rotation = evas_gl_rotation_get(client_->GetEvasGL());
  // Transform root render pass if rotation is 90 or 180 or 270.
  if (ShouldTransformRootRenderPass(rotation)) {
    if (rotation != 180) {
      root_render_pass->output_rect.set_height(bounds.width());
      root_render_pass->output_rect.set_width(bounds.height());
    }
    TransformRootRenderPass(root_render_pass, bounds, rotation);
  }

  renderer_->DecideRenderPassAllocationsForFrame(render_pass_list);
  // Draw frame by compositing render passes.
  renderer_->DrawFrame(&render_pass_list, display_info.GetDIPScale(),
                       device_viewport_size);

#if defined(TIZEN_TBM_SUPPORT)
  if (client_->OffscreenRenderingEnabled())
    renderer_->SwapBuffers(std::vector<ui::LatencyInfo>());
#endif
}

#if defined(OS_TIZEN_TV_PRODUCT)
void EvasGLDelegatedFrameHost::ClearAllTilesResources() {
  if (!is_initialized_ || !local_surface_id_.is_valid())
    return;
  render_pass_list_.clear();
  ClearRenderPasses();
  resources_in_frame_.clear();
  client_->DelegatedFrameHostSendReclaimCompositorResources(
      local_surface_id_, returned_resources_);
  returned_resources_.clear();
}
#endif

#if defined(TIZEN_TBM_SUPPORT)
void* EvasGLDelegatedFrameHost::RenderedOffscreenBuffer() {
  return output_surface_->RenderedOffscreenBuffer();
}
#endif

void EvasGLDelegatedFrameHost::UnrefResources(
    const std::vector<viz::ReturnedResource>& resources) {
  std::copy(resources.begin(), resources.end(),
            std::back_inserter(returned_resources_));
}

bool EvasGLDelegatedFrameHost::ShouldTransformRootRenderPass(int rotation) {
  return (rotation != 0);
}

void EvasGLDelegatedFrameHost::TransformRootRenderPass(
    viz::RenderPass* root_render_pass,
    const gfx::Rect& bounds,
    int rotation) {
  double scale_x, scale_y, m01, m10, translate_x, translate_y, new_scale_x,
      new_scale_y, new_m01, new_m10, new_translate_x, new_translate_y;

  for (const auto& shared_quad_state :
       root_render_pass->shared_quad_state_list) {
    // Rotate model by matrix transformation
    SkMatrix44& matrix = shared_quad_state->quad_to_target_transform.matrix();

    // Normalize the matrix
    double m33 = matrix.getDouble(3, 3);
    if ((m33 != 0.0) && (m33 != 1.0)) {
      SkMScalar scale = SK_MScalar1 / m33;
      for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
          matrix.setDouble(i, j, matrix.getDouble(i, j) * scale);
    }

    scale_x = matrix.getDouble(0, 0);
    scale_y = matrix.getDouble(1, 1);
    m01 = matrix.getDouble(0, 1);
    m10 = matrix.getDouble(1, 0);
    translate_x = matrix.getDouble(0, 3);
    translate_y = matrix.getDouble(1, 3);

    if (rotation == 90) {
      new_scale_x = m10;
      new_scale_y = -m01;
      new_m01 = scale_y;
      new_m10 = -scale_x;
      new_translate_x = translate_y;
      new_translate_y = bounds.width() - translate_x;
    } else if (rotation == 180) {
      new_scale_x = -scale_x;
      new_scale_y = -scale_y;
      new_m01 = -m01;
      new_m10 = -m10;
      new_translate_x = bounds.width() - translate_x;
      new_translate_y = bounds.height() - translate_y;
    } else {  // 270
      new_scale_x = -m10;
      new_scale_y = m01;
      new_m01 = -scale_y;
      new_m10 = scale_x;
      new_translate_x = bounds.height() - translate_y;
      new_translate_y = translate_x;
    }
    matrix.setDouble(0, 0, new_scale_x);
    matrix.setDouble(1, 1, new_scale_y);
    matrix.setDouble(0, 1, new_m01);
    matrix.setDouble(1, 0, new_m10);
    matrix.setDouble(0, 3, new_translate_x);
    matrix.setDouble(1, 3, new_translate_y);
  }
}

void EvasGLDelegatedFrameHost::ClearRenderPasses(
    const viz::ResourceIdSet& resources_in_frame) {
  // It needs MakeCurrent to release GL resource inside ResourceProvider.
  ScopedRestoreNonOwnedEvasGLContext restore_context;
  client_->MakeCurrent();
  resource_provider_->DeclareUsedResourcesFromChild(child_id_,
                                                    resources_in_frame);
}

void EvasGLDelegatedFrameHost::ClearChildId() {
  if (!child_id_)
    return;

  resource_provider_->DestroyChild(child_id_);
  resources_in_frame_.clear();
  child_id_ = 0;
}

}  // namespace content
