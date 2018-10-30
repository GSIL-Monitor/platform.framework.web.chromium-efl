// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_COMPOSITOR_EVASGL_OUTPUT_SURFACE_CLIENT_H_
#define CONTENT_BROWSER_COMPOSITOR_EVASGL_OUTPUT_SURFACE_CLIENT_H_

#include "components/viz/service/display/output_surface_client.h"

namespace content {

class EvasGLOutputSurfaceClient : public viz::OutputSurfaceClient {
 public:
  EvasGLOutputSurfaceClient() = default;

  void SetNeedsRedrawRect(const gfx::Rect& damage_rect) override {}
  void DidReceiveSwapBuffersAck() override {}
  void DidReceiveTextureInUseResponses(
      const gpu::TextureInUseResponses& responses) override {}
};

}  // namespace content

#endif  // CONTENT_BROWSER_COMPOSITOR_EVASGL_OUTPUT_SURFACE_CLIENT_H_
