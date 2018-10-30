// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/teec_context_resource.h"

#include <memory>
#include <utility>

#include "base/logging.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/proxy/resource_callback_helpers.h"
#include "ppapi/shared_impl/ppapi_globals.h"
#include "ppapi/shared_impl/resource_tracker.h"

namespace ppapi {
namespace proxy {

TEECContextResource::TEECContextResource(Connection connection,
                                         PP_Instance instance)
    : PluginResource(connection, instance), context_opened_(false) {
  SendCreate(BROWSER, PpapiHostMsg_TEECContext_Create());
}

TEECContextResource::~TEECContextResource() {
  PostAbortIfNecessary(open_callback_);
}

thunk::PPB_TEECContext_API* TEECContextResource::AsPPB_TEECContext_API() {
  return this;
}

int32_t TEECContextResource::Open(const std::string& name,
                                  PP_TEEC_Result* result,
                                  scoped_refptr<TrackedCallback> callback) {
  if (TrackedCallback::IsPending(open_callback_)) {
    LOG(ERROR) << "OpenContext is already in progress";
    return PP_ERROR_INPROGRESS;
  }

  if (context_opened_) {
    LOG(ERROR) << "Context has been already opened";
    return PP_ERROR_BADARGUMENT;
  }

  open_callback_ = callback;
  Call<PpapiPluginMsg_TEECContext_OpenReply>(
      BROWSER, PpapiHostMsg_TEECContext_Open(name),
      base::Bind(&TEECContextResource::OnOpenReply, base::Unretained(this),
                 open_callback_, result));
  return PP_OK_COMPLETIONPENDING;
}

void TEECContextResource::OnOpenReply(scoped_refptr<TrackedCallback> callback,
                                      PP_TEEC_Result* out,
                                      const ResourceMessageReplyParams& params,
                                      const PP_TEEC_Result& val) {
  open_callback_ = nullptr;

  if (!TrackedCallback::IsPending(callback))
    return;

  if (params.result() == PP_OK) {
    context_opened_ = true;
    if (out)
      *out = val;
  }

  Run(callback, params.result());
}

}  // namespace proxy
}  // namespace ppapi
