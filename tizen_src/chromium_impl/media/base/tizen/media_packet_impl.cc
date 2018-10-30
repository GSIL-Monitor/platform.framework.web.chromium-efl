// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/tizen/media_packet_impl.h"

#include "media/base/timestamp_constants.h"

namespace media {

MediaPacket::MediaPacket(base::TimeDelta timestamp,
                         base::TimeDelta time_duration,
                         media_format_s* format,
                         media::DemuxerStream::Type type)
    : timestamp_(timestamp),
      time_duration_(time_duration),
      format_(format),
      type_(type) {}

MediaPacket::~MediaPacket() {}

media_packet_h MediaPacket::PrepareAndGet() {
  if (!is_prepared_) {
    ScopedMediaPacket packet;
    if (Prepare(packet))
      SetMediaPacket(std::move(packet));
    else
      LOG(ERROR) << "Failed to prepare media packet";
  }
  return Get();
}

media_packet_h MediaPacket::Get() const {
  return media_packet_.get();
}

std::string MediaPacket::ToString() const {
  std::stringstream ss;
  ss << "timestamp: " << timestamp_
     << " type: " << DemuxerStream::ConvertTypeToString(type_);
  return ss.str();
}

void MediaPacket::SetMediaPacket(ScopedMediaPacket packet) {
  media_packet_ = std::move(packet);
  is_prepared_ = !!media_packet_;
}

bool MediaPacket::SetPts(media_packet_h packet, base::TimeDelta timestamp) {
  auto nanoseconds = timestamp.InNanoseconds();
  if (nanoseconds == std::numeric_limits<int64_t>::max()) {
    LOG(ERROR) << "Converting timestamp: " << timestamp
               << " to nanoseconds causes overflow!";
    return false;
  }

  if (media_packet_set_pts(packet, nanoseconds) != MEDIA_PACKET_ERROR_NONE) {
    LOG(ERROR) << "media_packet_set_pts failed";
    return false;
  }

  return true;
}

bool MediaPacket::SetDuration(media_packet_h packet,
                              base::TimeDelta time_duration) {
  auto nanoseconds = time_duration.InNanoseconds();
  if (nanoseconds == std::numeric_limits<int64_t>::max()) {
    LOG(ERROR) << "Converting time duration: " << time_duration
               << " to nanoseconds causes overflow!";
    return false;
  }

  if (media_packet_set_duration(packet, nanoseconds) !=
      MEDIA_PACKET_ERROR_NONE) {
    LOG(ERROR) << "media_packet_set_duration failed";
    return false;
  }

  return true;
}

bool MediaPacket::Prepare(ScopedMediaPacket& packet) {
  if (!SetPts(packet.get(), timestamp_))
    return false;

  return SetDuration(packet.get(), time_duration_);
}

EosMediaPacket::EosMediaPacket(media_format_s* format, DemuxerStream::Type type)
    : media::MediaPacket(media::kNoTimestamp,
                         media::kNoTimestamp,
                         format,
                         type) {}

std::string EosMediaPacket::ToString() const {
  std::stringstream ss;
  ss << "EOS type: " << DemuxerStream::ConvertTypeToString(type_);
  return ss.str();
}

bool EosMediaPacket::Prepare(ScopedMediaPacket& packet) {
  media_packet_h media_packet = nullptr;
  if (media_packet_create(format_, nullptr, nullptr, &media_packet) !=
      MEDIA_PACKET_ERROR_NONE) {
    LOG(ERROR) << "media_packet_create failed";
    return false;
  }

  if (media_packet_set_flags(media_packet, MEDIA_PACKET_END_OF_STREAM) !=
      MEDIA_PACKET_ERROR_NONE) {
    LOG(ERROR) << "media_packet_set_flags failed";
  }
  packet.reset(media_packet);
  return true;
}

CleanMediaPacket::CleanMediaPacket(void* data,
                                   uint64_t size,
                                   base::TimeDelta timestamp,
                                   base::TimeDelta time_duration,
                                   media_format_s* format,
                                   media::DemuxerStream::Type type)
    : MediaPacket(timestamp, time_duration, format, type),
      data_(data),
      size_(size) {}

std::string CleanMediaPacket::ToString() const {
  std::stringstream ss;
  ss << MediaPacket::ToString() << " data ptr: " << data_
     << " data size: " << size_;
  return ss.str();
}

bool CleanMediaPacket::Prepare(ScopedMediaPacket& media_packet) {
  media_packet_h packet = nullptr;
  if (media_packet_create_from_external_memory(format_, data_, size_, nullptr,
                                               nullptr, &packet) !=
      MEDIA_PACKET_ERROR_NONE) {
    LOG(ERROR) << "media_packet_create_from_external_memory failed";
    return false;
  }
  media_packet.reset(packet);

  return MediaPacket::Prepare(media_packet);
}

ShmCleanMediaPacket::ShmCleanMediaPacket(
    base::SharedMemoryHandle foreign_memory_handle,
    uint64_t size,
    base::TimeDelta timestamp,
    base::TimeDelta time_duration,
    media_format_s* format,
    media::DemuxerStream::Type type)
    : CleanMediaPacket(nullptr, size, timestamp, time_duration, format, type),
      foreign_memory_handle_(foreign_memory_handle) {}

std::string ShmCleanMediaPacket::ToString() const {
  std::stringstream ss;
  ss << CleanMediaPacket::ToString()
     << " shm handle: " << foreign_memory_handle_.GetHandle();
  return ss.str();
}

bool ShmCleanMediaPacket::Prepare(ScopedMediaPacket& media_packet) {
  shared_memory_.reset(new base::SharedMemory(foreign_memory_handle_, false));
  if (!shared_memory_->Map(size_)) {
    LOG(ERROR) << "Failed to map shared memory of size " << size_;
    return false;
  }
  data_ = shared_memory_->memory();
  return CleanMediaPacket::Prepare(media_packet);
}

#if defined(OS_TIZEN_TV_PRODUCT)
EncryptedMediaPacket::EncryptedMediaPacket(int tz_handle,
                                           int tz_size,
                                           base::TimeDelta timestamp,
                                           base::TimeDelta time_duration,
                                           media_format_s* format,
                                           media::DemuxerStream::Type type)
    : MediaPacket(timestamp, time_duration, format, type), tz_size_(tz_size) {
  PrepareDrmInfo(tz_handle);
}

std::string EncryptedMediaPacket::ToString() const {
  std::stringstream ss;
  ss << MediaPacket::ToString() << " tz_handle : " << drm_info_.tz_handle
     << " tz_size: " << tz_size_;
  return ss.str();
}

bool EncryptedMediaPacket::Prepare(ScopedMediaPacket& media_packet) {
  media_packet_h packet = nullptr;
  if (media_packet_create(format_, nullptr, nullptr, &packet) !=
      MEDIA_PACKET_ERROR_NONE) {
    LOG(ERROR) << "media_packet_create failed";
    return false;
  }

  media_packet.reset(packet);

  if (media_packet_set_buffer_size(packet, tz_size_) !=
      MEDIA_PACKET_ERROR_NONE) {
    LOG(ERROR) << "media_packet_set_buffer_size failed";
    return false;
  }

  if (!SetDrmInfo(packet, &drm_info_))
    return false;

  return MediaPacket::Prepare(media_packet);
}

void EncryptedMediaPacket::PrepareDrmInfo(int tz_handle) {
  memset(&drm_info_, 0, sizeof(drm_info_));
  drm_info_.drm_type = ES_DRM_TYPE_EME;  // drm_type
  drm_info_.algorithm = DRMB_AES128_CTR; // algorithm parameter
  drm_info_.format = DRMB_FORMAT_FMP4;   // media format
  drm_info_.phase = DRMB_PHASE_NONE;     // cipher phase

  // In GBytes data size will be written before the data.
  drm_info_.pKID = nullptr;  // KID
  drm_info_.uKIDLen = 0;     // KID lenght
  drm_info_.pIV = nullptr;   // initializetion vector
  drm_info_.uIVLen = 0;      // initializetion vector length
  drm_info_.pKey = nullptr;  // for clearkey
  drm_info_.uKeyLen = 0;     // key len

  // SubData Size insert for pSubData in GBytes structure
  drm_info_.pSubData = nullptr;
  drm_info_.drm_handle = 0;
  drm_info_.tz_handle = tz_handle;
}

bool EncryptedMediaPacket::SetDrmInfo(media_packet_h packet,
                                      es_player_drm_info* drm_info) {
  if (media_packet_set_extra(packet, drm_info) != MEDIA_PACKET_ERROR_NONE) {
    LOG(ERROR) << "media_packet_set_extra failed";
    return false;
  }
  return true;
}
#endif

}  // namespace media
