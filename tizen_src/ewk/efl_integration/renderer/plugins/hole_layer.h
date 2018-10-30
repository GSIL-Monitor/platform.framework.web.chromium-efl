// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_EFL_INTEGRATION_RENDERER_PLUGINS_HOLE_LAYER_H_
#define EWK_EFL_INTEGRATION_RENDERER_PLUGINS_HOLE_LAYER_H_

#include "base/macros.h"
#include "cc/blink/web_layer_impl.h"
#include "cc/layers/layer.h"

namespace cc {

class HoleLayer : public Layer {
 public:
  HoleLayer() {}

  std::unique_ptr<LayerImpl> CreateLayerImpl(LayerTreeImpl* tree_impl) override;

 private:
  ~HoleLayer() override {}
  DISALLOW_COPY_AND_ASSIGN(HoleLayer);
};

}  // namespace cc

#endif  // EWK_EFL_INTEGRATION_RENDERER_PLUGINS_HOLE_LAYER_H_
