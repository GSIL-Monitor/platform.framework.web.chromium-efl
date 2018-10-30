// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_RESOURCE_CALLBACK_HELPERS_H_
#define PPAPI_PROXY_RESOURCE_CALLBACK_HELPERS_H_

#include <vector>
#include "ppapi/c/pp_errors.h"
#include "ppapi/proxy/resource_message_params.h"
#include "ppapi/proxy/serialized_var.h"
#include "ppapi/shared_impl/array_writer.h"
#include "ppapi/shared_impl/tracked_callback.h"

namespace ppapi {
namespace proxy {

void Run(scoped_refptr<TrackedCallback>, int32_t result);

// Needs to pass callback as cref to RefPtr due to fact
// that TrackedCallback::isPending accepts only cref to RefPtr.
void PostAbortIfNecessary(const scoped_refptr<TrackedCallback>&);

void OnReply(scoped_refptr<TrackedCallback>, const ResourceMessageReplyParams&);

template <typename T>
void OnReplyWithOutput(scoped_refptr<TrackedCallback> callback,
                       T* out,
                       const ResourceMessageReplyParams& params,
                       const T& val) {
  if (!TrackedCallback::IsPending(callback))
    return;

  DCHECK(out);
  if (out && params.result() == PP_OK)
    *out = val;

  Run(callback, params.result());
}

void OnReplyWithPPVarOutput(scoped_refptr<TrackedCallback>,
                            PP_Var* out,
                            PP_Instance,
                            const ResourceMessageReplyParams&,
                            const ppapi::proxy::SerializedVar& val);

template <typename T>
void OnReplyWithArrayOutput(scoped_refptr<TrackedCallback> callback,
                            PP_ArrayOutput output,
                            const ResourceMessageReplyParams& params,
                            const std::vector<T>& data) {
  if (!TrackedCallback::IsPending(callback))
    return;

  int32_t result = params.result();
  if (result == PP_OK) {
    ArrayWriter writer(output);
    if (writer.is_null())
      result = PP_ERROR_BADARGUMENT;
    else if (!writer.StoreVector(data))
      result = PP_ERROR_NOMEMORY;
  }

  Run(callback, result);
}

}  // namespace proxy
}  // namespace ppapi

#endif  // PPAPI_PROXY_RESOURCE_CALLBACK_HELPERS_H_
