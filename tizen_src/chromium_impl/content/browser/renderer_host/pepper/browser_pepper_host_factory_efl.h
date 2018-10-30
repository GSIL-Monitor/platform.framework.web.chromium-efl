// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_PEPPER_HOST_FACTORY_EFL_H_
#define BROWSER_PEPPER_HOST_FACTORY_EFL_H_

#include "base/macros.h"

#include "base/compiler_specific.h"
#include "ppapi/host/host_factory.h"

namespace content {
class BrowserPpapiHost;
}  // namespace content

class BrowserPepperHostFactoryEfl : public ppapi::host::HostFactory {
 public:
  // Non-owning pointer to the filter must outlive this class.
  explicit BrowserPepperHostFactoryEfl(content::BrowserPpapiHost* host);
  ~BrowserPepperHostFactoryEfl() override;

  std::unique_ptr<ppapi::host::ResourceHost> CreateResourceHost(
      ppapi::host::PpapiHost* host,
      PP_Resource resource,
      PP_Instance instance,
      const IPC::Message& message) override;

 private:
  // Non-owning pointer.
  content::BrowserPpapiHost* host_;

  DISALLOW_COPY_AND_ASSIGN(BrowserPepperHostFactoryEfl);
};

#endif  // BROWSER_PEPPER_HOST_FACTORY_EFL_H_
