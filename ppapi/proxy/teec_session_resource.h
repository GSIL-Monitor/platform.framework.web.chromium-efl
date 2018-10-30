// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_TEEC_SESSION_RESOURCE_H_
#define PPAPI_PROXY_TEEC_SESSION_RESOURCE_H_

#include <unordered_set>
#include "base/synchronization/lock.h"
#include "ppapi/proxy/plugin_resource.h"
#include "ppapi/proxy/ppapi_proxy_export.h"
#include "ppapi/shared_impl/proxy_lock.h"
#include "ppapi/thunk/ppb_teec_session_api.h"

namespace ppapi {
namespace proxy {

class ResourceMessageReplyParams;

class TEECSessionResource : public PluginResource,
                            public thunk::PPB_TEECSession_API {
 public:
  TEECSessionResource(Connection connection,
                      PP_Instance instance,
                      PP_Resource context);
  virtual ~TEECSessionResource();

  // PluginResource overrides.
  thunk::PPB_TEECSession_API* AsPPB_TEECSession_API() override;

  int32_t Open(const PP_TEEC_UUID* destination,
               uint32_t connection_method,
               uint32_t connection_data_size,
               const void* connection_data,
               PP_TEEC_Operation* operation,
               PP_TEEC_Result* result,
               scoped_refptr<TrackedCallback> callback) override;

  int32_t InvokeCommand(uint32_t command_id,
                        PP_TEEC_Operation* operation,
                        PP_TEEC_Result* result,
                        scoped_refptr<TrackedCallback> callback) override;

  int32_t RequestCancellation(const PP_TEEC_Operation* operation,
                              scoped_refptr<TrackedCallback>) override;

 private:
  struct callback_ptr_hash {
    std::size_t operator()(scoped_refptr<TrackedCallback> const& cp) const {
      return reinterpret_cast<std::size_t>(
          const_cast<TrackedCallback*>(cp.get()));
    }
  };

  uint32_t next_operation_id_;
  PP_TEEC_Operation null_operation_;

  bool session_opened_;
  scoped_refptr<TrackedCallback> open_callback_;
  std::unordered_set<scoped_refptr<TrackedCallback>, callback_ptr_hash>
      pending_callbacks_;

  void OnOpenReply(scoped_refptr<TrackedCallback> callback,
                   PP_TEEC_Result* out,
                   PP_TEEC_Operation* operation_out,
                   const ResourceMessageReplyParams& params,
                   const PP_TEEC_Result& val,
                   const PP_TEEC_Operation& operation);
  void OnInvokeCommandReply(scoped_refptr<TrackedCallback> callback,
                            PP_TEEC_Result* out,
                            PP_TEEC_Operation* operation_out,
                            const ResourceMessageReplyParams& params,
                            const PP_TEEC_Result& val,
                            const PP_TEEC_Operation& operation);
  void OnOperationCommandReply(scoped_refptr<TrackedCallback> callback,
                               PP_TEEC_Result* out,
                               PP_TEEC_Operation* operation_out,
                               const ResourceMessageReplyParams& params,
                               const PP_TEEC_Result& val,
                               const PP_TEEC_Operation& operation);
  void OnRequestCancellationReply(scoped_refptr<TrackedCallback> callback,
                                  const ResourceMessageReplyParams& params);

  DISALLOW_COPY_AND_ASSIGN(TEECSessionResource);
};

}  // namespace proxy
}  // namespace ppapi

#endif  //  PPAPI_PROXY_TEEC_SESSION_RESOURCE_H_
