// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/teec_shared_memory_resource.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/proxy/resource_callback_helpers.h"
#include "ppapi/shared_impl/ppapi_globals.h"
#include "ppapi/shared_impl/resource_tracker.h"

namespace ppapi {
namespace proxy {

TEECSharedMemoryResource::TEECSharedMemoryResource(Connection connection,
                                                   PP_Instance instance,
                                                   PP_Resource context,
                                                   uint32_t flags)
    : PluginResource(connection, instance),
      memory_initialized_(false),
      size_(0),
      map_count_(0) {
  SendCreate(BROWSER, PpapiHostMsg_TEECSharedMemory_Create(context, flags));
}

TEECSharedMemoryResource::~TEECSharedMemoryResource() {
  PostAbortIfNecessary(register_callback_);
  PostAbortIfNecessary(allocate_callback_);
}

thunk::PPB_TEECSharedMemory_API*
TEECSharedMemoryResource::AsPPB_TEECSharedMemory_API() {
  return this;
}

int32_t TEECSharedMemoryResource::Register(
    const void* buffer,
    uint32_t size,
    PP_TEEC_Result* result,
    scoped_refptr<TrackedCallback> callback) {
  if (TrackedCallback::IsPending(allocate_callback_)) {
    LOG(ERROR) << "Allocate is already in progress";
    return PP_ERROR_INPROGRESS;
  }

  if (TrackedCallback::IsPending(register_callback_)) {
    LOG(ERROR) << "Register is already in progress";
    return PP_ERROR_INPROGRESS;
  }

  if (memory_initialized_) {
    LOG(ERROR) << "Memory has been already initialized";
    return PP_ERROR_BADARGUMENT;
  }

  if (!buffer) {
    LOG(ERROR) << "buffer is null";
    return PP_ERROR_BADARGUMENT;
  }

  size_ = size;
  register_callback_ = callback;
  Call<PpapiPluginMsg_TEECSharedMemory_RegisterReply>(
      BROWSER, PpapiHostMsg_TEECSharedMemory_Register(/*buffer,*/ size),
      base::Bind(&TEECSharedMemoryResource::OnRegisterReply,
                 base::Unretained(this), register_callback_, result));
  return PP_OK_COMPLETIONPENDING;
}

void TEECSharedMemoryResource::OnRegisterReply(
    scoped_refptr<TrackedCallback> callback,
    PP_TEEC_Result* out,
    const ResourceMessageReplyParams& params,
    const PP_TEEC_Result& val) {
  register_callback_ = nullptr;

  if (!TrackedCallback::IsPending(callback))
    return;

  if (params.result() == PP_OK) {
    memory_initialized_ = true;
    if (out)
      *out = val;
  }

  Run(callback, params.result());
}

int32_t TEECSharedMemoryResource::Allocate(
    uint32_t size,
    PP_TEEC_Result* result,
    scoped_refptr<TrackedCallback> callback) {
  if (TrackedCallback::IsPending(allocate_callback_)) {
    LOG(ERROR) << "Allocate is already in progress";
    return PP_ERROR_INPROGRESS;
  }

  if (TrackedCallback::IsPending(register_callback_)) {
    LOG(ERROR) << "Register is already in progress";
    return PP_ERROR_INPROGRESS;
  }

  if (memory_initialized_) {
    LOG(ERROR) << "Memory has been already initialized";
    return PP_ERROR_BADARGUMENT;
  }

  size_ = size;
  allocate_callback_ = callback;
  Call<PpapiPluginMsg_TEECSharedMemory_AllocateReply>(
      BROWSER, PpapiHostMsg_TEECSharedMemory_Allocate(size),
      base::Bind(&TEECSharedMemoryResource::OnAllocateReply,
                 base::Unretained(this), allocate_callback_, result));
  return PP_OK_COMPLETIONPENDING;
}

void TEECSharedMemoryResource::OnAllocateReply(
    scoped_refptr<TrackedCallback> callback,
    PP_TEEC_Result* out,
    const ResourceMessageReplyParams& params,
    const PP_TEEC_Result& val) {
  allocate_callback_ = nullptr;

  if (!TrackedCallback::IsPending(callback))
    return;

  if (params.result() == PP_OK) {
    memory_initialized_ = true;
    std::vector<base::SharedMemoryHandle> handles;
    params.TakeAllSharedMemoryHandles(&handles);
    DCHECK_EQ(handles.size(), 1u);
    shm_.reset(new base::SharedMemory(handles[0], false));
    if (out)
      *out = val;
  }

  Run(callback, params.result());
}

void* TEECSharedMemoryResource::Map() {
  if (!shm_)
    return nullptr;
  if (map_count_++ == 0)
    shm_->Map(size_);
  return shm_->memory();
}

}  // namespace proxy
}  // namespace ppapi
