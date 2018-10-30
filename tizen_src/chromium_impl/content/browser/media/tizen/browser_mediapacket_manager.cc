// Copyright 2015 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/tizen/browser_mediapacket_manager.h"

#include <media_packet.h>
#include <unordered_map>

#include "content/common/media/media_player_messages_efl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "ipc/ipc_message_macros.h"

namespace {

typedef std::unordered_map<void*, content::RenderFrameHost*> MediaPacketMap;
MediaPacketMap media_packet_map;

base::Lock media_packet_map_locker;
}

namespace content {

// These functions are to add or the media_packet in any place.
// It's reentrant function which can be used on Browser Process,
// and using this on other Processes is undefined.
// So when it's used by GPU Thread, it should be guaranteed
// that GPU Thread lives on Browser Process.(like Android)

void AddMediaPacket(void* media_packet_handle,
                    RenderFrameHost* render_frame_host) {
  base::AutoLock lock(media_packet_map_locker);
  media_packet_map[media_packet_handle] = render_frame_host;
  DLOG(INFO) << " AddMediaPacket : " << media_packet_handle;
}

void CleanUpMediaPacket(RenderFrameHost* render_frame_host) {
  // clean up pending mediapacket.
  base::AutoLock lock(media_packet_map_locker);
  for (auto iter = media_packet_map.begin(); iter != media_packet_map.end();) {
    if (render_frame_host == iter->second) {
      LOG(INFO) << " Release : pending media packet : " << iter->first;
      if (MEDIA_PACKET_ERROR_NONE !=
          media_packet_destroy(static_cast<media_packet_h>(iter->first)))
        LOG(WARNING) << "Fail to release media_packet";

      iter = media_packet_map.erase(iter);
    } else {
      ++iter;
    }
  }
}

void DestroyMediaPacket(void* media_packet_handle) {
  {
    base::AutoLock lock(media_packet_map_locker);

    // prevent double free.
    if (!media_packet_map.erase(media_packet_handle)) {
      LOG(WARNING) << "MediaPakcet(" << media_packet_handle
                   << ") is not in media packet map";
      return;
    }
  }

  DLOG(INFO) << " Destroy media packet : " << media_packet_handle;
  if (MEDIA_PACKET_ERROR_NONE !=
      media_packet_destroy(static_cast<media_packet_h>(media_packet_handle)))
    LOG(WARNING) << "Fail to release media_packet";
}

scoped_refptr<BrowserMessageFilter> CreateBrowserMediapacketManager() {
  return new BrowserMediaPacketManager();
}

BrowserMediaPacketManager::BrowserMediaPacketManager()
    : BrowserMessageFilter(MediaPlayerMsgStart) {}

BrowserMediaPacketManager::~BrowserMediaPacketManager() {}

bool BrowserMediaPacketManager::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(BrowserMediaPacketManager, message)
  IPC_MESSAGE_HANDLER(MediaPlayerEflHostMsg_ReleaseTbmBuffer,
                      ReleaseMediaPacket)
  IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void BrowserMediaPacketManager::ReleaseMediaPacket(
    gfx::TbmBufferHandle packet) {
  DLOG(INFO) << "ReleaseMediaPacket, media_packet: " << packet.media_packet;
  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                          base::Bind(&DestroyMediaPacket, packet.media_packet));
}

}  // namespace content
