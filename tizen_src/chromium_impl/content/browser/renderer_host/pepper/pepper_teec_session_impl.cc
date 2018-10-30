// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/pepper_teec_session_impl.h"

#include "content/browser/renderer_host/pepper/browser_host_callback_helpers.h"
#include "content/browser/renderer_host/pepper/pepper_teec_context_host.h"
#include "content/browser/renderer_host/pepper/pepper_teec_shared_memory_host.h"
#include "content/public/browser/browser_ppapi_host.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/host/ppapi_host.h"
#include "ppapi/proxy/dispatch_reply_message.h"
#include "ppapi/proxy/ppapi_messages.h"

namespace {

TEEC_UUID ConvertTEECUUID(const PP_TEEC_UUID& uuid) {
  TEEC_UUID result{uuid.time_low,
                   uuid.time_mid,
                   uuid.time_hi_and_version,
                   {uuid.clock_seq_and_node[0], uuid.clock_seq_and_node[1],
                    uuid.clock_seq_and_node[2], uuid.clock_seq_and_node[3],
                    uuid.clock_seq_and_node[4], uuid.clock_seq_and_node[5],
                    uuid.clock_seq_and_node[6], uuid.clock_seq_and_node[7]}};

  return result;
}
}  // namespace

namespace content {

PepperTEECSessionImpl::PepperTEECSessionImpl(BrowserPpapiHost* host,
                                             PP_Instance instance,
                                             PP_Resource context)
    : host_(host), pp_instance_(instance), teec_session_initialized_(false) {
  auto teec_context_resource_host =
      host->GetPpapiHost()->GetResourceHost(context);
  if (!teec_context_resource_host) {
    LOG(ERROR) << "Couldn't find TEECContext host: " << context;
    return;
  }

  if (!teec_context_resource_host->IsTEECContextHost()) {
    LOG(ERROR) << "Context PP_Resource is not PepperTEECContextHost";
    return;
  }

  PepperTEECContextHost* teec_context_host =
      static_cast<PepperTEECContextHost*>(teec_context_resource_host);
  teec_context_host_ = teec_context_host->AsWeakPtr();
}

PepperTEECSessionImpl::~PepperTEECSessionImpl() {
  if (teec_session_initialized_.load())
    TEEC_CloseSession(&teec_session_);
}

int32_t PepperTEECSessionImpl::OnOpen(ppapi::host::HostMessageContext* context,
                                      const PP_TEEC_UUID& uuid,
                                      uint32_t connection_method,
                                      const std::string& connection_data,
                                      const PP_TEEC_Operation& operation) {
  if (!teec_context_host_)
    return PP_ERROR_RESOURCE_FAILED;

  if (teec_session_initialized_.load())
    return PP_ERROR_FAILED;

  if (!ValidateTEECOperation(operation))
    return PP_ERROR_BADARGUMENT;

  // lock required resources
  teec_context_impl_ = teec_context_host_->GetImpl();
  if (!teec_context_impl_->HasValidTEECContext()) {
    teec_context_impl_ = nullptr;
    return PP_ERROR_RESOURCE_FAILED;
  }
  LockSharedMemoryResources(operation, true);

  teec_context_impl_->ExecuteTEECTask(
      base::Bind(&PepperTEECSessionImpl::Open, this, uuid, connection_method,
                 connection_data, operation),
      base::Bind(
          &ReplyCallbackWithValueOutput<PpapiPluginMsg_TEECSession_OpenReply,
                                        PP_TEEC_Result, PP_TEEC_Operation>,
          pp_instance_, context->MakeReplyMessageContext()));

  return PP_OK_COMPLETIONPENDING;
}

PP_TEEC_Result PepperTEECSessionImpl::Open(
    const PP_TEEC_UUID& pp_destination,
    uint32_t connection_method,
    const std::string& connection_data,
    const PP_TEEC_Operation& pp_operation,
    TEEC_Context* teec_context,
    PP_TEEC_Operation* out_operation) {
  PP_TEEC_Result result;
  uint32_t return_origin;

  const void* data = connection_data.size() ? connection_data.data() : nullptr;
  auto destination = ConvertTEECUUID(pp_destination);

  if (pp_operation.operation_id) {
    auto operation = ConvertTEECOperation(pp_operation);
    PropagateSharedMemory(pp_operation, PP_TEEC_MEM_INPUT);

    result.return_code = PepperTEECContextImpl::ConvertReturnCode(
        TEEC_OpenSession(teec_context, &teec_session_, &destination,
                         connection_method, data, &operation, &return_origin));
    if (result.return_code == PP_TEEC_SUCCESS)
      teec_session_initialized_.store(true);
    else
      teec_context_impl_ = nullptr;

    PropagateSharedMemory(pp_operation, PP_TEEC_MEM_OUTPUT);
    PrepareOutputOperation(out_operation, pp_operation.operation_id, operation);

    LockSharedMemoryResources(pp_operation, false);
  } else {
    result.return_code = PepperTEECContextImpl::ConvertReturnCode(
        TEEC_OpenSession(teec_context, &teec_session_, &destination,
                         connection_method, data, nullptr, &return_origin));
    if (result.return_code == PP_TEEC_SUCCESS)
      teec_session_initialized_.store(true);
    else
      teec_context_impl_ = nullptr;
  }
  result.return_origin = static_cast<PP_TEEC_Return_Origin>(return_origin);

  return result;
}

int32_t PepperTEECSessionImpl::OnInvokeCommand(
    ppapi::host::HostMessageContext* context,
    uint32_t command_id,
    const PP_TEEC_Operation& operation) {
  // check if session was opened
  if (!teec_session_initialized_.load())
    return PP_ERROR_FAILED;

  // check if context has been removed in the meantime
  if (!teec_context_host_)
    return PP_ERROR_RESOURCE_FAILED;

  if (!ValidateTEECOperation(operation))
    return PP_ERROR_BADARGUMENT;

  // in main thread we must block used shared memory resources
  LockSharedMemoryResources(operation, true);

  teec_context_impl_->ExecuteTEECTask(
      base::Bind(&PepperTEECSessionImpl::InvokeCommand, this, command_id,
                 operation),
      base::Bind(&ReplyCallbackWithValueOutput<
                     PpapiPluginMsg_TEECSession_InvokeCommandReply,
                     PP_TEEC_Result, PP_TEEC_Operation>,
                 pp_instance_, context->MakeReplyMessageContext()));

  return PP_OK_COMPLETIONPENDING;
}

PP_TEEC_Result PepperTEECSessionImpl::InvokeCommand(
    uint32_t command_id,
    const PP_TEEC_Operation& pp_operation,
    TEEC_Context*,
    PP_TEEC_Operation* out_operation) {
  PP_TEEC_Result result;
  uint32_t return_origin;

  if (pp_operation.operation_id) {
    auto operation = ConvertTEECOperation(pp_operation);
    PropagateSharedMemory(pp_operation, PP_TEEC_MEM_INPUT);

    result.return_code =
        PepperTEECContextImpl::ConvertReturnCode(TEEC_InvokeCommand(
            &teec_session_, command_id, &operation, &return_origin));

    PropagateSharedMemory(pp_operation, PP_TEEC_MEM_OUTPUT);
    PrepareOutputOperation(out_operation, pp_operation.operation_id, operation);

    LockSharedMemoryResources(pp_operation, false);
  } else {
    result.return_code =
        PepperTEECContextImpl::ConvertReturnCode(TEEC_InvokeCommand(
            &teec_session_, command_id, nullptr, &return_origin));
  }

  result.return_origin = static_cast<PP_TEEC_Return_Origin>(return_origin);

  return result;
}

void PepperTEECSessionImpl::LockSharedMemoryResources(
    const PP_TEEC_Operation& pp_operation,
    bool lock) {
  if (pp_operation.operation_id)
    return;
  for (int i = 0; i < 4; ++i) {
    auto type = ((pp_operation.param_types) >> (8 * i)) & 0x7f;
    switch (type) {
      case PP_TEEC_MEMREF_WHOLE:
      case PP_TEEC_MEMREF_PARTIAL_INPUT:
      case PP_TEEC_MEMREF_PARTIAL_OUTPUT:
      case PP_TEEC_MEMREF_PARTIAL_INOUT: {
        auto memory_host =
            GetTEECSharedMemoryHost(pp_operation.params[i].memref);
        if (memory_host) {
          base::AutoLock auto_lock{used_memories_lock_};
          if (lock) {
            used_memories_.push_back(memory_host->GetImpl());
          } else {
            used_memories_.erase(std::find(used_memories_.begin(),
                                           used_memories_.end(),
                                           memory_host->GetImpl()));
          }
        }
      } break;
      default:
        break;
    }
  }
}

TEEC_Operation PepperTEECSessionImpl::ConvertTEECOperation(
    const PP_TEEC_Operation& pp_operation) {
  TEEC_Operation result;
  result.started = pp_operation.started;
  result.paramTypes = pp_operation.param_types;
  for (int i = 0; i < 4; ++i) {
    auto type = ((pp_operation.param_types) >> (8 * i)) & 0x7f;
    switch (type) {
      case PP_TEEC_VALUE_INPUT:
      case PP_TEEC_VALUE_OUTPUT:
      case PP_TEEC_VALUE_INOUT:
        result.params[i].value.a = pp_operation.params[i].value.a;
        result.params[i].value.b = pp_operation.params[i].value.b;
        break;
      case PP_TEEC_MEMREF_TEMP_OUTPUT:
      case PP_TEEC_MEMREF_TEMP_INOUT:
        // unsupported by tizen implementation
        break;
      case PP_TEEC_MEMREF_WHOLE:
      case PP_TEEC_MEMREF_PARTIAL_INPUT:
      case PP_TEEC_MEMREF_PARTIAL_OUTPUT:
      case PP_TEEC_MEMREF_PARTIAL_INOUT:
        result.params[i].memref.offset = pp_operation.params[i].memref.offset;
        result.params[i].memref.size = pp_operation.params[i].memref.size;
        result.params[i].memref.parent =
            GetMemoryFromReference(pp_operation.params[i].memref);
        break;
      default:
        break;
    }
  }

  return result;
}

void PepperTEECSessionImpl::PrepareOutputOperation(
    PP_TEEC_Operation* pp_operation,
    uint32_t operation_id,
    const TEEC_Operation& operation) {
  if (!pp_operation)
    return;

  pp_operation->operation_id = operation_id;
  pp_operation->started = operation.started;
  pp_operation->param_types = operation.paramTypes;
  for (int i = 0; i < 4; ++i) {
    auto type = ((pp_operation->param_types) >> (8 * i)) & 0x7f;
    switch (type) {
      case PP_TEEC_VALUE_OUTPUT:
      case PP_TEEC_VALUE_INOUT:
        pp_operation->params[i].value.a = operation.params[i].value.a;
        pp_operation->params[i].value.b = operation.params[i].value.b;
        break;
      case PP_TEEC_MEMREF_WHOLE:
      case PP_TEEC_MEMREF_PARTIAL_INPUT:
      case PP_TEEC_MEMREF_PARTIAL_INOUT:
        pp_operation->params[i].memref.offset =
            operation.params[i].memref.offset;
        pp_operation->params[i].memref.size = operation.params[i].memref.size;
        break;
      default:
        break;
    }
  }
}

bool PepperTEECSessionImpl::ValidateTEECOperation(
    const PP_TEEC_Operation& pp_operation) {
  // 0 means null operation
  if (!pp_operation.operation_id)
    return true;

  for (int i = 0; i < 4; ++i) {
    auto type = ((pp_operation.param_types) >> (8 * i)) & 0x7f;
    switch (type) {
      case PP_TEEC_MEMREF_TEMP_OUTPUT:
      case PP_TEEC_MEMREF_TEMP_INOUT:
        return false;
      case PP_TEEC_MEMREF_WHOLE:
      case PP_TEEC_MEMREF_PARTIAL_INPUT:
      case PP_TEEC_MEMREF_PARTIAL_OUTPUT:
      case PP_TEEC_MEMREF_PARTIAL_INOUT:
        if (!GetMemoryFromReference(pp_operation.params[i].memref))
          return false;
        break;
      default:
        break;
    }
  }
  return true;
}

void PepperTEECSessionImpl::PropagateSharedMemory(
    const PP_TEEC_Operation& pp_operation,
    PP_TEEC_MemoryType direction) {
  for (int i = 0; i < 4; ++i) {
    auto type = ((pp_operation.param_types) >> (8 * i)) & 0x7f;
    bool whole = false;
    if (type == PP_TEEC_MEMREF_PARTIAL_OUTPUT &&
        direction != PP_TEEC_MEM_OUTPUT)
      continue;
    if (type == PP_TEEC_MEMREF_PARTIAL_INPUT && direction != PP_TEEC_MEM_INPUT)
      continue;

    switch (type) {
      case PP_TEEC_MEMREF_WHOLE:
        whole = true;
      // fallthrough
      case PP_TEEC_MEMREF_PARTIAL_OUTPUT:  // fallthrough
      case PP_TEEC_MEMREF_PARTIAL_INPUT:   // fallthrough
      case PP_TEEC_MEMREF_PARTIAL_INOUT: {
        auto teec_shared_memory =
            GetTEECSharedMemoryHost(pp_operation.params[i].memref)->GetImpl();
        uint32_t offset = whole ? 0 : pp_operation.params[i].memref.offset;
        uint32_t size = whole ? 0 : pp_operation.params[i].memref.size;
        teec_shared_memory->PropagateSharedMemory(direction, offset, size);
        break;
      }
      default:
        break;
    }
  }
}

PepperTEECSharedMemoryHost* PepperTEECSessionImpl::GetTEECSharedMemoryHost(
    const PP_TEEC_RegisteredMemoryReference& memory_reference) {
  auto teec_shared_memory_resource_host =
      host_->GetPpapiHost()->GetResourceHost(memory_reference.parent);
  if (!teec_shared_memory_resource_host) {
    LOG(ERROR) << "Couldn't find TEECSharedMemory host: "
               << memory_reference.parent;
    return nullptr;
  }

  if (!teec_shared_memory_resource_host->IsTEECSharedMemoryHost()) {
    LOG(ERROR) << "Context PP_Resource is not PepperTEECContextHost";
    return nullptr;
  }

  return static_cast<PepperTEECSharedMemoryHost*>(
      teec_shared_memory_resource_host);
}

TEEC_SharedMemory* PepperTEECSessionImpl::GetMemoryFromReference(
    const PP_TEEC_RegisteredMemoryReference& memory_reference) {
  auto teec_shared_memory_host = GetTEECSharedMemoryHost(memory_reference);
  if (!teec_shared_memory_host)
    return nullptr;

  return teec_shared_memory_host->GetImpl()->GetSharedMemory();
}

int32_t PepperTEECSessionImpl::OnRequestCancellation(
    ppapi::host::HostMessageContext*,
    const PP_TEEC_Operation&) {
  return PP_ERROR_NOTSUPPORTED;
}

}  // namespace content
