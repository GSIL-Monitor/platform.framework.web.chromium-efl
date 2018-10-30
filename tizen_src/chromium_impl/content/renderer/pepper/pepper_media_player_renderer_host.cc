// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/pepper/pepper_media_player_renderer_host.h"

#include <memory>
#include <string>
#include <vector>

#include "cc/blink/web_layer_impl.h"
#include "content/public/renderer/renderer_ppapi_host.h"
#include "content/renderer/render_frame_impl.h"
#include "content/renderer/render_thread_impl.h"
#include "media/base/bind_to_current_loop.h"
#include "media/blink/video_frame_compositor.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/host/host_message_context.h"
#include "ppapi/host/ppapi_host.h"
#include "ppapi/host/resource_host.h"
#include "ppapi/proxy/dispatch_reply_message.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "third_party/WebKit/public/platform/WebURLLoader.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebElement.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebPluginContainer.h"

namespace content {

PepperMediaPlayerRendererHost::PepperMediaPlayerRendererHost(
    RendererPpapiHost* host,
    PP_Instance instance,
    PP_Resource resource,
    PP_MediaPlayerMode /* player_mode */)
    : ResourceHost(host->GetPpapiHost(), instance, resource),
      renderer_ppapi_host_(host),
      bound_instance_(nullptr),
      factory_(this),
      needs_paint_(true) {
  auto* plugin = GetPluginInstance();
  if (plugin) {
    UpdateVideoPosition(plugin, plugin->view_data().rect,
                        plugin->view_data().clip_rect);
    plugin->AddGeometryChangeListener(this);
  } else {
    LOG(FATAL) << "Tried to create a resource for a non-existing plugin.";
  }
  auto render_thread = RenderThreadImpl::current();
  compositor_task_runner_ = render_thread->compositor_task_runner();
  compositor_ = new media::VideoFrameCompositor(
#if defined(TIZEN_VIDEO_HOLE)
      media::BindToCurrentLoop(base::Bind(
          &PepperMediaPlayerRendererHost::OnDrawableContentRectChanged,
          factory_.GetWeakPtr())),
#endif
      compositor_task_runner_,
      base::BindRepeating(
          [](base::OnceCallback<void(viz::ContextProvider*)> callback) {}));

  video_layer_ = cc::VideoLayer::Create(compositor_, media::VIDEO_ROTATION_0);
}

PepperMediaPlayerRendererHost::~PepperMediaPlayerRendererHost() {
  compositor_->SetVideoFrameProviderClient(NULL);
  compositor_task_runner_->DeleteSoon(FROM_HERE, compositor_);

  if (bound_instance_)
    bound_instance_->BindGraphics(pp_instance(), 0);

  auto* plugin = GetPluginInstance();
  if (plugin)
    plugin->RemoveGeometryChangeListener(this);
}

bool PepperMediaPlayerRendererHost::IsMediaPlayerRendererHost() {
  return true;
}

void PepperMediaPlayerRendererHost::ViewInitiatedPaint() {
  if (!needs_paint_)
    return;

#if defined(TIZEN_VIDEO_HOLE)
  auto plugin = GetPluginInstance();
  DCHECK(plugin != nullptr);
  auto view_data = plugin->view_data();

  auto size = gfx::Size{view_data.rect.size.width, view_data.rect.size.height};
  auto video_frame = media::VideoFrame::CreateHoleFrame(size);
  compositor_task_runner_->PostTask(
      FROM_HERE, base::Bind(&media::VideoFrameCompositor::PaintSingleFrame,
                            base::Unretained(compositor_), video_frame, false));
#else
  LOG(ERROR) << "It is not possible to display video without "
             << "TIZEN_VIDEO_HOLE defined";
#endif  // defined(TIZEN_VIDEO_HOLE)

  needs_paint_ = false;
}

bool PepperMediaPlayerRendererHost::BindToInstance(
    PepperPluginInstanceImpl* new_instance) {
  bound_instance_ = new_instance;
  needs_paint_ = true;
  return true;
}

PepperPluginInstanceImpl* PepperMediaPlayerRendererHost::GetPluginInstance() {
  return bound_instance_
             ? bound_instance_
             : static_cast<PepperPluginInstanceImpl*>(
                   renderer_ppapi_host_->GetPluginInstance(pp_instance()));
}

void PepperMediaPlayerRendererHost::OnDrawableContentRectChanged(gfx::Rect rect,
                                                                 bool) {
  auto plugin = GetPluginInstance();
  DCHECK(plugin);

  needs_paint_ = true;
  UpdateVideoPosition(
      plugin,
      PP_MakeRectFromXYWH(rect.x(), rect.y(), rect.width(), rect.height()),
      plugin->view_data().clip_rect);
}

void PepperMediaPlayerRendererHost::GeometryDidChange(const PP_Rect& rect) {
  auto plugin = GetPluginInstance();
  DCHECK(plugin);

  needs_paint_ = true;
  UpdateVideoPosition(plugin, plugin->view_data().rect,
                      plugin->view_data().clip_rect);
}

void PepperMediaPlayerRendererHost::UpdateVideoPosition(
    PepperPluginInstanceImpl* plugin,
    const PP_Rect& hole_rect,
    const PP_Rect& clip_rect) {
  DCHECK(plugin);

  plugin->render_frame()->Send(new PpapiHostMsg_MediaPlayer_OnGeometryChanged(
      pp_instance(), pp_resource(), hole_rect, clip_rect));
}

}  // namespace content
