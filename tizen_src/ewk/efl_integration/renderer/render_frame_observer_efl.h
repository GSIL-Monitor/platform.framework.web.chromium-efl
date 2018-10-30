// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RENDER_FRAME_OBSERVER_EFL_H_
#define RENDER_FRAME_OBSERVER_EFL_H_

#include <vector>

#include "content/public/renderer/render_frame_observer.h"
#include "third_party/WebKit/public/platform/WebSize.h"

namespace blink {
class WebElement;
}

namespace content {
class RenderFrame;
class RenderFrameImpl;

class RenderFrameObserverEfl : public RenderFrameObserver {
 public:
  explicit RenderFrameObserverEfl(RenderFrame* render_frame);
  virtual ~RenderFrameObserverEfl();

  void OnDestruct() override;

  // IPC::Listener implementation.
  bool OnMessageReceived(const IPC::Message& message) override;

  void DidChangeScrollOffset() override;

  void WillSubmitForm(const blink::WebFormElement& form) override;

  void DidCreateScriptContext(v8::Local<v8::Context> context,
                              int world_id) override;
  void WillReleaseScriptContext(v8::Handle<v8::Context> context,
                                int world_id) override;

 private:
  void OnSelectPopupMenuItems(bool canceled,
                              const std::vector<int>& selected_indices);

  void OnLoadNotFoundErrorPage(std::string errorUrl);

  void OnMoveToNextOrPreviousSelectElement(bool direction);
  void OnRequestSelectCollectionInformation();

#if defined(OS_TIZEN_TV_PRODUCT)
  void DidFinishLoad() override;
  void WillSendSubmitEvent(const blink::WebFormElement& form) override;
#endif
  blink::WebSize max_scroll_offset_;
  blink::WebSize last_scroll_offset_;
};

} // namespace content

#endif  // RENDER_FRAME_OBSERVER_EFL_H_
