// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shared_mailbox_manager.h"

#include "base/memory/ptr_util.h"
#include "gpu/command_buffer/service/mailbox_manager_impl.h"

// static
gpu::gles2::MailboxManager* SharedMailboxManager::GetMailboxManager() {
  static std::unique_ptr<gpu::gles2::MailboxManager> mailbox_manager =
      base::MakeUnique<gpu::gles2::MailboxManagerImpl>();
  return mailbox_manager.get();
}
