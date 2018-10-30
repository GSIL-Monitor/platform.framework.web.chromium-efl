// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/plugins/hole_layer.h"

#include <algorithm>

#include "cc/layers/append_quads_data.h"
#include "cc/layers/layer_impl.h"
#include "cc/trees/occlusion.h"
#include "components/viz/common/quads/render_pass.h"
#include "components/viz/common/quads/shared_quad_state.h"
#include "components/viz/common/quads/solid_color_draw_quad.h"

namespace cc {

class HoleLayerImpl : public LayerImpl {
 public:
  static void AppendSolidQuads(viz::RenderPass* render_pass,
                               const Occlusion& occlusion_in_layer_space,
                               viz::SharedQuadState* shared_quad_state,
                               const gfx::Rect& visible_layer_rect,
                               SkColor color,
                               AppendQuadsData* append_quads_data,
                               bool opaque);

  HoleLayerImpl(LayerTreeImpl* tree_impl, int id) : LayerImpl(tree_impl, id) {}
  ~HoleLayerImpl() override {}

  // LayerImpl overrides.
  std::unique_ptr<LayerImpl> CreateLayerImpl(
      LayerTreeImpl* tree_impl) override {
    return base::WrapUnique(new HoleLayerImpl(tree_impl, id()));
  }
  void AppendQuads(viz::RenderPass* render_pass,
                   AppendQuadsData* append_quads_data) override;

 private:
  const char* LayerTypeAsString() const override { return "cc::HoleLayerImpl"; }

  DISALLOW_COPY_AND_ASSIGN(HoleLayerImpl);
};

const int kSolidQuadTileSize = 256;

std::unique_ptr<LayerImpl> HoleLayer::CreateLayerImpl(
    LayerTreeImpl* tree_impl) {
  return base::WrapUnique(new HoleLayerImpl(tree_impl, id()));
}

void HoleLayerImpl::AppendSolidQuads(viz::RenderPass* render_pass,
                                     const Occlusion& occlusion_in_layer_space,
                                     viz::SharedQuadState* shared_quad_state,
                                     const gfx::Rect& visible_layer_rect,
                                     SkColor color,
                                     AppendQuadsData* append_quads_data,
                                     bool opaque) {
  // We create a series of smaller quads instead of just one large one so that
  // the caller can reduce the total pixels drawn.
  for (int x = visible_layer_rect.x(); x < visible_layer_rect.right();
       x += kSolidQuadTileSize) {
    for (int y = visible_layer_rect.y(); y < visible_layer_rect.bottom();
         y += kSolidQuadTileSize) {
      gfx::Rect quad_rect(
          x, y, std::min(visible_layer_rect.right() - x, kSolidQuadTileSize),
          std::min(visible_layer_rect.bottom() - y, kSolidQuadTileSize));
      gfx::Rect visible_quad_rect =
          occlusion_in_layer_space.GetUnoccludedContentRect(quad_rect);
      if (visible_quad_rect.IsEmpty())
        continue;

      append_quads_data->visible_layer_area +=
          visible_quad_rect.width() * visible_quad_rect.height();

      viz::SolidColorDrawQuad* quad =
          render_pass->CreateAndAppendDrawQuad<viz::SolidColorDrawQuad>();
      if (opaque) {
        quad->SetAll(shared_quad_state, quad_rect, visible_quad_rect, false,
                     color, true);
      } else {
        quad->SetNew(shared_quad_state, quad_rect, visible_quad_rect, color,
                     false);
      }
    }
  }
}

void HoleLayerImpl::AppendQuads(viz::RenderPass* render_pass,
                                AppendQuadsData* append_quads_data) {
  viz::SharedQuadState* shared_quad_state =
      render_pass->CreateAndAppendSharedQuadState();
  PopulateSharedQuadState(shared_quad_state, true);

  AppendDebugBorderQuad(render_pass, gfx::Rect(bounds()), shared_quad_state,
                        append_quads_data);

  AppendSolidQuads(render_pass, draw_properties().occlusion_in_content_space,
                   shared_quad_state, gfx::Rect(bounds()), background_color(),
                   append_quads_data, contents_opaque());
}

}  // namespace cc
