// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_TEEC_CONTEXT_IMPL_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_TEEC_CONTEXT_IMPL_H_

#include <tee_client_api.h>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/samsung/pp_teec_samsung.h"

namespace base {
class TaskRunner;
}  // namespace base

namespace ppapi {
namespace host {
struct HostMessageContext;
}  // namespace host
}  // namespace ppapi

namespace content {

class PepperTEECContextImpl
    : public base::RefCountedThreadSafe<PepperTEECContextImpl> {
 public:
  explicit PepperTEECContextImpl(PP_Instance instance);

  void ExecuteTEECTask(
      const base::Callback<PP_TEEC_Result(TEEC_Context*)>& task,
      const base::Callback<void(int32_t, PP_TEEC_Result)>& reply);
  void ExecuteTEECTask(
      const base::Callback<PP_TEEC_Result(TEEC_Context*, PP_TEEC_Operation*)>&
          task,
      const base::Callback<void(int32_t, PP_TEEC_Result, PP_TEEC_Operation)>&
          reply);

  bool HasValidTEECContext() const { return teec_context_initialized_.load(); }
  static int32_t ConvertReturnCode(TEEC_Result result);

 private:
  friend class PepperTEECContextHost;
  friend class base::RefCountedThreadSafe<PepperTEECContextImpl>;

  PP_Instance pp_instance_;
  scoped_refptr<base::TaskRunner> io_runnner_;
  TEEC_Context teec_context_;
  std::atomic<bool> teec_context_initialized_;

  ~PepperTEECContextImpl();

  int32_t OnOpen(ppapi::host::HostMessageContext* context,
                 const std::string& name);
  PP_TEEC_Result Open(const std::string& name);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_TEEC_CONTEXT_IMPL_H_
