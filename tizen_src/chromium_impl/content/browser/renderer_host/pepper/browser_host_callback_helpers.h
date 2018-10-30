// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_BROWSER_HOST_CALLBACK_HELPERS_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_BROWSER_HOST_CALLBACK_HELPERS_H_

#include "components/nacl/browser/nacl_process_host.h"
#include "components/nacl/common/nacl_process_type.h"
#include "content/browser/ppapi_plugin_process_host.h"
#include "content/public/browser/browser_ppapi_host.h"
#include "ppapi/host/host_message_context.h"
#include "ppapi/host/ppapi_host.h"

namespace content {

namespace internal {

inline BrowserPpapiHost* GetBrowserHostForInstance(PP_Instance instance) {
  // check untrusted plugins
  for (BrowserChildProcessHostTypeIterator<nacl::NaClProcessHost> iter(
           PROCESS_TYPE_NACL_LOADER);
       !iter.Done(); ++iter)
    if (iter->browser_ppapi_host()->IsValidInstance(instance))
      return iter->browser_ppapi_host();
  // check trusted plugins
  for (PpapiPluginProcessHostIterator iter; !iter.Done(); ++iter)
    if (iter->host_impl()->IsValidInstance(instance))
      return iter->host_impl();
  return nullptr;
}

template <typename MsgT, typename... Args>
void SendReplyWithParams(PP_Instance instance,
                         ppapi::host::ReplyMessageContext context,
                         int32_t result,
                         const Args&... output) {
  BrowserPpapiHost* host = GetBrowserHostForInstance(instance);
  if (!host || !host->GetPpapiHost()) {
    LOG(WARNING) << "PpapiHost for a given PP_Instance not found!";
    return;
  }

  context.params.set_result(result);
  host->GetPpapiHost()->SendReply(context, MsgT(output...));
}

}  // namespace internal

template <typename MsgT>
void ReplyCallback(PP_Instance instance,
                   ppapi::host::ReplyMessageContext context,
                   int32_t result) {
  internal::SendReplyWithParams<MsgT>(instance, context, result);
}

template <typename MsgT, typename... Args>
void ReplyCallbackWithValueOutput(PP_Instance instance,
                                  ppapi::host::ReplyMessageContext context,
                                  int32_t result,
                                  Args... output) {
  internal::SendReplyWithParams<MsgT>(instance, context, result, output...);
}

template <typename MsgT, typename... Args>
void ReplyCallbackWithRefOutput(PP_Instance instance,
                                ppapi::host::ReplyMessageContext context,
                                int32_t result,
                                const Args&... output) {
  internal::SendReplyWithParams<MsgT>(instance, context, result, output...);
}

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_BROWSER_HOST_CALLBACK_HELPERS_H_
