// Copyright 2015 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_TIZEN_BROWSER_MEDIAPACKET_MANAGER_H_
#define CONTENT_BROWSER_MEDIA_TIZEN_BROWSER_MEDIAPACKET_MANAGER_H_

#include "content/public/browser/browser_message_filter.h"
#if defined(TIZEN_TBM_SUPPORT)
#include "ui/gfx/tbm_buffer_handle.h"
#endif

namespace content {

class CONTENT_EXPORT BrowserMediaPacketManager : public BrowserMessageFilter {
 public:
  BrowserMediaPacketManager();
  bool OnMessageReceived(const IPC::Message& message) override;

 protected:
  friend class base::RefCountedThreadSafe<BrowserMediaPacketManager>;
  ~BrowserMediaPacketManager() override;

 private:
  void ReleaseMediaPacket(gfx::TbmBufferHandle packet);

  DISALLOW_COPY_AND_ASSIGN(BrowserMediaPacketManager);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_TIZEN_BROWSER_MEDIAPACKET_MANAGER_H_
