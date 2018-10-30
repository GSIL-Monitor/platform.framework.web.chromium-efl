// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/pepper/pepper_compositor_host.h"

#include <stddef.h>
#include <limits>
#include <utility>

#include "base/logging.h"
#include "base/memory/shared_memory.h"
#include "cc/blink/web_layer_impl.h"
#include "cc/layers/layer.h"
#include "cc/layers/solid_color_layer.h"
#include "cc/layers/texture_layer.h"
#include "cc/trees/layer_tree_host.h"
#include "components/viz/client/client_shared_bitmap_manager.h"
#include "components/viz/common/quads/texture_mailbox.h"
#include "content/public/renderer/renderer_ppapi_host.h"
#include "content/renderer/pepper/gfx_conversion.h"
#include "content/renderer/pepper/host_globals.h"
#include "content/renderer/pepper/pepper_plugin_instance_impl.h"
#include "content/renderer/pepper/ppb_image_data_impl.h"
#include "content/renderer/render_thread_impl.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/host/ppapi_host.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppb_image_data_api.h"
#include "third_party/khronos/GLES2/gl2.h"
#include "ui/gfx/geometry/size_conversions.h"
#include "ui/gfx/transform.h"
#if defined(TIZEN_PEPPER_EXTENSIONS)
#include "cc/layers/video_layer.h"
#include "media/base/bind_to_current_loop.h"
#endif

using ppapi::host::HostMessageContext;
using ppapi::thunk::EnterResourceNoLock;
using ppapi::thunk::PPB_ImageData_API;

