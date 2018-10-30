// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_FRAME_PRIVATE_H
#define EWK_FRAME_PRIVATE_H

namespace content {
class RenderFrameHost;
}

struct NavigationPolicyParams;

class _Ewk_Frame {
public:
  _Ewk_Frame(content::RenderFrameHost* rfh);
  _Ewk_Frame(const NavigationPolicyParams &params);

  bool IsMainFrame() const { return is_main_frame_; }

private:
  bool is_main_frame_;
};


#endif // EWK_FRAME_PRIVATE_H
