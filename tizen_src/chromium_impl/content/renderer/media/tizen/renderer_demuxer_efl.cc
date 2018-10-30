// Copyright 2014 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/tizen/renderer_demuxer_efl.h"

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "content/child/thread_safe_sender.h"
#include "content/common/media/media_player_messages_efl.h"
#include "content/renderer/media/tizen/media_source_delegate_efl.h"
#include "content/renderer/media/tizen/renderer_media_player_manager_efl.h"
#include "content/renderer/render_thread_impl.h"

namespace content {

scoped_refptr<IPC::MessageFilter> CreateRendererDemuxerEfl() {
  return new RendererDemuxerEfl();
}

RendererDemuxerEfl::RendererDemuxerEfl()
    : thread_safe_sender_(RenderThreadImpl::current()->thread_safe_sender()),
      media_task_runner_(
          RenderThreadImpl::current()->GetMediaThreadTaskRunner()) {}

RendererDemuxerEfl::~RendererDemuxerEfl() {}

int RendererDemuxerEfl::GetNextDemuxerClientID() {
  // Don't use zero for IDs since it can be interpreted as having no ID.
  return next_demuxer_client_id_.GetNext() + 1;
}

void RendererDemuxerEfl::AddDelegate(int demuxer_client_id,
                                     MediaSourceDelegateEfl* delegate) {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  delegates_.AddWithID(delegate, demuxer_client_id);
}

void RendererDemuxerEfl::RemoveDelegate(int demuxer_client_id) {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  delegates_.Remove(demuxer_client_id);
}

bool RendererDemuxerEfl::OnMessageReceived(const IPC::Message& message) {
  switch (message.type()) {
    case MediaPlayerEflMsg_ReadFromDemuxer::ID:
    case MediaPlayerEflMsg_DemuxerSeekRequest::ID:
      media_task_runner_->PostTask(
          FROM_HERE,
          base::Bind(&RendererDemuxerEfl::DispatchMessage, this, message));
      return true;
  }
  return false;
}

void RendererDemuxerEfl::DemuxerReady(int demuxer_client_id,
                                      const media::DemuxerConfigs& configs) {
  thread_safe_sender_->Send(
      new MediaPlayerEflHostMsg_DemuxerReady(demuxer_client_id, configs));
}

#if defined(OS_TIZEN_TV_PRODUCT)
void RendererDemuxerEfl::DemuxerStateChanged(
    int demuxer_client_id,
    const media::DemuxerConfigs& configs) {
  thread_safe_sender_->Send(new MediaPlayerEflHostMsg_DemuxerStateChanged(
      demuxer_client_id, configs));
}
#endif

void RendererDemuxerEfl::DemuxerBufferedChanged(
    int demuxer_client_id,
    const media::Ranges<base::TimeDelta>& ranges) {
  thread_safe_sender_->Send(new MediaPlayerEflHostMsg_DemuxerBufferedChanged(
      demuxer_client_id, ranges));
}

bool RendererDemuxerEfl::ReadFromDemuxerAck(
    int demuxer_client_id,
    base::SharedMemoryHandle foreign_memory_handle,
    const media::DemuxedBufferMetaData& meta_data) {
  return thread_safe_sender_->Send(new MediaPlayerEflHostMsg_ReadFromDemuxerAck(
      demuxer_client_id, foreign_memory_handle, meta_data));
}

void RendererDemuxerEfl::DemuxerSeekDone(
    int demuxer_client_id,
    const base::TimeDelta& actual_browser_seek_time,
    const base::TimeDelta& video_key_frame) {
  thread_safe_sender_->Send(new MediaPlayerEflHostMsg_DemuxerSeekDone(
      demuxer_client_id, actual_browser_seek_time, video_key_frame));
}

void RendererDemuxerEfl::DurationChanged(int demuxer_client_id,
                                         const base::TimeDelta& duration) {
  thread_safe_sender_->Send(
      new MediaPlayerEflHostMsg_DurationChanged(demuxer_client_id, duration));
}

void RendererDemuxerEfl::DispatchMessage(const IPC::Message& message) {
  IPC_BEGIN_MESSAGE_MAP(RendererDemuxerEfl, message)
  IPC_MESSAGE_HANDLER(MediaPlayerEflMsg_ReadFromDemuxer, OnReadFromDemuxer)
  IPC_MESSAGE_HANDLER(MediaPlayerEflMsg_DemuxerSeekRequest,
                      OnDemuxerSeekRequest)
  IPC_END_MESSAGE_MAP()
}

void RendererDemuxerEfl::OnReadFromDemuxer(int demuxer_client_id,
                                           media::DemuxerStream::Type type) {
  MediaSourceDelegateEfl* delegate = delegates_.Lookup(demuxer_client_id);
  if (delegate)
    delegate->OnReadFromDemuxer(type);
}

void RendererDemuxerEfl::OnDemuxerSeekRequest(
    int demuxer_client_id,
    const base::TimeDelta& time_to_seek) {
  MediaSourceDelegateEfl* delegate = delegates_.Lookup(demuxer_client_id);
  if (delegate)
    delegate->StartSeek(time_to_seek, false);
}

}  // namespace content
