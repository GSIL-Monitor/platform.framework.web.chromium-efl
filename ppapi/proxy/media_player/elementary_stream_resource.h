// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_MEDIA_PLAYER_ELEMENTARY_STREAM_RESOURCE_H_
#define PPAPI_PROXY_MEDIA_PLAYER_ELEMENTARY_STREAM_RESOURCE_H_

#include <cstring>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/memory/ref_counted.h"
#include "ppapi/c/samsung/pp_media_player_samsung.h"
#include "ppapi/proxy/plugin_resource.h"
#include "ppapi/shared_impl/thread_aware_callback.h"
#include "ppapi/thunk/ppb_media_data_source_api.h"

namespace ppapi {
namespace proxy {

class ElementaryStreamListener {
 public:
  ElementaryStreamListener(const PPP_ElementaryStreamListener_Samsung* listener,
                           void* user_data);
  bool HandleReply(const ResourceMessageReplyParams& params,
                   const IPC::Message& msg);

 private:
  void OnNeedData(const ResourceMessageReplyParams& msg, int32_t bytes_max);
  void OnEnoughData(const ResourceMessageReplyParams& msg);
  void OnSeekData(const ResourceMessageReplyParams& msg,
                  PP_TimeTicks new_position);

  typedef void (*NeedData)(int32_t, void*);
  typedef void (*EnoughData)(void*);
  typedef void (*SeekData)(PP_TimeTicks, void*);

  std::unique_ptr<ThreadAwareCallback<NeedData>> need_data_callback_;
  std::unique_ptr<ThreadAwareCallback<EnoughData>> enough_data_callback_;
  std::unique_ptr<ThreadAwareCallback<SeekData>> seek_data_callback_;
  void* user_data_;
};

// This base class holds the non-template methods and attributes.
// SharedMemory comunication based on implementation in VideoDecoder
class ElementaryStreamResourceBase : public PluginResource {
 public:
  virtual ~ElementaryStreamResourceBase();

  void OnReplyReceived(const proxy::ResourceMessageReplyParams& params,
                       const IPC::Message& msg) override;

  void SetListener(std::unique_ptr<ElementaryStreamListener> listener);

 protected:
  // PPBElementaryStreamAPI common implementation
  int32_t AppendPacket(const PP_ESPacket*, scoped_refptr<TrackedCallback>);
  int32_t AppendEncryptedPacket(const PP_ESPacket*,
                                const PP_ESPacketEncryptionInfo*,
                                scoped_refptr<TrackedCallback>);
  int32_t AppendTrustZonePacket(const PP_ESPacket*,
                                const PP_TrustZoneReference*,
                                scoped_refptr<TrackedCallback>);
  int32_t Flush(scoped_refptr<TrackedCallback>);

  int32_t SetDRMInitData(uint32_t type_size,
                         const void* type,
                         uint32_t init_data_size,
                         const void* init_data,
                         scoped_refptr<TrackedCallback>);

  ElementaryStreamResourceBase(Connection, PP_Instance);

 private:
  // Struct to hold a shared memory buffer.
  struct ShmBuffer {
    ShmBuffer(std::unique_ptr<base::SharedMemory> shm,
              uint32_t size,
              uint32_t shm_id);
    ~ShmBuffer();

    const std::unique_ptr<base::SharedMemory> shm;
    void* addr;
    // Index into shm_buffers_ vector, used as an id. This should map 1:1 to
    // the index on the host side of the proxy.
    const uint32_t shm_id;
  };

  struct callback_ptr_hash {
    std::size_t operator()(scoped_refptr<TrackedCallback> const& cp) const {
      return reinterpret_cast<std::size_t>(
          const_cast<TrackedCallback*>(cp.get()));
    }
  };

  int32_t ReserveSHMBuffer(uint32_t size);
  void OnAppendReply(scoped_refptr<TrackedCallback>,
                     const ResourceMessageReplyParams&);
  void OnAppendReplyWithSHM(scoped_refptr<TrackedCallback>,
                            uint32_t shm_id,
                            const ResourceMessageReplyParams&);
  void OnFlushReply(const ResourceMessageReplyParams&);
  void OnSetDRMInitDataReply(const ResourceMessageReplyParams&);

  std::unordered_set<scoped_refptr<TrackedCallback>, callback_ptr_hash>
      append_callbacks_;
  scoped_refptr<TrackedCallback> flush_callback_;
  scoped_refptr<TrackedCallback> set_drm_init_data_callback_;

  std::unique_ptr<ElementaryStreamListener> listener_;

  std::vector<std::unique_ptr<ShmBuffer>> shm_buffers_;

  // List of available shared memory buffers.
  typedef std::vector<ShmBuffer*> ShmBufferList;
  ShmBufferList available_shm_buffers_;

  DISALLOW_COPY_AND_ASSIGN(ElementaryStreamResourceBase);
};

/*
 * Implementation of plugin side Resource proxy for Elementary Stream PPAPI,
 * see ppapi/api/samsung/ppb_media_data_source_samsung.idl
 * for full description of class and its methods.
 *
 * ElementaryStreamT must be PPBElementaryStreamAPI or one of its descendants
 */
template <typename ElementaryStreamT>
class ElementaryStreamResource : public ElementaryStreamResourceBase,
                                 public ElementaryStreamT {
 public:
  virtual ~ElementaryStreamResource() {}

  // PluginResource overrides.
  thunk::PPB_ElementaryStream_API* AsPPB_ElementaryStream_API() override {
    return this;
  }

  // PPBElementaryStreamAPI common implementation
  int32_t AppendPacket(const PP_ESPacket* packet,
                       scoped_refptr<TrackedCallback> callback) override {
    return ElementaryStreamResourceBase::AppendPacket(packet,
                                                      std::move(callback));
  }

  int32_t AppendEncryptedPacket(
      const PP_ESPacket* packet,
      const PP_ESPacketEncryptionInfo* info,
      scoped_refptr<TrackedCallback> callback) override {
    return ElementaryStreamResourceBase::AppendEncryptedPacket(
        packet, info, std::move(callback));
  }

  int32_t AppendTrustZonePacket(
      const PP_ESPacket* packet,
      const PP_TrustZoneReference* handle,
      scoped_refptr<TrackedCallback> callback) override {
    return ElementaryStreamResourceBase::AppendTrustZonePacket(
        packet, handle, std::move(callback));
  }

  int32_t Flush(scoped_refptr<TrackedCallback> callback) override {
    return ElementaryStreamResourceBase::Flush(std::move(callback));
  }

  int32_t SetDRMInitData(uint32_t type_size,
                         const void* type,
                         uint32_t init_data_size,
                         const void* init_data,
                         scoped_refptr<TrackedCallback> callback) override {
    return ElementaryStreamResourceBase::SetDRMInitData(
        type_size, type, init_data_size, init_data, std::move(callback));
  }

 protected:
  ElementaryStreamResource(Connection connection, PP_Instance instance)
      : ElementaryStreamResourceBase(connection, instance) {}
};

}  // namespace proxy
}  // namespace ppapi

#endif  // PPAPI_PROXY_MEDIA_PLAYER_ELEMENTARY_STREAM_RESOURCE_H_
