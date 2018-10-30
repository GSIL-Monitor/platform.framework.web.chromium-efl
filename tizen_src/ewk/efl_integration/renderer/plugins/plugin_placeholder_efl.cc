// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/plugins/plugin_placeholder_efl.h"

#include <libintl.h>

#include "gin/handle.h"
#include "third_party/WebKit/public/web/WebKit.h"

gin::WrapperInfo PluginPlaceholderEfl::kWrapperInfo = {gin::kEmbedderNativeGin};

PluginPlaceholderEfl::PluginPlaceholderEfl(content::RenderFrame* render_frame,
                                           blink::WebLocalFrame* frame,
                                           const blink::WebPluginParams& params,
                                           const std::string& html_data)
    : plugins::LoadablePluginPlaceholder(render_frame,
                                         frame,
                                         params,
                                         html_data) {}

PluginPlaceholderEfl::~PluginPlaceholderEfl() {
}

// static
PluginPlaceholderEfl* PluginPlaceholderEfl::CreateMissingPlugin(
    content::RenderFrame* render_frame,
    blink::WebLocalFrame* frame,
    const blink::WebPluginParams& params) {
  // |missing_plugin| will destroy itself when its WebViewPlugin is going away.
  PluginPlaceholderEfl* missing_plugin = new PluginPlaceholderEfl(
      render_frame, frame, params,
      dgettext("WebKit", "IDS_WEBVIEW_BODY_PLUG_IN_MISSING"));
  missing_plugin->AllowLoading();
  return missing_plugin;
}

void PluginPlaceholderEfl::OnBlockedContent(
    content::RenderFrame::PeripheralContentStatus status,
    bool is_same_origin) {
  NOTIMPLEMENTED();
}

blink::WebPlugin* PluginPlaceholderEfl::CreatePlugin() {
  NOTIMPLEMENTED();
  return nullptr;
}

v8::Local<v8::Value> PluginPlaceholderEfl::GetV8Handle(v8::Isolate* isolate) {
  return gin::CreateHandle(isolate, this).ToV8();
}
