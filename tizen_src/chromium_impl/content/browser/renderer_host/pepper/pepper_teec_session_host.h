// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_TEEC_SESSION_HOST_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_TEEC_SESSION_HOST_H_

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/renderer_host/pepper/pepper_teec_session_impl.h"
#include "ppapi/c/samsung/pp_teec_samsung.h"
#include "ppapi/host/resource_host.h"

namespace content {

class BrowserPpapiHost;
class PepperTEECSessionImpl;

class PepperTEECSessionHost : public ppapi::host::ResourceHost {
 public:
  PepperTEECSessionHost(BrowserPpapiHost* host,
                        PP_Instance instance,
                        PP_Resource resource,
                        PP_Resource context);
  ~PepperTEECSessionHost();

  bool IsTEECSessionHost() override;

 protected:
  // ppapi::host::ResourceHost override.
  int32_t OnResourceMessageReceived(
      const IPC::Message& msg,
      ppapi::host::HostMessageContext* context) override;

 private:
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

  scoped_refptr<PepperTEECSessionImpl> impl_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_TEEC_SESSION_HOST_H_
