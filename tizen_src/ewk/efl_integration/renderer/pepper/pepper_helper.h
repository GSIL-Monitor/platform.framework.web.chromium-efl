// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RENDERER_PEPPER_PEPPER_HELPER_H_
#define RENDERER_PEPPER_PEPPER_HELPER_H_

#include "base/compiler_specific.h"
#include "content/public/renderer/render_frame_observer.h"

// Copied from chrome/renderer/pepper with minor modifications.
// This class listens for Pepper creation events from the RenderFrame and
// attaches the parts required for EFL-specific plugin support.
class PepperHelper : public content::RenderFrameObserver {
 public:
  explicit PepperHelper(content::RenderFrame* render_frame);
  ~PepperHelper() override;

  // RenderFrameObserver.
  void DidCreatePepperPlugin(content::RendererPpapiHost* host) override;

  void OnDestruct() {};

 private:
  DISALLOW_COPY_AND_ASSIGN(PepperHelper);
};

#endif  // RENDERER_PEPPER_PEPPER_HELPER_H_
