// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/pepper_teec_shared_memory_impl.h"

#include "content/browser/renderer_host/pepper/browser_host_callback_helpers.h"
#include "content/browser/renderer_host/pepper/pepper_teec_context_host.h"
#include "content/browser/renderer_host/pepper/pepper_teec_context_impl.h"
#include "content/public/browser/browser_ppapi_host.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/host/host_message_context.h"
#include "ppapi/host/ppapi_host.h"
#include "ppapi/proxy/dispatch_reply_message.h"
#include "ppapi/proxy/host_dispatcher.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/shared_impl/host_resource.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppb_buffer_api.h"

namespace content {

PepperTEECSharedMemoryImpl::PepperTEECSharedMemoryImpl(BrowserPpapiHost* host,
                                                       PP_Instance instance,
                                                       PP_Resource context,
                                                       uint32_t flags)
    : pp_instance_(instance),
      host_(host),
      flags_(flags),
      size_(0),
      teec_shared_memory_initialized_(false) {
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

PepperTEECSharedMemoryImpl::~PepperTEECSharedMemoryImpl() {
  if (teec_shared_memory_initialized_.load())
    TEEC_ReleaseSharedMemory(&teec_shared_memory_);
}

TEEC_SharedMemory* PepperTEECSharedMemoryImpl::GetSharedMemory() {
  if (!teec_shared_memory_initialized_.load())
    return nullptr;

  return &teec_shared_memory_;
}

bool PepperTEECSharedMemoryImpl::PropagateSharedMemory(
    PP_TEEC_MemoryType direction,
    uint32_t offset,
    uint32_t size) {
  if (!teec_shared_memory_initialized_.load())
    return false;

  if ((flags_ & direction) != direction)
    return false;

  if (!size)
    size = size_;

  if (offset + size > size_)
    return false;

  if (direction == PP_TEEC_MEM_INPUT) {
    memcpy(reinterpret_cast<char*>(teec_shared_memory_.buffer) + offset,
           reinterpret_cast<char*>(shared_memory_->memory()) + offset, size);
  } else {
    memcpy(reinterpret_cast<char*>(shared_memory_->memory()) + offset,
           reinterpret_cast<char*>(teec_shared_memory_.buffer) + offset, size);
  }
  return true;
}

int32_t PepperTEECSharedMemoryImpl::OnRegister(
    ppapi::host::HostMessageContext* context,
    uint32_t size) {
  if (!teec_context_impl_)
    return PP_ERROR_RESOURCE_FAILED;

  return PP_ERROR_NOTSUPPORTED;
}

int32_t PepperTEECSharedMemoryImpl::OnAllocate(
    ppapi::host::HostMessageContext* context,
    uint32_t size) {
  if (!teec_context_host_)
    return PP_ERROR_RESOURCE_FAILED;

  if (teec_shared_memory_initialized_.load())
    return PP_ERROR_FAILED;

  size_ = size;

  auto reply_context = context->MakeReplyMessageContext();

  if (!PrepareBuffer(&reply_context))
    return PP_ERROR_NOMEMORY;

  // lock context
  teec_context_impl_ = teec_context_host_->GetImpl();

  teec_context_impl_->ExecuteTEECTask(
      base::Bind(&PepperTEECSharedMemoryImpl::Allocate, this),
      base::Bind(
          &ReplyCallbackWithValueOutput<
              PpapiPluginMsg_TEECSharedMemory_AllocateReply, PP_TEEC_Result>,
          pp_instance_, reply_context));

  return PP_OK_COMPLETIONPENDING;
}

PP_TEEC_Result PepperTEECSharedMemoryImpl::Allocate(
    TEEC_Context* teec_context) {
  PP_TEEC_Result result;

  // we assume teec_context here is a valid pointer;
  teec_shared_memory_.buffer = nullptr;
  teec_shared_memory_.flags = flags_;
  teec_shared_memory_.size = size_;

  result.return_code = PepperTEECContextImpl::ConvertReturnCode(
      TEEC_AllocateSharedMemory(teec_context, &teec_shared_memory_));
  if (result.return_code == PP_TEEC_SUCCESS) {
    teec_shared_memory_initialized_.store(true);
    result.return_origin = PP_TEEC_ORIGIN_TRUSTED_APP;
  } else {
    // allocating failed so free context lock
    teec_context_impl_ = nullptr;
    result.return_origin = PP_TEEC_ORIGIN_API;
  }
  return result;
}

bool PepperTEECSharedMemoryImpl::PrepareBuffer(
    ppapi::host::ReplyMessageContext* reply_context) {
  shared_memory_.reset(new base::SharedMemory());
  if (!shared_memory_->CreateAndMapAnonymous(size_))
    return false;
  base::SharedMemoryHandle shared_handle = shared_memory_->handle().Duplicate();
  reply_context->params.AppendHandle(
      ppapi::proxy::SerializedHandle(shared_handle, size_));
  return true;
}

}  // namespace content