namespace content {

namespace {

bool CheckPPFloatRect(const PP_FloatRect& rect, float width, float height) {
    const float kEpsilon = std::numeric_limits<float>::epsilon();
    return (rect.point.x >= -kEpsilon &&
            rect.point.y >= -kEpsilon &&
            rect.point.x + rect.size.width <= width + kEpsilon &&
            rect.point.y + rect.size.height <= height + kEpsilon);
}

int32_t VerifyCommittedLayer(const ppapi::CompositorLayerData* old_layer,
                             const ppapi::CompositorLayerData* new_layer,
                             std::unique_ptr<base::SharedMemory>* image_shm) {
  if (!new_layer->is_valid()) {
    LOG(ERROR) << "Invalid new layer";
    return PP_ERROR_BADARGUMENT;
  }

  if (new_layer->color) {
    // Make sure the old layer is a color layer too.
    if (old_layer && !old_layer->color) {
      LOG(ERROR) << "Invalid old layer type";
      return PP_ERROR_BADARGUMENT;
    }
    return PP_OK;
  }

#if defined(TIZEN_PEPPER_EXTENSIONS)
  if (new_layer->background_plane) {
    // Make sure the old layer is a background_plane layer too.
    if (old_layer && !old_layer->background_plane) {
      LOG(ERROR) << "Invalid old layer type";
      return PP_ERROR_BADARGUMENT;
    }
    return PP_OK;
  }
#endif

  if (new_layer->texture) {
    if (old_layer) {
      // Make sure the old layer is a texture layer too.
      if (!old_layer->texture) {
        LOG(ERROR) << "Invalid old layer type";
        return PP_ERROR_BADARGUMENT;
      }
      // The mailbox should be same, if the resource_id is not changed.
      if (new_layer->common.resource_id == old_layer->common.resource_id) {
        if (new_layer->texture->mailbox != old_layer->texture->mailbox) {
          LOG(ERROR) << "Different layers' mailboxes";
          return PP_ERROR_BADARGUMENT;
        }
        return PP_OK;
      }
    }
    if (!new_layer->texture->mailbox.Verify()) {
      LOG(ERROR) << "Invalid new layer mailbox";
      return PP_ERROR_BADARGUMENT;
    }

    // Make sure the source rect is not beyond the dimensions of the
    // texture.
    if (!CheckPPFloatRect(new_layer->texture->source_rect, 1.0f, 1.0f)) {
      LOG(ERROR) << "Source rect is beyond the dimensions of the texture";
      return PP_ERROR_BADARGUMENT;
    }
    return PP_OK;
  }

  if (new_layer->image) {
    if (old_layer) {
      // Make sure the old layer is an image layer too.
      if (!old_layer->image) {
        LOG(ERROR) << "Invalid old layer type";
        return PP_ERROR_BADARGUMENT;
      }
      // The image data resource should be same, if the resource_id is not
      // changed.
      if (new_layer->common.resource_id == old_layer->common.resource_id) {
        if (new_layer->image->resource != old_layer->image->resource) {
          LOG(ERROR) << "Different layers' image data resources";
          return PP_ERROR_BADARGUMENT;
        }
        return PP_OK;
      }
    }

    EnterResourceNoLock<PPB_ImageData_API> enter(new_layer->image->resource,
                                                 true);
    if (enter.failed()) {
      LOG(ERROR) << "Enter ImageData failed";
      return PP_ERROR_BADRESOURCE;
    }

    // TODO(penghuang): support all kinds of image.
    PP_ImageDataDesc desc;
    if (enter.object()->Describe(&desc) != PP_TRUE ||
        desc.stride != desc.size.width * 4 ||
#if !defined(TIZEN_PEPPER_EXTENSIONS)
        desc.format != PP_IMAGEDATAFORMAT_RGBA_PREMUL) {
#else
        desc.format != PP_IMAGEDATAFORMAT_BGRA_PREMUL) {
#endif
      LOG(ERROR) << "Wrong image data format";
      return PP_ERROR_BADARGUMENT;
    }

    // Make sure the source rect is not beyond the dimensions of the
    // image.
    if (!CheckPPFloatRect(new_layer->image->source_rect,
                          desc.size.width, desc.size.height)) {
      LOG(ERROR) << "Source rect is beyond the dimensions of the image";
      return PP_ERROR_BADARGUMENT;
    }

    base::SharedMemory* shm;
    uint32_t byte_count;
    if (enter.object()->GetSharedMemory(&shm, &byte_count) != PP_OK) {
      LOG(ERROR) << "Cannot get shared memory";
      return PP_ERROR_FAILED;
    }

    base::SharedMemoryHandle shm_handle =
        base::SharedMemory::DuplicateHandle(shm->handle());
    if (!base::SharedMemory::IsHandleValid(shm_handle)) {
      LOG(ERROR) << "Invalid handle for shared memory";
      return PP_ERROR_FAILED;
    }

    image_shm->reset(new base::SharedMemory(shm_handle, true));
    if (!(*image_shm)->Map(desc.stride * desc.size.height)) {
      image_shm->reset();
      LOG(ERROR) << "Insufficient memory to maped";
      return PP_ERROR_NOMEMORY;
    }
    return PP_OK;
  }
  LOG(ERROR) << "Unknown new layer type";
  return PP_ERROR_BADARGUMENT;
}

}  // namespace

PepperCompositorHost::LayerData::LayerData(
    const scoped_refptr<cc::Layer>& cc,
    const ppapi::CompositorLayerData& pp) : cc_layer(cc), pp_layer(pp) {}

PepperCompositorHost::LayerData::LayerData(const LayerData& other) = default;

PepperCompositorHost::LayerData::~LayerData() {}

PepperCompositorHost::PepperCompositorHost(
    RendererPpapiHost* host,
    PP_Instance instance,
    PP_Resource resource)
    : ResourceHost(host->GetPpapiHost(), instance, resource),
      bound_instance_(NULL),
#if defined(TIZEN_PEPPER_EXTENSIONS)
      renderer_ppapi_host_(host),
      compositor_task_runner_(base::MessageLoop::current()->task_runner()),
      video_frame_compositor_(NULL),
#endif
      weak_factory_(this) {
  layer_ = cc::Layer::Create();
  // TODO(penghuang): SetMasksToBounds() can be expensive if the layer is
  // transformed. Possibly better could be to explicitly clip the child layers
  // (by modifying their bounds).
  layer_->SetMasksToBounds(true);
  layer_->SetIsDrawable(true);
}

PepperCompositorHost::~PepperCompositorHost() {
  // Unbind from the instance when destroyed if we're still bound.
  if (bound_instance_)
    bound_instance_->BindGraphics(bound_instance_->pp_instance(), 0);
#if defined(TIZEN_PEPPER_EXTENSIONS)
  if (video_frame_compositor_) {
    video_frame_compositor_->SetVideoFrameProviderClient(NULL);
    if (compositor_task_runner_)
      compositor_task_runner_->DeleteSoon(FROM_HERE, video_frame_compositor_);
  }
#endif
}

bool PepperCompositorHost::BindToInstance(
    PepperPluginInstanceImpl* new_instance) {
  if (new_instance && new_instance->pp_instance() != pp_instance())
    return false;  // Can't bind other instance's contexts.
  if (bound_instance_ == new_instance)
    return true;  // Rebinding the same device, nothing to do.
  if (bound_instance_ && new_instance)
    return false;  // Can't change a bound device.
  bound_instance_ = new_instance;
  if (!bound_instance_)
    SendCommitLayersReplyIfNecessary();

  return true;
}

void PepperCompositorHost::ViewInitiatedPaint() {
  SendCommitLayersReplyIfNecessary();
}

void PepperCompositorHost::ImageReleased(
    int32_t id,
    std::unique_ptr<base::SharedMemory> shared_memory,
    std::unique_ptr<viz::SharedBitmap> bitmap,
    const gpu::SyncToken& sync_token,
    bool is_lost) {
  bitmap.reset();
  shared_memory.reset();
  ResourceReleased(id, sync_token, is_lost);
}

void PepperCompositorHost::ResourceReleased(int32_t id,
                                            const gpu::SyncToken& sync_token,
                                            bool is_lost) {
  host()->SendUnsolicitedReply(
      pp_resource(),
      PpapiPluginMsg_Compositor_ReleaseResource(id, sync_token, is_lost));
}

void PepperCompositorHost::SendCommitLayersReplyIfNecessary() {
  if (!commit_layers_reply_context_.is_valid())
    return;
  host()->SendReply(commit_layers_reply_context_,
                    PpapiPluginMsg_Compositor_CommitLayersReply());
  commit_layers_reply_context_ = ppapi::host::ReplyMessageContext();
}

void PepperCompositorHost::UpdateLayer(
    const scoped_refptr<cc::Layer>& layer,
    const ppapi::CompositorLayerData* old_layer,
    const ppapi::CompositorLayerData* new_layer,
    std::unique_ptr<base::SharedMemory> image_shm) {
  // Always update properties on cc::Layer, because cc::Layer
  // will ignore any setting with unchanged value.
  gfx::SizeF size(PP_ToGfxSize(new_layer->common.size));
  gfx::RectF clip_rect(PP_ToGfxRect(new_layer->common.clip_rect));

  // Pepper API uses DIP, so we must scale the layer's coordinates to
  // viewport in use-zoom-for-dsf.
  float dip_to_viewport_scale = 1 / viewport_to_dip_scale_;
  size.Scale(dip_to_viewport_scale);
  clip_rect.Scale(dip_to_viewport_scale);

  layer->SetIsDrawable(true);
  layer->SetBlendMode(SkBlendMode::kSrcOver);
  layer->SetOpacity(new_layer->common.opacity);

  layer->SetBounds(gfx::ToRoundedSize(size));
  layer->SetTransformOrigin(
      gfx::Point3F(size.width() / 2, size.height() / 2, 0.0f));
  gfx::Transform transform(gfx::Transform::kSkipInitialization);
  transform.matrix().setColMajorf(new_layer->common.transform.matrix);
  layer->SetTransform(transform);

  // Consider a (0,0,0,0) rect as no clip rect.
  if (new_layer->common.clip_rect.point.x != 0 ||
      new_layer->common.clip_rect.point.y != 0 ||
      new_layer->common.clip_rect.size.width != 0 ||
      new_layer->common.clip_rect.size.height != 0) {
    scoped_refptr<cc::Layer> clip_parent = layer->parent();
    if (clip_parent.get() == layer_.get()) {
      // Create a clip parent layer, if it does not exist.
      clip_parent = cc::Layer::Create();
      clip_parent->SetMasksToBounds(true);
      clip_parent->SetIsDrawable(true);
      layer_->ReplaceChild(layer.get(), clip_parent);
      clip_parent->AddChild(layer);
    }
    auto position = clip_rect.origin();
    clip_parent->SetPosition(position);
    clip_parent->SetBounds(gfx::ToRoundedSize(clip_rect.size()));
    layer->SetPosition(gfx::PointF(-position.x(), -position.y()));
  } else if (layer->parent() != layer_.get()) {
    // Remove the clip parent layer.
    layer_->ReplaceChild(layer->parent(), layer);
    layer->SetPosition(gfx::PointF());
  }

#if defined(TIZEN_PEPPER_EXTENSIONS)
  if (new_layer->background_plane) {
    if (!old_layer) {
      UpdateHole();
    }
    return;
  }
#endif

  if (new_layer->color) {
    layer->SetBackgroundColor(SkColorSetARGBMacro(
        new_layer->color->alpha * 255,
        new_layer->color->red * 255,
        new_layer->color->green * 255,
        new_layer->color->blue * 255));
    return;
  }

  if (new_layer->texture) {
    scoped_refptr<cc::TextureLayer> texture_layer(
        static_cast<cc::TextureLayer*>(layer.get()));
    if (!old_layer ||
        new_layer->common.resource_id != old_layer->common.resource_id) {
      viz::TextureMailbox mailbox(new_layer->texture->mailbox,
                                  new_layer->texture->sync_token,
                                  new_layer->texture->target);
      texture_layer->SetTextureMailbox(
          mailbox,
          viz::SingleReleaseCallback::Create(base::Bind(
              &PepperCompositorHost::ResourceReleased,
              weak_factory_.GetWeakPtr(), new_layer->common.resource_id)));
      // TODO(penghuang): get a damage region from the application and
      // pass it to SetNeedsDisplayRect().
      texture_layer->SetNeedsDisplay();
    }
    texture_layer->SetPremultipliedAlpha(new_layer->texture->premult_alpha);
    gfx::RectF rect = PP_ToGfxRectF(new_layer->texture->source_rect);
    texture_layer->SetUV(rect.origin(), rect.bottom_right());
    return;
  }

  if (new_layer->image) {
    if (!old_layer ||
        new_layer->common.resource_id != old_layer->common.resource_id) {
      scoped_refptr<cc::TextureLayer> image_layer(
          static_cast<cc::TextureLayer*>(layer.get()));
      EnterResourceNoLock<PPB_ImageData_API> enter(new_layer->image->resource,
                                                   true);
      DCHECK(enter.succeeded());

      // TODO(penghuang): support all kinds of image.
      PP_ImageDataDesc desc;
      PP_Bool rv = enter.object()->Describe(&desc);
      DCHECK_EQ(rv, PP_TRUE);
      DCHECK_EQ(desc.stride, desc.size.width * 4);
#if !defined(TIZEN_PEPPER_EXTENSIONS)
      DCHECK_EQ(desc.format, PP_IMAGEDATAFORMAT_RGBA_PREMUL);
#else
      DCHECK_EQ(desc.format, PP_IMAGEDATAFORMAT_BGRA_PREMUL);
#endif
      std::unique_ptr<viz::SharedBitmap> bitmap =
          RenderThreadImpl::current()
              ->shared_bitmap_manager()
              ->GetBitmapForSharedMemory(image_shm.get());

#if defined(TIZEN_PEPPER_EXTENSIONS)
      // OpenGL coordinates starts in bottom-left corner of window,
      // so Y value needs to be reversed, but for images we don't need
      // to flip because images coordinates starts in top-left corner.
      image_layer->SetFlipped(false);
#endif

      viz::TextureMailbox mailbox(bitmap.get(), PP_ToGfxSize(desc.size));
      image_layer->SetTextureMailbox(
          mailbox,
          viz::SingleReleaseCallback::Create(base::Bind(
              &PepperCompositorHost::ImageReleased, weak_factory_.GetWeakPtr(),
              new_layer->common.resource_id, base::Passed(&image_shm),
              base::Passed(&bitmap))));
      // TODO(penghuang): get a damage region from the application and
      // pass it to SetNeedsDisplayRect().
      image_layer->SetNeedsDisplay();

      // ImageData is always premultiplied alpha.
      image_layer->SetPremultipliedAlpha(true);
    }
    return;
  }
  // Should not be reached.
  NOTREACHED();
}

int32_t PepperCompositorHost::OnResourceMessageReceived(
    const IPC::Message& msg,
    HostMessageContext* context) {
  PPAPI_BEGIN_MESSAGE_MAP(PepperCompositorHost, msg)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL(
      PpapiHostMsg_Compositor_CommitLayers, OnHostMsgCommitLayers)
  PPAPI_END_MESSAGE_MAP()
  return ppapi::host::ResourceHost::OnResourceMessageReceived(msg, context);
}

bool PepperCompositorHost::IsCompositorHost() {
  return true;
}

int32_t PepperCompositorHost::OnHostMsgCommitLayers(
    HostMessageContext* context,
    const std::vector<ppapi::CompositorLayerData>& layers,
    bool reset) {
  if (commit_layers_reply_context_.is_valid()) {
    LOG(ERROR) << "Operation already in progress";
    return PP_ERROR_INPROGRESS;
  }

  std::unique_ptr<std::unique_ptr<base::SharedMemory>[]> image_shms;
  if (layers.size() > 0) {
    image_shms.reset(new std::unique_ptr<base::SharedMemory>[layers.size()]);
    if (!image_shms) {
      LOG(ERROR) << "Image shms is NULL";
      return PP_ERROR_NOMEMORY;
    }
    // Verfiy the layers first, if an error happens, we will return the error to
    // plugin and keep current layers set by the previous CommitLayers()
    // unchanged.
    for (size_t i = 0; i < layers.size(); ++i) {
      const ppapi::CompositorLayerData* old_layer = NULL;
      if (!reset && i < layers_.size())
        old_layer = &layers_[i].pp_layer;
      int32_t rv = VerifyCommittedLayer(old_layer, &layers[i], &image_shms[i]);
      if (rv != PP_OK)
        return rv;
    }
  }

  // ResetLayers() has been called, we need rebuild layer stack.
  if (reset) {
    layer_->RemoveAllChildren();
    layers_.clear();
  }

  for (size_t i = 0; i < layers.size(); ++i) {
    const ppapi::CompositorLayerData* pp_layer = &layers[i];
    LayerData* data = i >= layers_.size() ? NULL : &layers_[i];
    DCHECK(!data || data->cc_layer.get());
    scoped_refptr<cc::Layer> cc_layer = data ? data->cc_layer : NULL;
    ppapi::CompositorLayerData* old_layer = data ? &data->pp_layer : NULL;

    if (!cc_layer.get()) {
      if (pp_layer->color) {
        cc_layer = cc::SolidColorLayer::Create();
      } else if (pp_layer->texture || pp_layer->image) {
        cc_layer = cc::TextureLayer::CreateForMailbox(NULL);
#if defined(TIZEN_PEPPER_EXTENSIONS)
      } else if (pp_layer->background_plane) {
        video_frame_compositor_ = new media::VideoFrameCompositor(
#if defined(TIZEN_VIDEO_HOLE)
            media::BindToCurrentLoop(
                base::Bind(&PepperCompositorHost::OnDrawableContentRectChanged,
                           weak_factory_.GetWeakPtr())),
#endif
        compositor_task_runner_,
        base::BindRepeating(
          [](base::OnceCallback<void(viz::ContextProvider*)> callback){}));

        cc_layer = cc::VideoLayer::Create(video_frame_compositor_,
            media::VIDEO_ROTATION_0);
#endif  // TIZEN_PEPPER_EXTENSIONS
      }
      layer_->AddChild(cc_layer);
    }

    UpdateLayer(cc_layer, old_layer, pp_layer, std::move(image_shms[i]));

    if (old_layer)
      *old_layer = *pp_layer;
    else
      layers_.push_back(LayerData(cc_layer, *pp_layer));
  }

  // We need to force a commit for each CommitLayers() call, even if no layers
  // changed since the last call to CommitLayers(). This is so
  // WiewInitiatedPaint() will always be called.
  if (layer_->layer_tree_host())
    layer_->layer_tree_host()->SetNeedsCommit();

  // If the host is not bound to the instance, return PP_OK immediately.
  if (!bound_instance_)
    return PP_OK;

  commit_layers_reply_context_ = context->MakeReplyMessageContext();
  return PP_OK_COMPLETIONPENDING;
}

#if defined(TIZEN_PEPPER_EXTENSIONS)
void PepperCompositorHost::UpdateHole() {
#if defined(TIZEN_VIDEO_HOLE)
  if (bound_instance_ && video_frame_compositor_ && compositor_task_runner_) {
    auto view_data = bound_instance_->view_data();
    gfx::Size size(view_data.rect.size.width, view_data.rect.size.height);
    auto video_frame = media::VideoFrame::CreateHoleFrame(size);
    compositor_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(
            &media::VideoFrameCompositor::PaintSingleFrame,
            base::Unretained(video_frame_compositor_), video_frame, false));
  }
#endif  // TIZEN_VIDEO_HOLE
}

void PepperCompositorHost::OnDrawableContentRectChanged(gfx::Rect rect, bool) {
  UpdateHole();
}

void PepperCompositorHost::OnOpacityChanged(bool opaque) {
  UpdateHole();
}
#endif
}  // namespace content
