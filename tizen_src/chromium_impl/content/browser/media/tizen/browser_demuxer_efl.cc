// Copyright 2014 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/tizen/browser_demuxer_efl.h"
#include "content/common/media/media_player_messages_efl.h"
#include "media/base/decoder_buffer.h"

namespace content {

class BrowserDemuxerEfl::Internal : public media::DemuxerEfl {
 public:
  Internal(const scoped_refptr<BrowserDemuxerEfl>& demuxer,
           int demuxer_client_id)
      : demuxer_(demuxer), demuxer_client_id_(demuxer_client_id) {}

  ~Internal() override {
    DCHECK(ClientIDExists()) << demuxer_client_id_;
    demuxer_->RemoveDemuxerClient(demuxer_client_id_);
  }

  // |media::DemuxerEfl| implementation.
  void Initialize(media::DemuxerEflClient* client) override {
    DCHECK(!ClientIDExists()) << demuxer_client_id_;
    demuxer_->AddDemuxerClient(demuxer_client_id_, client);
  }

  void RequestDemuxerData(media::DemuxerStream::Type type) override {
    DCHECK(ClientIDExists()) << demuxer_client_id_;
    demuxer_->Send(
        new MediaPlayerEflMsg_ReadFromDemuxer(demuxer_client_id_, type));
  }

  void RequestDemuxerSeek(const base::TimeDelta& time_to_seek) override {
    DCHECK(ClientIDExists()) << demuxer_client_id_;
    demuxer_->Send(new MediaPlayerEflMsg_DemuxerSeekRequest(demuxer_client_id_,
                                                            time_to_seek));
  }

 private:
  // Helper for DCHECKing that the ID is still registered.
  bool ClientIDExists() {
    return demuxer_->demuxer_clients_.Lookup(demuxer_client_id_);
  }

  scoped_refptr<BrowserDemuxerEfl> demuxer_;
  int demuxer_client_id_;

  DISALLOW_COPY_AND_ASSIGN(Internal);
};

scoped_refptr<BrowserMessageFilter> CreateBrowserDemuxerEfl() {
  return new BrowserDemuxerEfl();
}

BrowserDemuxerEfl::BrowserDemuxerEfl()
    : BrowserMessageFilter(MediaPlayerMsgStart) {}

BrowserDemuxerEfl::~BrowserDemuxerEfl() {}

void BrowserDemuxerEfl::OverrideThreadForMessage(const IPC::Message& message,
                                                 BrowserThread::ID* thread) {
  switch (message.type()) {
    case MediaPlayerEflHostMsg_DemuxerReady::ID:
    case MediaPlayerEflHostMsg_ReadFromDemuxerAck::ID:
    case MediaPlayerEflHostMsg_DurationChanged::ID:
    case MediaPlayerEflHostMsg_DemuxerSeekDone::ID:
    case MediaPlayerEflHostMsg_DemuxerBufferedChanged::ID:
      *thread = BrowserThread::UI;
      break;
    default:
      break;
  }
}

bool BrowserDemuxerEfl::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(BrowserDemuxerEfl, message)
  IPC_MESSAGE_HANDLER(MediaPlayerEflHostMsg_DemuxerReady, OnDemuxerReady)
#if defined(OS_TIZEN_TV_PRODUCT)
  IPC_MESSAGE_HANDLER(MediaPlayerEflHostMsg_DemuxerStateChanged,
                      OnDemuxerStateChanged)
#endif
  IPC_MESSAGE_HANDLER(MediaPlayerEflHostMsg_DemuxerBufferedChanged,
                      OnDemuxerBufferedChanged)
  IPC_MESSAGE_HANDLER(MediaPlayerEflHostMsg_ReadFromDemuxerAck,
                      OnReadFromDemuxerAck)
  IPC_MESSAGE_HANDLER(MediaPlayerEflHostMsg_DurationChanged, OnDurationChanged)
  IPC_MESSAGE_HANDLER(MediaPlayerEflHostMsg_DemuxerSeekDone, OnDemuxerSeekDone)
  IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

std::unique_ptr<media::DemuxerEfl> BrowserDemuxerEfl::CreateDemuxer(
    int demuxer_client_id) {
  return std::unique_ptr<media::DemuxerEfl>(
      new Internal(this, demuxer_client_id));
}

void BrowserDemuxerEfl::AddDemuxerClient(int demuxer_client_id,
                                         media::DemuxerEflClient* client) {
  demuxer_clients_.AddWithID(client, demuxer_client_id);
}

void BrowserDemuxerEfl::RemoveDemuxerClient(int demuxer_client_id) {
  demuxer_clients_.Remove(demuxer_client_id);
}

void BrowserDemuxerEfl::OnDemuxerReady(int demuxer_client_id,
                                       const media::DemuxerConfigs& configs) {
  media::DemuxerEflClient* client = demuxer_clients_.Lookup(demuxer_client_id);
  if (client)
    client->OnDemuxerConfigsAvailable(configs);
}

#if defined(OS_TIZEN_TV_PRODUCT)
void BrowserDemuxerEfl::OnDemuxerStateChanged(
    int demuxer_client_id,
    const media::DemuxerConfigs& configs) {
  media::DemuxerEflClient* client = demuxer_clients_.Lookup(demuxer_client_id);
  if (client)
    client->OnDemuxerStateChanged(configs);
}
#endif

void BrowserDemuxerEfl::OnDemuxerBufferedChanged(
    int demuxer_client_id,
    const media::Ranges<base::TimeDelta>& ranges) {
  if (media::DemuxerEflClient* client =
          demuxer_clients_.Lookup(demuxer_client_id))
    client->OnDemuxerBufferedChanged(ranges);
}

void BrowserDemuxerEfl::OnReadFromDemuxerAck(
    int demuxer_client_id,
    base::SharedMemoryHandle foreign_memory_handle,
    const media::DemuxedBufferMetaData& meta_data) {
  media::DemuxerEflClient* client = demuxer_clients_.Lookup(demuxer_client_id);
  if (client)
    client->OnDemuxerDataAvailable(foreign_memory_handle, meta_data);
  else {
    LOG(ERROR) << "OnReadFromDemuxerAck, client is null";
    if (base::SharedMemory::IsHandleValid(foreign_memory_handle))
      base::SharedMemory::CloseHandle(foreign_memory_handle);
#if defined(OS_TIZEN_TV_PRODUCT)
    media::ReleaseTzHandle(meta_data.tz_handle, meta_data.size);
#endif
  }
}

void BrowserDemuxerEfl::OnDemuxerSeekDone(
    int demuxer_client_id,
    const base::TimeDelta& actual_browser_seek_time,
    const base::TimeDelta& video_key_frame) {
  media::DemuxerEflClient* client = demuxer_clients_.Lookup(demuxer_client_id);
  if (client)
    client->OnDemuxerSeekDone(actual_browser_seek_time, video_key_frame);
}

void BrowserDemuxerEfl::OnDurationChanged(int demuxer_client_id,
                                          const base::TimeDelta& duration) {
  media::DemuxerEflClient* client = demuxer_clients_.Lookup(demuxer_client_id);
  if (client)
    client->OnDemuxerDurationChanged(duration);
}

}  // namespace content
