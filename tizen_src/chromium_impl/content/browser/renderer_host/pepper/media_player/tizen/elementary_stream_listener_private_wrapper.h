// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_ELEMENTARY_STREAM_LISTENER_PRIVATE_WRAPPER_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_ELEMENTARY_STREAM_LISTENER_PRIVATE_WRAPPER_H_

#include "base/bind.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/lock.h"
#include "content/browser/renderer_host/pepper/media_player/elementary_stream_listener_private.h"
#include "content/public/browser/browser_thread.h"
#include "ppapi/c/pp_time.h"

namespace base {
class Location;
}  // namespace base

namespace content {

// Helper class which dispatches all data related events to the main thread.
class ElementaryStreamListenerPrivateWrapper {
 public:
  ElementaryStreamListenerPrivateWrapper();

  void SetListener(base::WeakPtr<ElementaryStreamListenerPrivate> listener);
  void RemoveListener(ElementaryStreamListenerPrivate* listener);

  // Called on player deletion and clean-up
  void ClearListener();

  // Below 3 functions are called on player thread
  // and deals with values passed by Platform Player API

  // NOLINTNEXTLINE(runtime/int)
  void OnNeedData(unsigned bytes_max);
  void OnEnoughData();
  void OnSeekData(PP_TimeTicks offset);

 private:
  template <typename Method, typename... Args>
  void DispatchLocked(const base::Location& from_here,
                      const Method& method,
                      const Args&... args);

  base::WeakPtr<ElementaryStreamListenerPrivate> listener_;
  base::Lock listener_lock_;

  DISALLOW_COPY_AND_ASSIGN(ElementaryStreamListenerPrivateWrapper);
};

template <typename Method, typename... Args>
void ElementaryStreamListenerPrivateWrapper::DispatchLocked(
    const base::Location& from_here,
    const Method& method,
    const Args&... args) {
  base::AutoLock guard(listener_lock_);
  BrowserThread::PostTask(BrowserThread::IO, from_here,
                          base::Bind(method, listener_, args...));
}

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_ELEMENTARY_STREAM_LISTENER_PRIVATE_WRAPPER_H_
