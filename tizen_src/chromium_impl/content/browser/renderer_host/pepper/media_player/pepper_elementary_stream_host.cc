// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/media_player/pepper_elementary_stream_host.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "content/browser/renderer_host/pepper/browser_host_callback_helpers.h"
#include "content/browser/renderer_host/pepper/browser_ppapi_host_impl.h"
#include "content/browser/renderer_host/pepper/media_player/pepper_elementary_stream_private.h"
#include "content/public/browser/browser_thread.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/host/host_message_context.h"
#include "ppapi/host/ppapi_host.h"
#include "ppapi/proxy/dispatch_reply_message.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/proxy/serialized_handle.h"
#include "ppapi/proxy/video_decoder_constants.h"
#include "ppapi/shared_impl/media_player/es_packet.h"

namespace content {

namespace {

class ShmWrapper : public base::RefCountedThreadSafe<ShmWrapper> {
 public:
  ShmWrapper() = default;
  explicit ShmWrapper(std::unique_ptr<base::SharedMemory> shm)
      : shm_(std::move(shm)) {}

  std::unique_ptr<base::SharedMemory> ReleaseShm() {
    std::unique_ptr<base::SharedMemory> ret = std::move(shm_);
    shm_ = nullptr;
    return std::move(ret);
  }

 private:
  friend class base::RefCountedThreadSafe<ShmWrapper>;
  ~ShmWrapper() = default;

  std::unique_ptr<base::SharedMemory> shm_;
};

scoped_refptr<ShmWrapper> CreateSHMOnFileThread(uint32_t size) {
  std::unique_ptr<base::SharedMemory> shm(new base::SharedMemory);
  if (!shm->CreateAnonymous(size)) {
    LOG(ERROR) << "Cannot create shared memory buffer";
    return base::WrapRefCounted(new ShmWrapper(nullptr));
  }

  return base::WrapRefCounted(new ShmWrapper(std::move(shm)));
}

void PassSHMToHost(base::WeakPtr<PepperElementaryStreamHost> host,
                   ppapi::host::ReplyMessageContext reply_context,
                   uint32_t shm_id,
                   uint32_t shm_size,
                   scoped_refptr<ShmWrapper> shm) {
  if (!host)
    return;

  host->OnSHMCreated(reply_context, shm_id, shm_size, shm->ReleaseShm());
}

}  // namespace

PepperElementaryStreamHost::PepperElementaryStreamHost(BrowserPpapiHost* host,
                                                       PP_Instance instance,
                                                       PP_Resource resource)
    : ResourceHost(host->GetPpapiHost(), instance, resource),
      weak_ptr_factory_(this),
      host_(host) {}

PepperElementaryStreamHost::~PepperElementaryStreamHost() {}

int32_t PepperElementaryStreamHost::OnResourceMessageReceived(
    const IPC::Message& msg,
    ppapi::host::HostMessageContext* context) {
  PPAPI_BEGIN_MESSAGE_MAP(PepperElementaryStreamHost, msg)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_ElementaryStream_GetShm,
                                    OnGetShm)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_ElementaryStream_AppendPacket,
                                    OnAppendPacket)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL(
      PpapiHostMsg_ElementaryStream_AppendPacketSHM, OnAppendPacketSHM)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL(
      PpapiHostMsg_ElementaryStream_AppendEncryptedPacket,
      OnAppendEncryptedPacket)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL(
      PpapiHostMsg_ElementaryStream_AppendEncryptedPacketSHM,
      OnAppendEncryptedPacketSHM)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL(
      PpapiHostMsg_ElementaryStream_AppendTrustZonePacket,
      OnAppendTrustZonePacket)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(PpapiHostMsg_ElementaryStream_Flush,
                                      OnFlush)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL(
      PpapiHostMsg_ElementaryStream_SetDRMInitData, OnSetDRMInitData)
  PPAPI_END_MESSAGE_MAP()
  LOG(ERROR) << "Resource message unresolved";
  return PP_ERROR_FAILED;
}

base::WeakPtr<ElementaryStreamListenerPrivate>
PepperElementaryStreamHost::ListenerAsWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

int32_t PepperElementaryStreamHost::ValidateShmBuffer(uint32_t shm_id,
                                                      uint32_t shm_size) {
  // |shm_id| is just an index into shm_buffers_. Make sure it's in range.
  if (static_cast<size_t>(shm_id) >= shm_buffers_.size()) {
    LOG(ERROR) << "Shm_id must be in shm_buffers_'s range, shm_id = " << shm_id
               << ", shm_buffers size = " << shm_buffers_.size();
    return PP_ERROR_FAILED;
  }

  // Reject an attempt to pass a busy buffer to the decoder again.
  if (shm_buffer_busy_[shm_id]) {
    LOG(ERROR) << "Rejecting an attempt to pass a busy buffer to the decoder";
    return PP_ERROR_FAILED;
  }

  return PP_OK;
}

