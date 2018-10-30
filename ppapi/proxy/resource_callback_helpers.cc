// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/resource_callback_helpers.h"

namespace ppapi {
namespace proxy {

void Run(scoped_refptr<TrackedCallback> callback, int32_t result) {
  if (TrackedCallback::IsPending(callback))
    callback->Run(result);
}

void PostAbortIfNecessary(const scoped_refptr<TrackedCallback>& callback) {
  if (TrackedCallback::IsPending(callback))
    callback->PostAbort();
}

void OnReply(scoped_refptr<TrackedCallback> callback,
             const ResourceMessageReplyParams& params) {
  Run(callback, params.result());
}

}  // namespace proxy
}  // namespace ppapi

// FIXME consider merging/resuing CallbackMessageHelpers.h
