// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_EFL_INTEGRATION_WRT_HBBTV_WIDGET_H_
#define EWK_EFL_INTEGRATION_WRT_HBBTV_WIDGET_H_

#include <string>

#include "content/public/renderer/render_thread_observer.h"
#include "url/gurl.h"
#include "wrt/v8widget.h"

class HbbtvRenderThreadObserver;

// Have to be created on the RenderThread.
class HbbtvWidget : public V8Widget {
 public:
  HbbtvWidget(const base::CommandLine& command_line);
  ~HbbtvWidget() override;

  content::RenderThreadObserver* GetObserver() override;
  void RegisterURLSchemesAsCORSEnabled(std::string);
  void RegisterJSPluginMimeTypes(std::string);
  void SetTimeOffset(double time_offset);

 private:
  double time_offset_ = 0.0;
  std::string schemes_;
  std::unique_ptr<HbbtvRenderThreadObserver> observer_;
};

#endif  // EWK_EFL_INTEGRATION_WRT_HBBTV_WIDGET_H_
