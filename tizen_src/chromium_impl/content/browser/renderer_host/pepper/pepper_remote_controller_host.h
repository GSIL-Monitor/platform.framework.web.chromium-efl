// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_REMOTE_CONTROLLER_HOST_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_REMOTE_CONTROLLER_HOST_H_

#include <memory>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/samsung/ppb_remote_controller_samsung.h"
#include "ppapi/host/resource_host.h"

namespace content {

class BrowserPpapiHost;

class PepperRemoteControllerHost
    : public ppapi::host::ResourceHost,
      public base::SupportsWeakPtr<PepperRemoteControllerHost> {
 public:
  PepperRemoteControllerHost(BrowserPpapiHost* host,
                             PP_Instance instance,
                             PP_Resource resource);

  ~PepperRemoteControllerHost() override;

  // Interface for Platform Remote Controller implementation
  class PlatformDelegate {
   public:
    virtual ~PlatformDelegate() {}

    virtual void RegisterKeys(const std::vector<std::string>& keys,
                              const base::Callback<void(int32_t)>& cb) = 0;

    virtual void UnRegisterKeys(const std::vector<std::string>& keys,
                                const base::Callback<void(int32_t)>& cb) = 0;
  };

 protected:
  // ppapi::host::ResourceHost override.
  int32_t OnResourceMessageReceived(
      const IPC::Message& msg,
      ppapi::host::HostMessageContext* context) override;

 private:
  int32_t OnHostMsgRegisterKeys(ppapi::host::HostMessageContext* context,
                                const std::vector<std::string>& keys);

  int32_t OnHostMsgUnRegisterKeys(ppapi::host::HostMessageContext* context,
                                  const std::vector<std::string>& keys);

  void DidRegisterKeys(ppapi::host::ReplyMessageContext reply_context,
                       int32_t result);

  void DidUnRegisterKeys(ppapi::host::ReplyMessageContext reply_context,
                         int32_t result);

  // Used to lazy initialize delegate_
  PlatformDelegate* GetPlatformDelegate();

  BrowserPpapiHost* host_;
  std::unique_ptr<PlatformDelegate> delegate_;

  DISALLOW_COPY_AND_ASSIGN(PepperRemoteControllerHost);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_REMOTE_CONTROLLER_HOST_H_