// From pepper_video_decoder_host.cc
int32_t PepperElementaryStreamHost::OnGetShm(
    ppapi::host::HostMessageContext* context,
    uint32_t shm_id,
    uint32_t shm_size) {
  // Make the buffers larger since we hope to reuse them.
  shm_size = std::max(shm_size, static_cast<uint32_t>(
                                    ppapi::proxy::kMinimumBitstreamBufferSize));
  if (shm_size > ppapi::proxy::kMaximumBitstreamBufferSize) {
    LOG(ERROR) << "Wrong shm_size = " << shm_size << "- should be less or "
               << "equal to " << ppapi::proxy::kMaximumBitstreamBufferSize;
    return PP_ERROR_FAILED;
  }

  if (shm_id >= ppapi::proxy::kMaximumPendingDecodes) {
    LOG(ERROR) << "Wrong shm_id = " << shm_id << " - should be less than "
               << ppapi::proxy::kMaximumPendingDecodes;
    return PP_ERROR_FAILED;
  }
  // The shm_id must be inside or at the end of shm_buffers_.
  if (shm_id > shm_buffers_.size()) {
    LOG(ERROR) << "The shm_id = " << shm_id << "must be inside or at the end "
               << "of shm_buffers_ = " << shm_buffers_.size();
    return PP_ERROR_FAILED;
  }
  // Reject an attempt to reallocate a busy shm buffer.
  if (shm_id < shm_buffers_.size() && shm_buffer_busy_[shm_id]) {
    LOG(ERROR) << "Rejecting an attempt to reallocate a busy shm buffer, "
               << "shm_id should be greater or equal to shm_buffers size, "
               << "shm_id = " << shm_id
               << ", size of the shm_bufers = " << shm_buffers_.size()
               << ", shm_buffer_busy = "
               << static_cast<bool>(shm_buffer_busy_[shm_id]);
    return PP_ERROR_FAILED;
  }

  BrowserThread::PostTaskAndReplyWithResult(
      BrowserThread::FILE, FROM_HERE,
      base::Bind(&CreateSHMOnFileThread, shm_size),
      base::Bind(&PassSHMToHost, weak_ptr_factory_.GetWeakPtr(),
                 context->MakeReplyMessageContext(), shm_id, shm_size));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperElementaryStreamHost::OnAppendPacket(
    ppapi::host::HostMessageContext* context,
    const PP_ESPacket& packet) {
  ElementaryStreamPrivate()->AppendPacket(
      std::unique_ptr<ppapi::ESPacket>(new ppapi::ESPacket(packet)),
      base::Bind(
          &ReplyCallback<PpapiPluginMsg_ElementaryStream_AppendPacketReply>,
          pp_instance(), context->MakeReplyMessageContext()));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperElementaryStreamHost::OnAppendPacketSHM(
    ppapi::host::HostMessageContext* context,
    const PP_ESPacket& packet,
    uint32_t shm_id,
    uint32_t size) {
  int32_t validate_ret = ValidateShmBuffer(shm_id, size);
  if (validate_ret != PP_OK)
    return validate_ret;

  shm_buffer_busy_[shm_id] = true;
  ElementaryStreamPrivate()->AppendPacket(
      std::unique_ptr<ppapi::ESPacket>(
          new ppapi::ESPacket(packet, shm_buffers_[shm_id].get(), size)),
      base::Bind(&PepperElementaryStreamHost::OnPacketAppended,
                 weak_ptr_factory_.GetWeakPtr(),
                 context->MakeReplyMessageContext(), shm_id));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperElementaryStreamHost::OnAppendEncryptedPacket(
    ppapi::host::HostMessageContext* context,
    const PP_ESPacket& packet,
    const PP_ESPacketEncryptionInfo& info) {
  ElementaryStreamPrivate()->AppendEncryptedPacket(
      std::unique_ptr<ppapi::EncryptedESPacket>(
          new ppapi::EncryptedESPacket(packet, info)),
      base::Bind(
          &ReplyCallback<
              PpapiPluginMsg_ElementaryStream_AppendEncryptedPacketReply>,
          pp_instance(), context->MakeReplyMessageContext()));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperElementaryStreamHost::OnAppendEncryptedPacketSHM(
    ppapi::host::HostMessageContext* context,
    const PP_ESPacket& packet,
    const PP_ESPacketEncryptionInfo& info,
    uint32_t shm_id,
    uint32_t size) {
  int32_t validate_ret = ValidateShmBuffer(shm_id, size);
  if (validate_ret != PP_OK)
    return validate_ret;

  shm_buffer_busy_[shm_id] = true;
  ElementaryStreamPrivate()->AppendEncryptedPacket(
      std::unique_ptr<ppapi::EncryptedESPacket>(new ppapi::EncryptedESPacket(
          packet, shm_buffers_[shm_id].get(), size, info)),
      base::Bind(&PepperElementaryStreamHost::OnEncryptedPacketAppended,
                 weak_ptr_factory_.GetWeakPtr(),
                 context->MakeReplyMessageContext(), shm_id));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperElementaryStreamHost::OnAppendTrustZonePacket(
    ppapi::host::HostMessageContext* context,
    const PP_ESPacket& packet,
    const PP_TrustZoneReference& handle) {
  ElementaryStreamPrivate()->AppendTrustZonePacket(
      std::unique_ptr<ppapi::ESPacket>(new ppapi::ESPacket(packet)), handle,
      base::Bind(
          &ReplyCallback<
              PpapiPluginMsg_ElementaryStream_AppendTrustZonePacketReply>,
          pp_instance(), context->MakeReplyMessageContext()));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperElementaryStreamHost::OnFlush(
    ppapi::host::HostMessageContext* context) {
  ElementaryStreamPrivate()->Flush(
      base::Bind(&ReplyCallback<PpapiPluginMsg_ElementaryStream_FlushReply>,
                 pp_instance(), context->MakeReplyMessageContext()));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperElementaryStreamHost::OnSetDRMInitData(
    ppapi::host::HostMessageContext* context,
    const std::vector<uint8_t>& type,
    const std::vector<uint8_t>& init_data) {
  ElementaryStreamPrivate()->SetDRMInitData(
      type, init_data,
      base::Bind(
          &ReplyCallback<PpapiPluginMsg_ElementaryStream_SetDRMInitDataReply>,
          pp_instance(), context->MakeReplyMessageContext()));
  return PP_OK_COMPLETIONPENDING;
}

void PepperElementaryStreamHost::OnSHMCreated(
    ppapi::host::ReplyMessageContext reply_context,
    uint32_t shm_id,
    uint32_t shm_size,
    std::unique_ptr<base::SharedMemory> shm) {
  if (!shm || !shm->Map(shm_size)) {
    LOG(ERROR) << "shm is NULL or cannot map the shared memory for shm";
    reply_context.params.set_result(PP_ERROR_FAILED);
    host()->SendReply(reply_context,
                      PpapiPluginMsg_ElementaryStream_GetShmReply(shm_size));
    return;
  }

  base::SharedMemoryHandle shm_handle = shm->handle().Duplicate();
  if (!shm_handle.IsValid()) {
    LOG(ERROR) << "Failed to share handle with plugin process";
    reply_context.params.set_result(PP_ERROR_FAILED);
    host()->SendReply(reply_context,
                      PpapiPluginMsg_ElementaryStream_GetShmReply(shm_size));
    return;
  }

  if (shm_id == shm_buffers_.size()) {
    shm_buffers_.push_back(std::move(shm));
    shm_buffer_busy_.push_back(false);
  } else {
    shm_buffers_[shm_id] = std::move(shm);
  }

  ppapi::proxy::SerializedHandle handle(shm_handle, shm_size);
  reply_context.params.AppendHandle(handle);
  host()->SendReply(reply_context,
                    PpapiPluginMsg_ElementaryStream_GetShmReply(shm_size));
}

void PepperElementaryStreamHost::OnPacketAppended(
    ppapi::host::ReplyMessageContext context,
    uint32_t shm_id,
    int32_t result) {
  if (shm_id < shm_buffer_busy_.size() && shm_buffer_busy_[shm_id])
    shm_buffer_busy_[shm_id] = false;
  else
    LOG(ERROR) << "SHM buffer already freed";

  context.params.set_result(result);
  host()->SendReply(context,
                    PpapiPluginMsg_ElementaryStream_AppendPacketReply());
}

void PepperElementaryStreamHost::OnEncryptedPacketAppended(
    ppapi::host::ReplyMessageContext context,
    uint32_t shm_id,
    int32_t result) {
  if (shm_id < shm_buffer_busy_.size() && shm_buffer_busy_[shm_id])
    shm_buffer_busy_[shm_id] = false;
  else
    LOG(ERROR) << "SHM buffer already freed";

  context.params.set_result(result);
  host()->SendReply(
      context, PpapiPluginMsg_ElementaryStream_AppendEncryptedPacketReply());
}

void PepperElementaryStreamHost::OnNeedData(int32_t bytes_max) {
  host()->SendUnsolicitedReply(
      pp_resource(),
      PpapiPluginMsg_ElementaryStreamListener_OnNeedData(bytes_max));
}

void PepperElementaryStreamHost::OnEnoughData() {
  host()->SendUnsolicitedReply(
      pp_resource(), PpapiPluginMsg_ElementaryStreamListener_OnEnoughData());
}

void PepperElementaryStreamHost::OnSeekData(PP_TimeTicks new_position) {
  host()->SendUnsolicitedReply(
      pp_resource(),
      PpapiPluginMsg_ElementaryStreamListener_OnSeekData(new_position));
}

}  // namespace content
