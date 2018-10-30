// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/plugins/plugin_placeholder_avplayer.h"

#include <algorithm>
#include <string>

#include "base/memory/ptr_util.h"
#include "gin/handle.h"
#include "renderer/plugins/hole_layer.h"
#include "third_party/WebKit/public/web/WebKit.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebPluginContainer.h"

gin::WrapperInfo PluginPlaceholderAvplayer::kWrapperInfo = {
    gin::kEmbedderNativeGin};

// static
bool PluginPlaceholderAvplayer::SupportsMimeType(const std::string& mimetype) {
  return (mimetype == "application/avplayer");
}

// static
PluginPlaceholderAvplayer* PluginPlaceholderAvplayer::Create(
    content::RenderFrame* render_frame,
    blink::WebLocalFrame* frame,
    const blink::WebPluginParams& params) {
  return new PluginPlaceholderAvplayer(render_frame, frame, params);
}

PluginPlaceholderAvplayer::PluginPlaceholderAvplayer(
    content::RenderFrame* render_frame,
    blink::WebLocalFrame* frame,
    const blink::WebPluginParams& params)
    : plugins::PluginPlaceholderBase(render_frame,
                                     frame,
                                     params,
                                     "" /* html_data */),
      web_layer_(nullptr),
      local_rect_(0, 0, 0, 0) {
  LOG(INFO) << "PluginPlaceholderAvplayer: create with type "
            << params.mime_type.Utf8();
}

PluginPlaceholderAvplayer::~PluginPlaceholderAvplayer() {}

void PluginPlaceholderAvplayer::OnUnobscuredRectUpdate(
    const gfx::Rect& unobscured_rect) {
  if (unobscured_rect.width() > 0 && unobscured_rect.height() > 0 &&
      local_rect_ != unobscured_rect) {
    local_rect_ = unobscured_rect;
    LOG(INFO) << "PluginPlaceholderAvplayer: hole rect changed "
              << local_rect_.ToString();
    NotifyTransparentChanged();
  }
}

void PluginPlaceholderAvplayer::NotifyTransparentChanged() {
  if (!web_layer_) {
    if (!plugin())
      return;
    blink::WebPluginContainer* container = plugin()->Container();
    if (!container)
      return;
    scoped_refptr<cc::HoleLayer> layer_ = new cc::HoleLayer;
    web_layer_ = base::WrapUnique(new cc_blink::WebLayerImpl(layer_));
    container->SetWebLayer(web_layer_.get());
  }

  // Transparent color + opaque means hole punch through.
  web_layer_->SetBackgroundColor(SK_ColorTRANSPARENT);
  web_layer_->SetOpaque(true);
  web_layer_->SetContentsOpaqueIsFixed(true);

  LOG(INFO) << "PluginPlaceholderAvplayer: Change Transparent succeeded";
}

v8::Local<v8::Value> PluginPlaceholderAvplayer::GetV8Handle(
    v8::Isolate* isolate) {
  return gin::CreateHandle(isolate, this).ToV8();
}
