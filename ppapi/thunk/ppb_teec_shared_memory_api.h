// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_THUNK_PPB_TEEC_SHARED_MEMORY_API_H_
#define PPAPI_THUNK_PPB_TEEC_SHARED_MEMORY_API_H_

#include "base/memory/ref_counted.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/samsung/ppb_teec_shared_memory_samsung.h"
#include "ppapi/thunk/ppapi_thunk_export.h"

namespace ppapi {

class TrackedCallback;

namespace thunk {

class PPAPI_THUNK_EXPORT PPB_TEECSharedMemory_API {
 public:
  virtual ~PPB_TEECSharedMemory_API() {}

  virtual int32_t Register(const void*,
                           uint32_t size,
                           PP_TEEC_Result*,
                           scoped_refptr<TrackedCallback>) = 0;

  virtual int32_t Allocate(uint32_t,
                           PP_TEEC_Result*,
                           scoped_refptr<TrackedCallback>) = 0;

  virtual void* Map() = 0;
};

}  // namespace thunk
}  // namespace ppapi

#endif  // PPAPI_THUNK_PPB_TEEC_SHARED_MEMORY_API_H_
