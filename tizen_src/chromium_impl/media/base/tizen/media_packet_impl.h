// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/shared_memory.h"
#include "base/memory/shared_memory_handle.h"
#include "media/base/demuxer_stream.h"
#include "media/base/tizen/media_player_util_efl.h"

#if defined(OS_TIZEN_TV_PRODUCT)
#include <player_product.h>
#else
#include <player.h>
#endif

namespace media {

class MediaPacket {
 public:
  MediaPacket(base::TimeDelta timestamp,
              base::TimeDelta time_duration,
              media_format_s* format,
              media::DemuxerStream::Type type);

  virtual ~MediaPacket();

  media_packet_h PrepareAndGet();
  media_packet_h Get() const;
  virtual std::string ToString() const;

  base::TimeDelta GetTimeStamp() const { return timestamp_; }
  base::TimeDelta GetTimeDuration() const { return time_duration_; }
  media::DemuxerStream::Type GetType() const { return type_; }

 protected:
  virtual bool Prepare(ScopedMediaPacket& packet);
  void SetMediaPacket(ScopedMediaPacket packet);
  static bool SetPts(media_packet_h packet, base::TimeDelta timestamp);
  static bool SetDuration(media_packet_h packet, base::TimeDelta time_duration);

  bool is_prepared_ = false;
  ScopedMediaPacket media_packet_;
  base::TimeDelta timestamp_;
  base::TimeDelta time_duration_;
  media_format_s* format_;
  media::DemuxerStream::Type type_;
};

class EosMediaPacket : public MediaPacket {
 public:
  EosMediaPacket(media_format_s* format, media::DemuxerStream::Type type);

  std::string ToString() const override;

 protected:
  bool Prepare(ScopedMediaPacket& packet) override;
};

class CleanMediaPacket : public MediaPacket {
 public:
  CleanMediaPacket(void* data,
                   uint64_t size,
                   base::TimeDelta timestamp,
                   base::TimeDelta time_duration,
                   media_format_s* format,
                   media::DemuxerStream::Type type);

  std::string ToString() const override;

 protected:
  bool Prepare(ScopedMediaPacket& media_packet) override;

  void* data_;
  uint64_t size_;
};

class ShmCleanMediaPacket : public CleanMediaPacket {
 public:
  ShmCleanMediaPacket(base::SharedMemoryHandle foreign_memory_handle,
                      uint64_t size,
                      base::TimeDelta timestamp,
                      base::TimeDelta time_duration,
                      media_format_s* format,
                      media::DemuxerStream::Type type);

  std::string ToString() const override;

 protected:
  bool Prepare(ScopedMediaPacket& media_packet) override;

  std::unique_ptr<base::SharedMemory> shared_memory_;
  base::SharedMemoryHandle foreign_memory_handle_;
};

#if defined(OS_TIZEN_TV_PRODUCT)
class EncryptedMediaPacket : public MediaPacket {
 public:
  EncryptedMediaPacket(int tz_handle,
                       int tz_size,
                       base::TimeDelta timestamp,
                       base::TimeDelta time_duration,
                       media_format_s* format,
                       media::DemuxerStream::Type type);

  std::string ToString() const override;

 protected:
  bool Prepare(ScopedMediaPacket& media_packet) override;
  void PrepareDrmInfo(int tz_handle);
  static bool SetDrmInfo(media_packet_h packet, es_player_drm_info* drm_info);

  int tz_size_;
  es_player_drm_info drm_info_;
};
#endif

}  // namespace media
