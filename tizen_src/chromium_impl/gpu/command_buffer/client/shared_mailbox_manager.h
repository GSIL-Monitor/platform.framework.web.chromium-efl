// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/client/gles2_impl_export.h"

namespace gpu {
namespace gles2 {
class MailboxManager;
}
}  // namespace gpu

struct GLES2_IMPL_EXPORT SharedMailboxManager {
  static gpu::gles2::MailboxManager* GetMailboxManager();
};
