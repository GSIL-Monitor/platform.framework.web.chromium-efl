// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "hbbtv_widget.h"

#include "base/strings/string_number_conversions.h"
#include "command_line_efl.h"
#include "common/content_switches_efl.h"
#include "common/render_messages_ewk.h"
#include "content/public/renderer/render_thread.h"
#include "content/renderer/render_thread_impl.h"
#include "third_party/WebKit/common/mime_util/mime_util.h"
#include "third_party/WebKit/public/web/WebSecurityPolicy.h"
#include "v8/include/v8.h"
#include "wrt/hbbtv_dynamicplugin.h"

class HbbtvRenderThreadObserver : public content::RenderThreadObserver {
 public:
  explicit HbbtvRenderThreadObserver(HbbtvWidget* hbbtv_widget)
      : hbbtv_widget_(hbbtv_widget) {}

  bool OnControlMessageReceived(const IPC::Message& message) override {
    bool handled = true;
    IPC_BEGIN_MESSAGE_MAP(HbbtvRenderThreadObserver, message)
    IPC_MESSAGE_FORWARD(HbbtvMsg_RegisterURLSchemesAsCORSEnabled, hbbtv_widget_,
                        HbbtvWidget::RegisterURLSchemesAsCORSEnabled)
    IPC_MESSAGE_FORWARD(HbbtvMsg_RegisterJSPluginMimeTypes, hbbtv_widget_,
                        HbbtvWidget::RegisterJSPluginMimeTypes)
    IPC_MESSAGE_FORWARD(HbbtvMsg_SetTimeOffset, hbbtv_widget_,
                        HbbtvWidget::SetTimeOffset)
    IPC_MESSAGE_UNHANDLED(handled = false)
    IPC_END_MESSAGE_MAP()
    return handled;
  }

 private:
  HbbtvWidget* hbbtv_widget_;
};

HbbtvWidget::HbbtvWidget(const base::CommandLine& command_line)
    : V8Widget(V8Widget::Type::HBBTV) {
  DCHECK(content::RenderThread::Get())
      << "HbbtvWidget must be constructed on the render thread";

  id_ = command_line.GetSwitchValueASCII(switches::kTizenAppId);
  if (id_.empty())
    return;
  observer_.reset(new HbbtvRenderThreadObserver(this));

  std::string injected_bundle_path =
      command_line.GetSwitchValueASCII(switches::kInjectedBundlePath);
  CHECK(!injected_bundle_path.empty());
  HbbtvDynamicPlugin::Get().InitRenderer(injected_bundle_path);
  SetPlugin(&HbbtvDynamicPlugin::Get());

  if (command_line.HasSwitch(switches::kCORSEnabledURLSchemes)) {
    std::string cors_enabled_url_schemes =
        command_line.GetSwitchValueASCII(switches::kCORSEnabledURLSchemes);
    RegisterURLSchemesAsCORSEnabled(cors_enabled_url_schemes);
  }
  if (command_line.HasSwitch(switches::kTimeOffset)) {
    std::string time_offset =
        command_line.GetSwitchValueASCII(switches::kTimeOffset);
    double offset = 0.0;
    if (base::StringToDouble(time_offset, &offset))
      SetTimeOffset(offset);
  }
}

HbbtvWidget::~HbbtvWidget() {}

content::RenderThreadObserver* HbbtvWidget::GetObserver() {
  return observer_.get();
}

void HbbtvWidget::RegisterURLSchemesAsCORSEnabled(std::string schemes) {
  LOG(INFO) << "schemes = " << schemes;
  if (schemes_ == schemes)
    return;

  schemes_ = schemes;
  std::stringstream stream(schemes);
  std::string scheme;
  while (std::getline(stream, scheme, ',')) {
    if(scheme == "hbbtv-package") {
      blink::WebSecurityPolicy::RegisterURLSchemeAsSecure(
          blink::WebString::FromUTF8(scheme));
    }
    blink::WebSecurityPolicy::RegisterURLSchemeAsCORSEnabled(
        blink::WebString::FromUTF8(scheme));
  }
}

void HbbtvWidget::RegisterJSPluginMimeTypes(std::string mime_types) {
  LOG(INFO) << "mime_types = " << mime_types;
  blink::RegisterJavascriptPluginMimeTypes(mime_types);
}

void HbbtvWidget::SetTimeOffset(double time_offset) {
  if (time_offset_ == time_offset)
    return;

  time_offset_ = time_offset;
  base::Time::SetTimeOffset(time_offset_);
  LOG(INFO) << "time_offset = " << time_offset;
}
