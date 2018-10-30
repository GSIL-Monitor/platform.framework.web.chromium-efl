// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/media_player/elementary_stream_resource.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/proxy/resource_callback_helpers.h"
#include "ppapi/proxy/video_decoder_constants.h"
#include "ppapi/shared_impl/thread_aware_callback.h"

namespace ppapi {
namespace proxy {

namespace {

constexpr uint32_t kSHMThreshold = 32 * 1024;
constexpr uint32_t kInvalidSHMId = std::numeric_limits<uint32_t>::max();
}  // namespace

ElementaryStreamListener::ElementaryStreamListener(
    const PPP_ElementaryStreamListener_Samsung* listener,
    void* user_data)
    : user_data_(user_data) {
  need_data_callback_.reset(
      ThreadAwareCallback<NeedData>::Create(listener->OnNeedData));
  enough_data_callback_.reset(
      ThreadAwareCallback<EnoughData>::Create(listener->OnEnoughData));
  seek_data_callback_.reset(
      ThreadAwareCallback<SeekData>::Create(listener->OnSeekData));

  // Should fail earlier
  DCHECK(need_data_callback_.get() && enough_data_callback_.get() &&
         seek_data_callback_.get());
}

bool ElementaryStreamListener::HandleReply(
    const ResourceMessageReplyParams& params,
    const IPC::Message& msg) {
  PPAPI_BEGIN_MESSAGE_MAP(ElementaryStreamListener, msg)
  PPAPI_DISPATCH_PLUGIN_RESOURCE_CALL(
      PpapiPluginMsg_ElementaryStreamListener_OnNeedData, OnNeedData)
  PPAPI_DISPATCH_PLUGIN_RESOURCE_CALL_0(
      PpapiPluginMsg_ElementaryStreamListener_OnEnoughData, OnEnoughData)
  PPAPI_DISPATCH_PLUGIN_RESOURCE_CALL(
      PpapiPluginMsg_ElementaryStreamListener_OnSeekData, OnSeekData)
  PPAPI_DISPATCH_PLUGIN_RESOURCE_CALL_UNHANDLED(return false)
  PPAPI_END_MESSAGE_MAP()
  return true;
}

void ElementaryStreamListener::OnNeedData(const ResourceMessageReplyParams&,
                                          int32_t bytes_max) {
  need_data_callback_->RunOnTargetThread(bytes_max, user_data_);
}

void ElementaryStreamListener::OnEnoughData(const ResourceMessageReplyParams&) {
  enough_data_callback_->RunOnTargetThread(user_data_);
}

void ElementaryStreamListener::OnSeekData(
    const ResourceMessageReplyParams& params,
    PP_TimeTicks new_position) {
  seek_data_callback_->RunOnTargetThread(new_position, user_data_);
}

ElementaryStreamResourceBase::ShmBuffer::ShmBuffer(
    std::unique_ptr<base::SharedMemory> shm_ptr,
    uint32_t size,
    uint32_t shm_id)
    : shm(std::move(shm_ptr)), addr(nullptr), shm_id(shm_id) {
  if (shm->Map(size))
    addr = shm->memory();
}

ElementaryStreamResourceBase::ShmBuffer::~ShmBuffer() = default;

ElementaryStreamResourceBase::ElementaryStreamResourceBase(
    Connection connection,
    PP_Instance instance)
    : PluginResource(connection, instance) {}

ElementaryStreamResourceBase::~ElementaryStreamResourceBase() {
  for (auto it = append_callbacks_.begin(); it != append_callbacks_.end(); ++it)
    PostAbortIfNecessary(*it);

  PostAbortIfNecessary(flush_callback_);
  PostAbortIfNecessary(set_drm_init_data_callback_);
}

void ElementaryStreamResourceBase::OnReplyReceived(
    const ResourceMessageReplyParams& params,
    const IPC::Message& msg) {
  if (listener_ && listener_->HandleReply(params, msg))
    return;

  PluginResource::OnReplyReceived(params, msg);
}

void ElementaryStreamResourceBase::SetListener(
    std::unique_ptr<ElementaryStreamListener> listener) {
  DCHECK(!listener_);
  listener_.swap(listener);
}

int32_t ElementaryStreamResourceBase::AppendPacket(
    const PP_ESPacket* pkt,
    scoped_refptr<TrackedCallback> callback) {
  if (!pkt) {
    LOG(ERROR) << "Invalid PP_ESPacket";
    return PP_ERROR_BADARGUMENT;
  }

  auto add_result = append_callbacks_.insert(callback);
  if (!add_result.second) {
    LOG(ERROR) << "AppendPacket is already in progress";
    return PP_ERROR_INPROGRESS;
  }

  if (pkt->size >= kSHMThreshold && ReserveSHMBuffer(pkt->size) == PP_OK) {
    // At this point we should have shared memory to hold the plugin's buffer.
    DCHECK(!available_shm_buffers_.empty() &&
           available_shm_buffers_.back()->shm->mapped_size() >= pkt->size);

    PP_ESPacket ipc_pkt = *pkt;
    ipc_pkt.size = 0;
    ipc_pkt.buffer = nullptr;

    ShmBuffer* shm_buffer = available_shm_buffers_.back();
    available_shm_buffers_.pop_back();
    memcpy(shm_buffer->addr, pkt->buffer, pkt->size);

    Call<PpapiPluginMsg_ElementaryStream_AppendPacketReply>(
        BROWSER,
        PpapiHostMsg_ElementaryStream_AppendPacketSHM(
            ipc_pkt, shm_buffer->shm_id, pkt->size),
        base::Bind(&ElementaryStreamResourceBase::OnAppendReplyWithSHM,
                   base::Unretained(this), callback, shm_buffer->shm_id));
  } else {
    Call<PpapiPluginMsg_ElementaryStream_AppendPacketReply>(
        BROWSER, PpapiHostMsg_ElementaryStream_AppendPacket(*pkt),
        base::Bind(&ElementaryStreamResourceBase::OnAppendReply,
                   base::Unretained(this), callback));
  }

  return PP_OK_COMPLETIONPENDING;
}

int32_t ElementaryStreamResourceBase::AppendEncryptedPacket(
    const PP_ESPacket* pkt,
    const PP_ESPacketEncryptionInfo* info,
    scoped_refptr<TrackedCallback> callback) {
  if (!pkt || !info) {
    LOG(ERROR) << "Invalid PP_ESPacket or PP_ESPacketEncryptionInfo";
    return PP_ERROR_BADARGUMENT;
  }

  auto add_result = append_callbacks_.insert(callback);
  if (!add_result.second) {
    LOG(ERROR) << "AppendPacket is already in progress";
    return PP_ERROR_INPROGRESS;
  }

  if (pkt->size >= kSHMThreshold && ReserveSHMBuffer(pkt->size) == PP_OK) {
    // At this point we should have shared memory to hold the plugin's buffer.
    DCHECK(!available_shm_buffers_.empty() &&
           available_shm_buffers_.back()->shm->mapped_size() >= pkt->size);

    PP_ESPacket ipc_pkt = *pkt;
    ipc_pkt.size = 0;
    ipc_pkt.buffer = nullptr;

    ShmBuffer* shm_buffer = available_shm_buffers_.back();
    available_shm_buffers_.pop_back();
    memcpy(shm_buffer->addr, pkt->buffer, pkt->size);

    Call<PpapiPluginMsg_ElementaryStream_AppendEncryptedPacketReply>(
        BROWSER,
        PpapiHostMsg_ElementaryStream_AppendEncryptedPacketSHM(
            ipc_pkt, *info, shm_buffer->shm_id, pkt->size),
        base::Bind(&ElementaryStreamResourceBase::OnAppendReplyWithSHM,
                   base::Unretained(this), callback, shm_buffer->shm_id));
  } else {
    Call<PpapiPluginMsg_ElementaryStream_AppendEncryptedPacketReply>(
        BROWSER,
        PpapiHostMsg_ElementaryStream_AppendEncryptedPacket(*pkt, *info),
        base::Bind(&ElementaryStreamResourceBase::OnAppendReply,
                   base::Unretained(this), callback));
  }

  return PP_OK_COMPLETIONPENDING;
}

int32_t ElementaryStreamResourceBase::AppendTrustZonePacket(
    const PP_ESPacket* pkt,
    const PP_TrustZoneReference* handle,
    scoped_refptr<TrackedCallback> callback) {
  if (!pkt || !handle) {
    LOG(ERROR) << "Invalid PP_ESPacket or PP_TrustZoneReference";
    return PP_ERROR_BADARGUMENT;
  }

  if (pkt->size != 0 || pkt->buffer != nullptr) {
    LOG(ERROR) << "PP_ESPacket fields not set properly";
    return PP_ERROR_BADARGUMENT;
  }

  auto add_result = append_callbacks_.insert(callback);
  if (!add_result.second) {
    LOG(ERROR) << "AppendPacket is already in progress";
    return PP_ERROR_INPROGRESS;
  }

  Call<PpapiPluginMsg_ElementaryStream_AppendTrustZonePacketReply>(
      BROWSER,
      PpapiHostMsg_ElementaryStream_AppendTrustZonePacket(*pkt, *handle),
      base::Bind(&ElementaryStreamResourceBase::OnAppendReply,
                 base::Unretained(this), callback));
  return PP_OK_COMPLETIONPENDING;
}

int32_t ElementaryStreamResourceBase::Flush(
    scoped_refptr<TrackedCallback> callback) {
  if (TrackedCallback::IsPending(flush_callback_)) {
    LOG(ERROR) << "Flush is already in progress";
    return PP_ERROR_INPROGRESS;
  }

  flush_callback_ = callback;
  Call<PpapiPluginMsg_ElementaryStream_FlushReply>(
      BROWSER, PpapiHostMsg_ElementaryStream_Flush(),
      base::Bind(&ElementaryStreamResourceBase::OnFlushReply,
                 base::Unretained(this)));
  return PP_OK_COMPLETIONPENDING;
}

int32_t ElementaryStreamResourceBase::SetDRMInitData(
    uint32_t type_size,
    const void* type_ptr,
    uint32_t init_data_size,
    const void* init_data_ptr,
    scoped_refptr<TrackedCallback> callback) {
  if (TrackedCallback::IsPending(set_drm_init_data_callback_)) {
    LOG(ERROR) << "SetDRMInitData is already in progress";
    return PP_ERROR_INPROGRESS;
  }

  std::vector<uint8_t> type(
      reinterpret_cast<const uint8_t*>(type_ptr),
      reinterpret_cast<const uint8_t*>(type_ptr) + type_size);
  std::vector<uint8_t> init_data(
      reinterpret_cast<const uint8_t*>(init_data_ptr),
      reinterpret_cast<const uint8_t*>(init_data_ptr) + init_data_size);

  set_drm_init_data_callback_ = callback;
  Call<PpapiPluginMsg_ElementaryStream_SetDRMInitDataReply>(
      BROWSER, PpapiHostMsg_ElementaryStream_SetDRMInitData(type, init_data),
      base::Bind(&ElementaryStreamResourceBase::OnSetDRMInitDataReply,
                 base::Unretained(this)));
  return PP_OK_COMPLETIONPENDING;
}

int32_t ElementaryStreamResourceBase::ReserveSHMBuffer(uint32_t size) {
  if (!available_shm_buffers_.empty() &&
      available_shm_buffers_.back()->shm->mapped_size() >= size)
    return PP_OK;

  uint32_t shm_id;
  if (!available_shm_buffers_.empty()) {
    // Signal the host to grow a buffer by passing a legal index. Choose the
    // last available shm buffer for simplicity.
    shm_id = available_shm_buffers_.back()->shm_id;
    available_shm_buffers_.pop_back();
  } else if (shm_buffers_.size() < kMaximumPendingDecodes) {
    // Signal the host to create a new shm buffer by passing an index outside
    // the legal range.
    shm_id = static_cast<uint32_t>(shm_buffers_.size());
  } else {
    // Too many pending appends. Can't allocate new SHM buffer.
    // Return error to signal need of fallback to copying data to IPC channel.
    return PP_ERROR_NOMEMORY;
  }

  // Synchronously get shared memory. Use GenericSyncCall, so we can get the
  // reply params, which contain the handle.
  uint32_t shm_size = 0;
  IPC::Message reply;
  ResourceMessageReplyParams reply_params;
  int32_t result = GenericSyncCall(
      BROWSER, PpapiHostMsg_ElementaryStream_GetShm(shm_id, size), &reply,
      &reply_params);
  if (result != PP_OK) {
    LOG(ERROR) << "Error in GenericSyncCall, result =" << result;
    return PP_ERROR_FAILED;
  }
  if (!UnpackMessage<PpapiPluginMsg_ElementaryStream_GetShmReply>(reply,
                                                                  &shm_size)) {
    LOG(ERROR) << "Cannot unpack message";
    return PP_ERROR_FAILED;
  }
  base::SharedMemoryHandle shm_handle = base::SharedMemoryHandle();
  if (!reply_params.TakeSharedMemoryHandleAtIndex(0, &shm_handle)) {
    LOG(ERROR) << "Cannot take shared memory handle at index 0";
    return PP_ERROR_NOMEMORY;
  }
  std::unique_ptr<base::SharedMemory> shm(
      new base::SharedMemory(shm_handle, false /* read_only */));
  std::unique_ptr<ShmBuffer> shm_buffer(
      new ShmBuffer(std::move(shm), shm_size, shm_id));
  if (!shm_buffer->addr) {
    LOG(ERROR) << "shm_buffer->addr is NULL";
    return PP_ERROR_NOMEMORY;
  }

  available_shm_buffers_.push_back(shm_buffer.get());
  if (shm_id < shm_buffers_.size())
    shm_buffers_[shm_id] = std::move(shm_buffer);
  else
    shm_buffers_.push_back(std::move(shm_buffer));

  return PP_OK;
}

void ElementaryStreamResourceBase::OnAppendReply(
    scoped_refptr<TrackedCallback> callback,
    const ResourceMessageReplyParams& params) {
  append_callbacks_.erase(callback);
  callback->Run(params.result());
}

void ElementaryStreamResourceBase::OnFlushReply(
    const ResourceMessageReplyParams& params) {
  flush_callback_->Run(params.result());
}

void ElementaryStreamResourceBase::OnAppendReplyWithSHM(
    scoped_refptr<TrackedCallback> callback,
    uint32_t shm_id,
    const ResourceMessageReplyParams& params) {
  if (shm_id < shm_buffers_.size()) {
    // Make the shm buffer available.
    available_shm_buffers_.push_back(shm_buffers_[shm_id].get());
  } else {
    LOG(ERROR) << "shm_id outside valid range";
    NOTREACHED();
  }

  OnAppendReply(callback, params);
}

void ElementaryStreamResourceBase::OnSetDRMInitDataReply(
    const ResourceMessageReplyParams& params) {
  set_drm_init_data_callback_->Run(params.result());
}

}  // namespace proxy
}  // namespace ppapi
