// Copyright 2014 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_TIZEN_BROWSER_DEMUXER_EFL_H_
#define CONTENT_BROWSER_MEDIA_TIZEN_BROWSER_DEMUXER_EFL_H_

#include "base/containers/id_map.h"
#include "base/memory/shared_memory.h"
#include "content/public/browser/browser_message_filter.h"
#include "media/base/ranges.h"
#include "media/base/tizen/demuxer_efl.h"

namespace content {

// Represents the browser process half of an IPC-based demuxer proxy.
// It vends out media::DemuxerEfl instances that are registered with an
// end point in the renderer process.
//
// Refer to RendererDemuxerEfl for the renderer process half.
class CONTENT_EXPORT BrowserDemuxerEfl : public BrowserMessageFilter {
 public:
  BrowserDemuxerEfl();
  ~BrowserDemuxerEfl() override;

  // BrowserMessageFilter overrides.
  void OverrideThreadForMessage(const IPC::Message& message,
                                BrowserThread::ID* thread) override;
  bool OnMessageReceived(const IPC::Message& message) override;

  // Returns an uninitialized demuxer implementation associated with
  // |demuxer_client_id|, which can be used to communicate with the real demuxer
  // in the renderer process.
  std::unique_ptr<media::DemuxerEfl> CreateDemuxer(int demuxer_client_id);

 protected:
  friend class base::RefCountedThreadSafe<BrowserDemuxerEfl>;

 private:
  class Internal;

  // Called by internal demuxer implementations to add/remove a client
  // association.
  void AddDemuxerClient(int demuxer_client_id, media::DemuxerEflClient* client);
  void RemoveDemuxerClient(int demuxer_client_id);

  // IPC message handlers.
  void OnDemuxerReady(int demuxer_client_id,
                      const media::DemuxerConfigs& configs);
#if defined(OS_TIZEN_TV_PRODUCT)
  void OnDemuxerStateChanged(int demuxer_client_id,
                             const media::DemuxerConfigs& configs);
#endif
  void OnDemuxerBufferedChanged(int demuxer_client_id,
                                const media::Ranges<base::TimeDelta>& ranges);
  void OnReadFromDemuxerAck(int demuxer_client_id,
                            base::SharedMemoryHandle foreign_memory_handle,
                            const media::DemuxedBufferMetaData& meta_data);
  void OnDemuxerSeekDone(int demuxer_client_id,
                         const base::TimeDelta& actual_browser_seek_time,
                         const base::TimeDelta& video_key_frame);
  void OnDurationChanged(int demuxer_client_id,
                         const base::TimeDelta& duration);

  base::IDMap<media::DemuxerEflClient*> demuxer_clients_;

  DISALLOW_COPY_AND_ASSIGN(BrowserDemuxerEfl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_TIZEN_BROWSER_DEMUXER_EFL_H_
