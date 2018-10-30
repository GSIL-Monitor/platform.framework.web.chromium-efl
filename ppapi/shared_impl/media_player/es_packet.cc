// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/shared_impl/media_player/es_packet.h"

#include <algorithm>
#include <cstring>

#include "base/logging.h"
#include "ppapi/c/pp_bool.h"

namespace ppapi {

ESPacket::ESPacket(const PP_ESPacket& pkt)
    : pts_(pkt.pts),
      dts_(pkt.dts),
      duration_(pkt.duration),
      key_frame_(PP_ToBool(pkt.is_key_frame)),
      data_(reinterpret_cast<const uint8_t*>(pkt.buffer),
            reinterpret_cast<const uint8_t*>(pkt.buffer) + pkt.size) {
  data_ptr_ = data_.data();
  data_size_ = data_.size();
}

ESPacket::ESPacket(const PP_ESPacket& pkt,
                   base::SharedMemory* shm,
                   uint32_t shm_size)
    : pts_(pkt.pts),
      dts_(pkt.dts),
      duration_(pkt.duration),
      key_frame_(PP_ToBool(pkt.is_key_frame)),
      data_ptr_(shm->memory()),
      data_size_(shm_size) {}

ESPacket::ESPacket(ESPacket&& rhs) = default;

ESPacket& ESPacket::operator=(ESPacket&& rhs) = default;

ESPacket::~ESPacket() = default;

EncryptedESPacket::SubsampleDescription::SubsampleDescription(uint32_t clear,
                                                              uint32_t cipher)
    : clear_bytes(clear), cipher_bytes(cipher) {}

EncryptedESPacket::SubsampleDescription::SubsampleDescription() = default;

EncryptedESPacket::SubsampleDescription::SubsampleDescription(
    const SubsampleDescription& rhs) = default;

EncryptedESPacket::SubsampleDescription::SubsampleDescription(
    SubsampleDescription&& rhs) = default;

EncryptedESPacket::SubsampleDescription&
EncryptedESPacket::SubsampleDescription::operator=(
    const SubsampleDescription& rhs) = default;

EncryptedESPacket::SubsampleDescription&
EncryptedESPacket::SubsampleDescription::operator=(SubsampleDescription&& rhs) =
    default;

EncryptedESPacket::SubsampleDescription::~SubsampleDescription() = default;

EncryptedESPacket::EncryptedESPacket(const PP_ESPacket& pkt,
                                     const PP_ESPacketEncryptionInfo& info)
    : ESPacket(pkt),
      key_id_(reinterpret_cast<const uint8_t*>(info.key_id),
              reinterpret_cast<const uint8_t*>(info.key_id) + info.key_id_size),
      iv_(reinterpret_cast<const uint8_t*>(info.iv),
          reinterpret_cast<const uint8_t*>(info.iv) + info.iv_size) {
  CopySubsamples(info);
}

EncryptedESPacket::EncryptedESPacket(const PP_ESPacket& pkt,
                                     base::SharedMemory* shm,
                                     uint32_t shm_size,
                                     const PP_ESPacketEncryptionInfo& info)
    : ESPacket(pkt, shm, shm_size),
      key_id_(reinterpret_cast<const uint8_t*>(info.key_id),
              reinterpret_cast<const uint8_t*>(info.key_id) + info.key_id_size),
      iv_(reinterpret_cast<const uint8_t*>(info.iv),
          reinterpret_cast<const uint8_t*>(info.iv) + info.iv_size) {
  CopySubsamples(info);
}

EncryptedESPacket::EncryptedESPacket(EncryptedESPacket&& rhs) = default;

EncryptedESPacket& EncryptedESPacket::operator=(EncryptedESPacket&& rhs) =
    default;

EncryptedESPacket::~EncryptedESPacket() = default;

void EncryptedESPacket::CopySubsamples(const PP_ESPacketEncryptionInfo& info) {
  for (uint32_t i = 0; i < info.num_subsamples; ++i) {
    subsamples_.emplace_back(info.subsamples[i].clear_bytes,
                             info.subsamples[i].cipher_bytes);
  }
}

}  // namespace ppapi
