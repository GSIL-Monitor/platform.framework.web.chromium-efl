// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_PEPPER_PEPPER_MEDIA_PLAYER_RENDERER_HOST_H_
#define CONTENT_RENDERER_PEPPER_PEPPER_MEDIA_PLAYER_RENDERER_HOST_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "cc/layers/video_layer.h"
#include "content/renderer/pepper/pepper_plugin_instance_impl.h"
#include "media/blink/video_frame_compositor.h"
#include "ppapi/c/pp_rect.h"
#include "ppapi/c/samsung/pp_media_player_samsung.h"
#include "ppapi/host/resource_host.h"
#include "third_party/WebKit/public/platform/WebURLLoaderClient.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"

namespace blink {
class WebFrame;
class WebURLLoader;
}  // namespace blink

namespace content {

class RendererPpapiHost;

class PepperMediaPlayerRendererHost
    : public ppapi::host::ResourceHost,
      public PepperPluginInstanceImpl::GeometryChangeListener {
 public:
  PepperMediaPlayerRendererHost(RendererPpapiHost*,
                                PP_Instance,
                                PP_Resource,
                                PP_MediaPlayerMode);
  ~PepperMediaPlayerRendererHost() override;

  // PepperPluginInstanceImpl::GeometryChangeListener implementation
  void GeometryDidChange(const PP_Rect&) override;

  bool IsMediaPlayerRendererHost() override;
  void ViewInitiatedPaint();
  bool BindToInstance(PepperPluginInstanceImpl* new_instance);

  scoped_refptr<cc::VideoLayer> video_layer() { return video_layer_; }

 private:
  PepperPluginInstanceImpl* GetPluginInstance();

  // Update video rect position on the brwoser-side player host. This methods
  // initiates an IPC data exchange with BROWSER process.
  void UpdateVideoPosition(PepperPluginInstanceImpl* plugin,
                           const PP_Rect& hole_rect,
                           const PP_Rect& clip_rect);

  // Compositor CBs:
  void OnDrawableContentRectChanged(gfx::Rect rect, bool);

  RendererPpapiHost* renderer_ppapi_host_;
  PepperPluginInstanceImpl* bound_instance_;
  base::WeakPtrFactory<PepperMediaPlayerRendererHost> factory_;

  scoped_refptr<cc::VideoLayer> video_layer_;
  scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner_;
  media::VideoFrameCompositor* compositor_;

  bool needs_paint_;

  DISALLOW_COPY_AND_ASSIGN(PepperMediaPlayerRendererHost);
};

}  // namespace content

#endif  // CONTENT_RENDERER_PEPPER_PEPPER_MEDIA_PLAYER_RENDERER_HOST_H_
