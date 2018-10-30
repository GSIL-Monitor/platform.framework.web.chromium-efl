// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_THUNK_PPB_TEEC_SESSION_API_H_
#define PPAPI_THUNK_PPB_TEEC_SESSION_API_H_

#include "base/memory/ref_counted.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/samsung/ppb_teec_session_samsung.h"
#include "ppapi/thunk/ppapi_thunk_export.h"

namespace ppapi {

class TrackedCallback;

namespace thunk {

class PPAPI_THUNK_EXPORT PPB_TEECSession_API {
 public:
  virtual ~PPB_TEECSession_API() {}

  virtual int32_t Open(const PP_TEEC_UUID*,
                       uint32_t,
                       uint32_t,
                       const void*,
                       PP_TEEC_Operation*,
                       PP_TEEC_Result*,
                       scoped_refptr<TrackedCallback>) = 0;

  virtual int32_t InvokeCommand(uint32_t,
                                PP_TEEC_Operation*,
                                PP_TEEC_Result*,
                                scoped_refptr<TrackedCallback>) = 0;

  virtual int32_t RequestCancellation(const PP_TEEC_Operation*,
                                      scoped_refptr<TrackedCallback>) = 0;
};

}  // namespace thunk
}  // namespace ppapi

#endif  // PPAPI_THUNK_PPB_TEEC_SESSION_API_H_
