// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_ELEMENTARY_STREAM_HOST_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_ELEMENTARY_STREAM_HOST_H_

#include <vector>

#include "base/memory/shared_memory.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/renderer_host/pepper/media_player/elementary_stream_listener_private.h"
#include "ppapi/c/samsung/pp_media_player_samsung.h"
#include "ppapi/host/resource_host.h"

namespace content {

class PepperElementaryStreamPrivate;
class BrowserPpapiHost;

class PepperElementaryStreamHost : public ppapi::host::ResourceHost,
                                   public ElementaryStreamListenerPrivate {
 public:
  PepperElementaryStreamHost(BrowserPpapiHost*, PP_Instance, PP_Resource);
  virtual ~PepperElementaryStreamHost();

  // ElementaryStreamListenerPrivate
  void OnNeedData(int32_t bytes_max) override;
  void OnEnoughData() override;
  void OnSeekData(PP_TimeTicks new_position) override;

  void OnSHMCreated(ppapi::host::ReplyMessageContext reply_context,
                    uint32_t shm_id,
                    uint32_t shm_size,
                    std::unique_ptr<base::SharedMemory> shm);

 protected:
  int32_t OnResourceMessageReceived(const IPC::Message&,
                                    ppapi::host::HostMessageContext*) override;

  base::WeakPtr<ElementaryStreamListenerPrivate> ListenerAsWeakPtr();

  virtual PepperElementaryStreamPrivate* ElementaryStreamPrivate() = 0;

 private:
  int32_t ValidateShmBuffer(uint32_t shm_id, uint32_t shm_size);
  int32_t OnGetShm(ppapi::host::HostMessageContext* context,
                   uint32_t shm_id,
                   uint32_t shm_size);
  int32_t OnAppendPacket(ppapi::host::HostMessageContext*, const PP_ESPacket&);
  int32_t OnAppendPacketSHM(ppapi::host::HostMessageContext*,
                            const PP_ESPacket&,
                            uint32_t shm_id,
                            uint32_t size);
  int32_t OnAppendEncryptedPacket(ppapi::host::HostMessageContext*,
                                  const PP_ESPacket&,
                                  const PP_ESPacketEncryptionInfo&);
  int32_t OnAppendEncryptedPacketSHM(ppapi::host::HostMessageContext*,
                                     const PP_ESPacket&,
                                     const PP_ESPacketEncryptionInfo&,
                                     uint32_t shm_id,
                                     uint32_t size);
  int32_t OnAppendTrustZonePacket(ppapi::host::HostMessageContext* context,
                                  const PP_ESPacket& packet,
                                  const PP_TrustZoneReference& handle);
  int32_t OnFlush(ppapi::host::HostMessageContext*);
  int32_t OnSetDRMInitData(ppapi::host::HostMessageContext*,
                           const std::vector<uint8_t>& type,
                           const std::vector<uint8_t>& init_data);

  void OnPacketAppended(ppapi::host::ReplyMessageContext context,
                        uint32_t shm_id,
                        int32_t result);

  void OnEncryptedPacketAppended(ppapi::host::ReplyMessageContext context,
                                 uint32_t shm_id,
                                 int32_t result);

  base::WeakPtrFactory<PepperElementaryStreamHost> weak_ptr_factory_;

  // A vector holding our shm buffers, in sync with a similar vector in the
  // resource. We use a buffer's index in these vectors as its id on both sides
  // of the proxy. Only add buffers or update them in place so as not to
  // invalidate these ids.
  std::vector<std::unique_ptr<base::SharedMemory>> shm_buffers_;
  // A vector of true/false indicating if a shm buffer is in use by the decoder.
  // This is parallel to |shm_buffers_|.
  std::vector<uint8_t> shm_buffer_busy_;

  // Non-owning pointer
  BrowserPpapiHost* host_;

  DISALLOW_COPY_AND_ASSIGN(PepperElementaryStreamHost);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_ELEMENTARY_STREAM_HOST_H_
