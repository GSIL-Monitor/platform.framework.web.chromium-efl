// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_THUNK_PPB_REMOTE_CONTROLLER_SAMSUNG_API_H_
#define PPAPI_THUNK_PPB_REMOTE_CONTROLLER_SAMSUNG_API_H_

#include "ppapi/c/samsung/ppb_remote_controller_samsung.h"
#include "ppapi/shared_impl/singleton_resource_id.h"
#include "ppapi/thunk/ppapi_thunk_export.h"

namespace ppapi {

namespace thunk {

class PPAPI_THUNK_EXPORT PPB_RemoteController_API {
 public:
  virtual ~PPB_RemoteController_API() {}

  virtual int32_t RegisterKeys(PP_Instance instance,
                               uint32_t key_count,
                               const char* keys[]) = 0;
  virtual int32_t UnRegisterKeys(PP_Instance instance,
                                 uint32_t key_count,
                                 const char* keys[]) = 0;

  static const SingletonResourceID kSingletonResourceID =
      REMOTE_CONTROLLER_SINGLETON_ID;
};

}  // namespace thunk
}  // namespace ppapi

#endif  // PPAPI_THUNK_PPB_REMOTE_CONTROLLER_SAMSUNG_API_H_
