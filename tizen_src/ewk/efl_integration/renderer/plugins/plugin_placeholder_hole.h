// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_EFL_INTEGRATION_RENDERER_PLUGINS_PLUGIN_PLACEHOLDER_HOLE_H_
#define EWK_EFL_INTEGRATION_RENDERER_PLUGINS_PLUGIN_PLACEHOLDER_HOLE_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "components/plugins/renderer/plugin_placeholder.h"

namespace cc_blink {
class WebLayerImpl;
}

class PluginPlaceholderHole : public plugins::PluginPlaceholderBase,
                              public gin::Wrappable<PluginPlaceholderHole> {
 public:
  static gin::WrapperInfo kWrapperInfo;

  static bool SupportsMimeType(blink::WebLocalFrame* frame,
                               const std::string& mimetype);

  static PluginPlaceholderHole* Create(content::RenderFrame* render_frame,
                                       blink::WebLocalFrame* frame,
                                       const blink::WebPluginParams& params);

 private:
  static void SetTransparentV8Impl(
      const v8::FunctionCallbackInfo<v8::Value>& args);

  PluginPlaceholderHole(content::RenderFrame* render_frame,
                        blink::WebLocalFrame* frame,
                        const blink::WebPluginParams& params);
  ~PluginPlaceholderHole() override;

  // WebViewPlugin::Delegate method.
  void PluginDestroyed() override;
  v8::Local<v8::Value> GetV8Handle(v8::Isolate*) override;
  v8::Local<v8::Object> GetV8ScriptableObject(
      v8::Isolate* isolate) const override;
#if defined(OS_TIZEN_TV_PRODUCT)
  bool PluginIsAvplayer() { return false; }
#endif
  void OnUnobscuredRectUpdate(const gfx::Rect& unobscured_rect) override;

  // From v8 to c++.
  void SetTransparentHelper(bool transparent);

  // From c++ to v8.
  void NotifyGeometryChanged();
  void NotitfyAttributes() const;

  scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;
  mutable v8::Persistent<v8::Object> v8_object_;
  std::unique_ptr<cc_blink::WebLayerImpl> web_layer_;
  gfx::Rect local_rect_;
  gfx::Point local_position_in_root_frame_;

  // Same life time.
  blink::WebLocalFrame* main_frame_;

  base::WeakPtrFactory<PluginPlaceholderHole> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PluginPlaceholderHole);
};

#endif  // EWK_EFL_INTEGRATION_RENDERER_PLUGINS_PLUGIN_PLACEHOLDER_HOLE_H_
