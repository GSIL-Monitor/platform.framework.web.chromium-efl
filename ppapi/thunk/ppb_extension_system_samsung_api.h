// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_THUNK_PPB_EXTENSION_SYSTEM_SAMSUNG_API_H_
#define PPAPI_THUNK_PPB_EXTENSION_SYSTEM_SAMSUNG_API_H_

#include "ppapi/c/pp_instance.h"
#include "ppapi/shared_impl/singleton_resource_id.h"
#include "ppapi/thunk/ppapi_thunk_export.h"

namespace ppapi {
namespace thunk {

class PPAPI_THUNK_EXPORT PPB_ExtensionSystem_API {
 public:
  virtual ~PPB_ExtensionSystem_API() {}

  virtual PP_Var GetEmbedderName() = 0;
  virtual PP_Var GetCurrentExtensionInfo() = 0;
  virtual int32_t GenericSyncCall(PP_Var operation_name,
                                  PP_Var operation_data,
                                  PP_Var* operation_result) = 0;

  static const SingletonResourceID kSingletonResourceID =
      EXTENSION_SYSTEM_SINGLETON_ID;
};

}  // namespace thunk
}  // namespace ppapi

#endif  // PPAPI_THUNK_PPB_EXTENSION_SYSTEM_SAMSUNG_API_H_
