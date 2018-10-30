// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLUGIN_PLACEHOLDER_EFL_H_
#define PLUGIN_PLACEHOLDER_EFL_H_

#include "components/plugins/renderer/loadable_plugin_placeholder.h"

class PluginPlaceholderEfl : public plugins::LoadablePluginPlaceholder,
                             public gin::Wrappable<PluginPlaceholderEfl> {
 public:
  static gin::WrapperInfo kWrapperInfo;

  // Creates a new WebViewPlugin with a MissingPlugin as a delegate.
  static PluginPlaceholderEfl* CreateMissingPlugin(
      content::RenderFrame* render_frame,
      blink::WebLocalFrame* frame,
      const blink::WebPluginParams& params);
  void OnBlockedContent(content::RenderFrame::PeripheralContentStatus status,
                        bool is_same_origin) override;

 private:
  PluginPlaceholderEfl(content::RenderFrame* render_frame,
                       blink::WebLocalFrame* frame,
                       const blink::WebPluginParams& params,
                       const std::string& html_data);
  ~PluginPlaceholderEfl() override;

  // WebViewPlugin::Delegate (via PluginPlaceholder) method
  v8::Local<v8::Value> GetV8Handle(v8::Isolate*) override;
  blink::WebPlugin* CreatePlugin() override;

  DISALLOW_COPY_AND_ASSIGN(PluginPlaceholderEfl);
};

#endif  // PLUGIN_PLACEHOLDER_EFL_H_
