// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/media_player/tizen/elementary_stream_listener_private_wrapper.h"

#include "base/location.h"
#include "base/message_loop/message_loop.h"

namespace content {

void ElementaryStreamListenerPrivateWrapper::SetListener(
    base::WeakPtr<ElementaryStreamListenerPrivate> listener) {
  base::AutoLock guard(listener_lock_);
  listener_ = listener;
}

void ElementaryStreamListenerPrivateWrapper::RemoveListener(
    ElementaryStreamListenerPrivate* listener) {
  base::AutoLock guard(listener_lock_);
  if (listener_.get() != listener)
    return;

  listener_ = nullptr;
}

void ElementaryStreamListenerPrivateWrapper::ClearListener() {
  base::AutoLock guard(listener_lock_);
  listener_ = nullptr;
}

ElementaryStreamListenerPrivateWrapper::ElementaryStreamListenerPrivateWrapper()
    : listener_(nullptr) {}

// NOLINTNEXTLINE(runtime/int)
void ElementaryStreamListenerPrivateWrapper::OnNeedData(unsigned bytes_max) {
  DispatchLocked(FROM_HERE, &ElementaryStreamListenerPrivate::OnNeedData,
                 bytes_max);
}

void ElementaryStreamListenerPrivateWrapper::OnEnoughData() {
  DispatchLocked(FROM_HERE, &ElementaryStreamListenerPrivate::OnEnoughData);
}

void ElementaryStreamListenerPrivateWrapper::OnSeekData(PP_TimeTicks offset) {
  DispatchLocked(FROM_HERE, &ElementaryStreamListenerPrivate::OnSeekData,
                 offset);
}

}  // namespace content
