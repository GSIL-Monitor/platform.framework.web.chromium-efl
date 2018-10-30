// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_EXTENSION_SYSTEM_RESOURCE_H_
#define PPAPI_PROXY_EXTENSION_SYSTEM_RESOURCE_H_

#include <string>

#include "ppapi/c/samsung/ppb_extension_system_samsung.h"
#include "ppapi/proxy/plugin_resource.h"
#include "ppapi/proxy/ppapi_proxy_export.h"
#include "ppapi/shared_impl/scoped_pp_var.h"
#include "ppapi/thunk/ppb_extension_system_samsung_api.h"

namespace ppapi {
namespace proxy {

class ExtensionSystemResource : public PluginResource,
                                public thunk::PPB_ExtensionSystem_API {
 public:
  ExtensionSystemResource(Connection connection, PP_Instance instance);

  ~ExtensionSystemResource() override;

  // Resource implementation.
  thunk::PPB_ExtensionSystem_API* AsPPB_ExtensionSystem_API() override;

  // PPBExtensionSystemAPI implementation.
  PP_Var GetEmbedderName() override;
  PP_Var GetCurrentExtensionInfo() override;
  int32_t GenericSyncCall(PP_Var operation_name,
                          PP_Var operation_data,
                          PP_Var* operation_result) override;

 private:
  ScopedPPVar current_extension_info_;
  std::string embedder_name_;
};

}  // namespace proxy
}  // namespace ppapi

#endif  // PPAPI_PROXY_EXTENSION_SYSTEM_RESOURCE_H_
