// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_PEPPER_RENDERER_PEPPER_HOST_FACTORY_EFL_H_
#define CONTENT_RENDERER_PEPPER_RENDERER_PEPPER_HOST_FACTORY_EFL_H_

#include <memory>

#include "base/macros.h"
#include "base/compiler_specific.h"
#include "ppapi/host/host_factory.h"

namespace content {
class RendererPpapiHost;
}

class RendererPepperHostFactoryEfl : public ppapi::host::HostFactory {
 public:
  explicit RendererPepperHostFactoryEfl(content::RendererPpapiHost* host);
  ~RendererPepperHostFactoryEfl() override;

  // HostFactory.
  std::unique_ptr<ppapi::host::ResourceHost> CreateResourceHost(
      ppapi::host::PpapiHost* host,
      PP_Resource resource,
      PP_Instance instance,
      const IPC::Message& message) override;

 private:
  // Not owned by this object.
  content::RendererPpapiHost* host_;

  DISALLOW_COPY_AND_ASSIGN(RendererPepperHostFactoryEfl);
};

#endif  // CONTENT_RENDERER_PEPPER_RENDERER_PEPPER_HOST_FACTORY_EFL_H_

