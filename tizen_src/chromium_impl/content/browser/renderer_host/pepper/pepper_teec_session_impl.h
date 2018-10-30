// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_TEEC_SESSION_IMPL_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_TEEC_SESSION_IMPL_H_

#include <tee_client_api.h>

#include "base/memory/weak_ptr.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/samsung/pp_teec_samsung.h"
#include "ppapi/host/resource_host.h"

namespace ppapi {
namespace host {
struct HostMessageContext;
}  // namespace host
}  // namespace ppapi

namespace content {

class BrowserPpapiHost;
class PepperTEECContextHost;
class PepperTEECContextImpl;
class PepperTEECSharedMemoryHost;
class PepperTEECSharedMemoryImpl;

class PepperTEECSessionImpl
    : public base::RefCountedThreadSafe<PepperTEECSessionImpl> {
 public:
  PepperTEECSessionImpl(BrowserPpapiHost* host,
                        PP_Instance instance,
                        PP_Resource context);

 private:
  friend class PepperTEECSessionHost;
  friend class base::RefCountedThreadSafe<PepperTEECSessionImpl>;

  ~PepperTEECSessionImpl();

  int32_t OnOpen(ppapi::host::HostMessageContext* context,
                 const PP_TEEC_UUID& pp_destination,
                 uint32_t connection_method,
                 const std::string& connection_data,
                 const PP_TEEC_Operation& pp_operation);

  int32_t OnInvokeCommand(ppapi::host::HostMessageContext* context,
                          uint32_t command_id,
                          const PP_TEEC_Operation& operation);

  int32_t OnRequestCancellation(ppapi::host::HostMessageContext* context,
                                const PP_TEEC_Operation& operation);

  PP_TEEC_Result Open(const PP_TEEC_UUID& uuid,
                      uint32_t connection_method,
                      const std::string& connection_data,
                      const PP_TEEC_Operation& operation,
                      TEEC_Context* teec_context,
                      PP_TEEC_Operation* out_operation);

  PP_TEEC_Result InvokeCommand(uint32_t command_id,
                               const PP_TEEC_Operation& pp_operation,
                               TEEC_Context* teec_context,
                               PP_TEEC_Operation* out_operation);

  void LockSharedMemoryResources(const PP_TEEC_Operation& pp_operation,
                                 bool lock);
  bool ValidateTEECOperation(const PP_TEEC_Operation& pp_operation);
  TEEC_Operation ConvertTEECOperation(const PP_TEEC_Operation& pp_operation);
  void PrepareOutputOperation(PP_TEEC_Operation* pp_operation,
                              uint32_t operation_id,
                              const TEEC_Operation& operation);

  void PropagateSharedMemory(const PP_TEEC_Operation& pp_operation,
                             PP_TEEC_MemoryType direction);

  TEEC_SharedMemory* GetMemoryFromReference(
      const PP_TEEC_RegisteredMemoryReference& memory_reference);
  PepperTEECSharedMemoryHost* GetTEECSharedMemoryHost(
      const PP_TEEC_RegisteredMemoryReference& memory_reference);

  BrowserPpapiHost* host_;
  PP_Instance pp_instance_;
  base::WeakPtr<PepperTEECContextHost> teec_context_host_;
  scoped_refptr<PepperTEECContextImpl> teec_context_impl_;
  std::vector<scoped_refptr<PepperTEECSharedMemoryImpl>> used_memories_;
  base::Lock used_memories_lock_;

  std::atomic<bool> teec_session_initialized_;
  TEEC_Session teec_session_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_TEEC_SESSION_IMPL_H_
