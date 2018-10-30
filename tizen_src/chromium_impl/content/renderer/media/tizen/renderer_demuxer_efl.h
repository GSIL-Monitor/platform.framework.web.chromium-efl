// Copyright 2014 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_TIZEN_RENDERER_DEMUXER_EFL_H_
#define CONTENT_RENDERER_MEDIA_TIZEN_RENDERER_DEMUXER_EFL_H_

#include "base/atomic_sequence_num.h"
#include "base/containers/id_map.h"
#include "media/base/ranges.h"
#include "base/memory/shared_memory.h"
#include "ipc/message_filter.h"
#include "media/base/tizen/demuxer_stream_player_params_efl.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace content {

class MediaSourceDelegateEfl;
class ThreadSafeSender;

// Represents the renderer process half of an IPC-based implementation of
// media::DemuxerEfl.
//
// Refer to BrowserDemuxerEfl for the browser process half.
class RendererDemuxerEfl : public IPC::MessageFilter {
 public:
  RendererDemuxerEfl();

  // Returns the next available demuxer client ID for use in IPC messages.
  //
  // Safe to call on any thread.
  int GetNextDemuxerClientID();

  // Associates |delegate| with |demuxer_client_id| for handling incoming IPC
  // messages.
  //
  // Must be called on media thread.
  void AddDelegate(int demuxer_client_id, MediaSourceDelegateEfl* delegate);

  // Removes the association created by AddDelegate().
  //
  // Must be called on media thread.
  void RemoveDelegate(int demuxer_client_id);

  // IPC::ChannelProxy::MessageFilter overrides.
  bool OnMessageReceived(const IPC::Message& message) override;

  // media::DemuxerEflClient "implementation".
  void DemuxerReady(int demuxer_client_id,
                    const media::DemuxerConfigs& configs);

#if defined(OS_TIZEN_TV_PRODUCT)
  void DemuxerStateChanged(int demuxer_client_id,
                           const media::DemuxerConfigs& configs);
#endif

  void DemuxerBufferedChanged(int demuxer_client_id,
                              const media::Ranges<base::TimeDelta>& ranges);

  bool ReadFromDemuxerAck(int demuxer_client_id,
                          base::SharedMemoryHandle foreign_memory_handle,
                          const media::DemuxedBufferMetaData& meta_data);
  void DemuxerSeekDone(int demuxer_client_id,
                       const base::TimeDelta& actual_browser_seek_time,
                       const base::TimeDelta& video_key_frame);
  void DurationChanged(int demuxer_client_id, const base::TimeDelta& duration);

 protected:
  friend class base::RefCountedThreadSafe<RendererDemuxerEfl>;
  ~RendererDemuxerEfl() override;

 private:
  void DispatchMessage(const IPC::Message& message);
  void OnReadFromDemuxer(int demuxer_client_id,
                         media::DemuxerStream::Type type);
  void OnDemuxerSeekRequest(int demuxer_client_id,
                            const base::TimeDelta& time_to_seek);

  base::AtomicSequenceNumber next_demuxer_client_id_;

  base::IDMap<MediaSourceDelegateEfl*> delegates_;
  scoped_refptr<ThreadSafeSender> thread_safe_sender_;
  scoped_refptr<base::SingleThreadTaskRunner> media_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(RendererDemuxerEfl);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_TIZEN_RENDERER_DEMUXER_EFL_H_
