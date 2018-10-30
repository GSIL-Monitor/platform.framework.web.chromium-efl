// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_EFL_INTEGRATION_RENDERER_PLUGINS_PLUGIN_PLACEHOLDER_AVPLAYER_H_
#define EWK_EFL_INTEGRATION_RENDERER_PLUGINS_PLUGIN_PLACEHOLDER_AVPLAYER_H_

#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/plugins/renderer/plugin_placeholder.h"
#include "ui/gfx/geometry/rect.h"

namespace cc_blink {
class WebLayerImpl;
}

class PluginPlaceholderAvplayer
    : public plugins::PluginPlaceholderBase,
      public gin::Wrappable<PluginPlaceholderAvplayer> {
 public:
  static gin::WrapperInfo kWrapperInfo;

  static bool SupportsMimeType(const std::string& mimetype);

  static PluginPlaceholderAvplayer* Create(
      content::RenderFrame* render_frame,
      blink::WebLocalFrame* frame,
      const blink::WebPluginParams& params);

 private:
  PluginPlaceholderAvplayer(content::RenderFrame* render_frame,
                            blink::WebLocalFrame* frame,
                            const blink::WebPluginParams& params);
  ~PluginPlaceholderAvplayer() override;

  // WebViewPlugin::Delegate method.
  v8::Local<v8::Value> GetV8Handle(v8::Isolate*) override;
#if defined(OS_TIZEN_TV_PRODUCT)
  bool PluginIsAvplayer() { return true; }
#endif
  void OnUnobscuredRectUpdate(const gfx::Rect& unobscured_rect) override;
  void NotifyTransparentChanged();

  std::unique_ptr<cc_blink::WebLayerImpl> web_layer_;
  gfx::Rect local_rect_;

  DISALLOW_COPY_AND_ASSIGN(PluginPlaceholderAvplayer);
};

#endif  // EWK_EFL_INTEGRATION_RENDERER_PLUGINS_PLUGIN_PLACEHOLDER_AVPLAYER_H_
