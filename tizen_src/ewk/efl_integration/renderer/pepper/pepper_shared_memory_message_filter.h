// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file is copied from chrome/renderer/pepper/
// TODO(a.bujalski) consider moving class from chrome/ to content/

#ifndef RENDERER_PEPPER_PEPPER_SHARED_MEMORY_MESSAGE_FILTER_H_
#define RENDERER_PEPPER_PEPPER_SHARED_MEMORY_MESSAGE_FILTER_H_

#include "base/compiler_specific.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/host/instance_message_filter.h"

#include <stdint.h>

namespace content {
class RendererPpapiHost;
}

namespace ppapi {
namespace proxy {
class SerializedHandle;
}
}  // namespace ppapi

// Implements the backend for shared memory messages from a plugin process.
class PepperSharedMemoryMessageFilter
    : public ppapi::host::InstanceMessageFilter {
 public:
  explicit PepperSharedMemoryMessageFilter(content::RendererPpapiHost* host);
  ~PepperSharedMemoryMessageFilter() override;

  // InstanceMessageFilter:
  bool OnInstanceMessageReceived(const IPC::Message& msg) override;

  bool Send(IPC::Message* msg);

 private:
  // Message handlers.
  void OnHostMsgCreateSharedMemory(
      PP_Instance instance,
      uint32_t size,
      int* host_shm_handle_id,
      ppapi::proxy::SerializedHandle* plugin_shm_handle);

  content::RendererPpapiHost* host_;

  DISALLOW_COPY_AND_ASSIGN(PepperSharedMemoryMessageFilter);
};

#endif  // RENDERER_PEPPER_PEPPER_SHARED_MEMORY_MESSAGE_FILTER_H_
