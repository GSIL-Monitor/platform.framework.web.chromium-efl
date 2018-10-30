// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_TEEC_CONTEXT_HOST_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_TEEC_CONTEXT_HOST_H_

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/renderer_host/pepper/pepper_teec_context_impl.h"
#include "ppapi/c/samsung/pp_teec_samsung.h"
#include "ppapi/host/resource_host.h"

namespace content {

class BrowserPpapiHost;

class PepperTEECContextHost
    : public ppapi::host::ResourceHost,
      public base::SupportsWeakPtr<PepperTEECContextHost> {
 public:
  PepperTEECContextHost(BrowserPpapiHost* host,
                        PP_Instance instance,
                        PP_Resource resource);
  ~PepperTEECContextHost();

  bool IsTEECContextHost() override;
  scoped_refptr<PepperTEECContextImpl> GetImpl() const { return impl_; }

 protected:
  // ppapi::host::ResourceHost override.
  int32_t OnResourceMessageReceived(
      const IPC::Message& msg,
      ppapi::host::HostMessageContext* context) override;

 private:
  scoped_refptr<PepperTEECContextImpl> impl_;

  int32_t OnOpen(ppapi::host::HostMessageContext*, const std::string&);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_TEEC_CONTEXT_HOST_H_
