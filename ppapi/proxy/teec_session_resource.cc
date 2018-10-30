// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/teec_session_resource.h"

#include <memory>
#include <string>
#include <utility>

#include "base/logging.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/proxy/resource_callback_helpers.h"
#include "ppapi/shared_impl/ppapi_globals.h"
#include "ppapi/shared_impl/resource_tracker.h"

namespace ppapi {
namespace proxy {

TEECSessionResource::TEECSessionResource(Connection connection,
                                         PP_Instance instance,
                                         PP_Resource context)
    : PluginResource(connection, instance),
      next_operation_id_(1),
      session_opened_(false) {
  null_operation_.operation_id = 0;
  SendCreate(BROWSER, PpapiHostMsg_TEECSession_Create(context));
}

TEECSessionResource::~TEECSessionResource() {
  for (auto it = pending_callbacks_.begin(); it != pending_callbacks_.end();
       ++it)
    PostAbortIfNecessary(*it);

  PostAbortIfNecessary(open_callback_);
}

thunk::PPB_TEECSession_API* TEECSessionResource::AsPPB_TEECSession_API() {
  return this;
}

int32_t TEECSessionResource::Open(const PP_TEEC_UUID* destination,
                                  uint32_t connection_method,
                                  uint32_t connection_data_size,
                                  const void* connection_data,
                                  PP_TEEC_Operation* operation,
                                  PP_TEEC_Result* result,
                                  scoped_refptr<TrackedCallback> callback) {
  if (TrackedCallback::IsPending(open_callback_)) {
    LOG(ERROR) << "Open is already in progress";
    return PP_ERROR_INPROGRESS;
  }

  if (session_opened_) {
    LOG(ERROR) << "Session has been already opened";
    return PP_ERROR_BADARGUMENT;
  }

  if (!destination) {
    LOG(ERROR) << "destination is null";
    return PP_ERROR_BADARGUMENT;
  }

  std::string connection_data_string;
  if (connection_data && connection_data_size)
    connection_data_string = std::string(
        static_cast<const char*>(connection_data), connection_data_size);

  PP_TEEC_Operation operation_value = operation ? *operation : null_operation_;
  if (operation)
    operation_value.operation_id = next_operation_id_++;

  open_callback_ = callback;
  Call<PpapiPluginMsg_TEECSession_OpenReply>(
      BROWSER,
      PpapiHostMsg_TEECSession_Open(*destination, connection_method,
                                    connection_data_string, operation_value),
      base::Bind(&TEECSessionResource::OnOpenReply, base::Unretained(this),
                 open_callback_, result, operation));
  return PP_OK_COMPLETIONPENDING;
}

int32_t TEECSessionResource::InvokeCommand(
    uint32_t command_id,
    PP_TEEC_Operation* operation,
    PP_TEEC_Result* result,
    scoped_refptr<TrackedCallback> callback) {
  if (!session_opened_) {
    LOG(ERROR) << "Session has not been opened";
    return PP_ERROR_BADARGUMENT;
  }

  if (TrackedCallback::IsPending(open_callback_)) {
    LOG(ERROR) << "Open is in progress";
    return PP_ERROR_BADARGUMENT;
  }

  PP_TEEC_Operation operation_value = operation ? *operation : null_operation_;
  if (operation)
    operation_value.operation_id = next_operation_id_++;

  auto add_result = pending_callbacks_.insert(callback);
  if (!add_result.second) {
    LOG(ERROR) << "InvokeCommand is already in progress";
    return PP_ERROR_INPROGRESS;
  }

  Call<PpapiPluginMsg_TEECSession_InvokeCommandReply>(
      BROWSER,
      PpapiHostMsg_TEECSession_InvokeCommand(command_id, operation_value),
      base::Bind(&TEECSessionResource::OnInvokeCommandReply,
                 base::Unretained(this), callback, result, operation));
  return PP_OK_COMPLETIONPENDING;
}

void TEECSessionResource::OnOpenReply(scoped_refptr<TrackedCallback> callback,
                                      PP_TEEC_Result* out,
                                      PP_TEEC_Operation* operation_out,
                                      const ResourceMessageReplyParams& params,
                                      const PP_TEEC_Result& val,
                                      const PP_TEEC_Operation& operation) {
  open_callback_ = nullptr;
  if (params.result() == PP_OK) {
    session_opened_ = true;
  }

  OnOperationCommandReply(callback, out, operation_out, params, val, operation);
}

void TEECSessionResource::OnInvokeCommandReply(
    scoped_refptr<TrackedCallback> callback,
    PP_TEEC_Result* out,
    PP_TEEC_Operation* operation_out,
    const ResourceMessageReplyParams& params,
    const PP_TEEC_Result& val,
    const PP_TEEC_Operation& operation) {
  pending_callbacks_.erase(callback);

  OnOperationCommandReply(callback, out, operation_out, params, val, operation);
}

void TEECSessionResource::OnOperationCommandReply(
    scoped_refptr<TrackedCallback> callback,
    PP_TEEC_Result* out,
    PP_TEEC_Operation* operation_out,
    const ResourceMessageReplyParams& params,
    const PP_TEEC_Result& val,
    const PP_TEEC_Operation& operation) {
  if (!TrackedCallback::IsPending(callback))
    return;

  if (params.result() == PP_OK) {
    if (out)
      *out = val;
    // write value params back
    if (operation_out) {
      for (int i = 0; i < 4; ++i) {
        auto type = ((operation_out->param_types) >> (8 * i)) & 0x7f;
        switch (type) {
          case PP_TEEC_VALUE_OUTPUT:
          case PP_TEEC_VALUE_INOUT:
            operation_out->params[i].value.a = operation.params[i].value.a;
            operation_out->params[i].value.b = operation.params[i].value.b;
            break;
          case PP_TEEC_MEMREF_WHOLE:
          case PP_TEEC_MEMREF_PARTIAL_OUTPUT:
          case PP_TEEC_MEMREF_PARTIAL_INOUT:
            // todo:
            break;
          default:
            break;
        }
      }
    }
  }

  Run(callback, params.result());
}

int32_t TEECSessionResource::RequestCancellation(
    const PP_TEEC_Operation* operation,
    scoped_refptr<TrackedCallback> callback) {
  if (!open_callback_) {
    LOG(ERROR) << "Session has not been opened";
    return PP_ERROR_BADARGUMENT;
  }

  if (TrackedCallback::IsPending(open_callback_)) {
    LOG(ERROR) << "Open is in progress";
    return PP_ERROR_BADARGUMENT;
  }

  if (!operation) {
    LOG(ERROR) << "Requesting cancellation of null operation";
    return PP_ERROR_BADARGUMENT;
  }

  auto add_result = pending_callbacks_.insert(callback);
  if (!add_result.second) {
    LOG(ERROR) << "RequestCancellation is already in progress";
    return PP_ERROR_INPROGRESS;
  }

  Call<PpapiPluginMsg_TEECSession_RequestCancellationReply>(
      BROWSER, PpapiHostMsg_TEECSession_RequestCancellation(*operation),
      base::Bind(&TEECSessionResource::OnRequestCancellationReply,
                 base::Unretained(this), callback));
  return PP_OK_COMPLETIONPENDING;
}

void TEECSessionResource::OnRequestCancellationReply(
    scoped_refptr<TrackedCallback> callback,
    const ResourceMessageReplyParams& params) {
  pending_callbacks_.erase(callback);

  // Run checks if callback is still pending
  Run(callback, params.result());
}

}  // namespace proxy
}  // namespace ppapi
