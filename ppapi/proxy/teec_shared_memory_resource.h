// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_TEEC_SHARED_MEMORY_RESOURCE_H_
#define PPAPI_PROXY_TEEC_SHARED_MEMORY_RESOURCE_H_

#include "ppapi/proxy/plugin_resource.h"
#include "ppapi/proxy/ppapi_proxy_export.h"
#include "ppapi/shared_impl/proxy_lock.h"
#include "ppapi/thunk/ppb_teec_shared_memory_api.h"

namespace ppapi {
namespace proxy {

class ResourceMessageReplyParams;

class TEECSharedMemoryResource : public PluginResource,
                                 public thunk::PPB_TEECSharedMemory_API {
 public:
  TEECSharedMemoryResource(Connection connection,
                           PP_Instance instance,
                           PP_Resource context,
                           uint32_t flags);
  virtual ~TEECSharedMemoryResource();

  // PluginResource overrides.
  thunk::PPB_TEECSharedMemory_API* AsPPB_TEECSharedMemory_API() override;

  int32_t Register(const void* buffer,
                   uint32_t size,
                   PP_TEEC_Result* result,
                   scoped_refptr<TrackedCallback> callback) override;

  int32_t Allocate(uint32_t size,
                   PP_TEEC_Result* result,
                   scoped_refptr<TrackedCallback> callback) override;

  void* Map() override;

 private:
  void OnAllocateReply(scoped_refptr<TrackedCallback> callback,
                       PP_TEEC_Result* out,
                       const ResourceMessageReplyParams& params,
                       const PP_TEEC_Result& val);
  void OnRegisterReply(scoped_refptr<TrackedCallback> callback,
                       PP_TEEC_Result* out,
                       const ResourceMessageReplyParams& params,
                       const PP_TEEC_Result& val);

  bool memory_initialized_;
  scoped_refptr<TrackedCallback> register_callback_;
  scoped_refptr<TrackedCallback> allocate_callback_;
  std::unique_ptr<base::SharedMemory> shm_;
  uint32_t size_;
  int map_count_;
  DISALLOW_COPY_AND_ASSIGN(TEECSharedMemoryResource);
};

}  // namespace proxy
}  // namespace ppapi

#endif  //  PPAPI_PROXY_TEEC_SHARED_MEMORY_RESOURCE_H_
