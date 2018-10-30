// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_TEEC_CONTEXT_RESOURCE_H_
#define PPAPI_PROXY_TEEC_CONTEXT_RESOURCE_H_

#include <string>

#include "ppapi/proxy/plugin_resource.h"
#include "ppapi/proxy/ppapi_proxy_export.h"
#include "ppapi/shared_impl/proxy_lock.h"
#include "ppapi/thunk/ppb_teec_context_api.h"

namespace ppapi {
namespace proxy {

class ResourceMessageReplyParams;

class TEECContextResource : public PluginResource,
                            public thunk::PPB_TEECContext_API {
 public:
  TEECContextResource(Connection connection, PP_Instance instance);
  virtual ~TEECContextResource();

  // PluginResource overrides.
  thunk::PPB_TEECContext_API* AsPPB_TEECContext_API() override;

  int32_t Open(const std::string& name,
               PP_TEEC_Result* result,
               scoped_refptr<TrackedCallback> callback) override;

 private:
  void OnOpenReply(scoped_refptr<TrackedCallback> callback,
                   PP_TEEC_Result* out,
                   const ResourceMessageReplyParams& params,
                   const PP_TEEC_Result& val);

  bool context_opened_;
  scoped_refptr<TrackedCallback> open_callback_;

  DISALLOW_COPY_AND_ASSIGN(TEECContextResource);
};

}  // namespace proxy
}  // namespace ppapi

#endif  //  PPAPI_PROXY_TEEC_CONTEXT_RESOURCE_H_
