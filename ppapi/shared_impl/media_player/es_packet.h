// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_SHARED_IMPL_MEDIA_PLAYER_ES_PACKET_H_
#define PPAPI_SHARED_IMPL_MEDIA_PLAYER_ES_PACKET_H_

#include <memory>
#include <utility>
#include <vector>

#include "base/memory/shared_memory.h"
#include "ppapi/c/samsung/pp_media_player_samsung.h"

namespace ppapi {

struct SerializedESPacket;
struct SerializedEncryptedESPacket;

class ESPacket {
 public:
  explicit ESPacket(const PP_ESPacket& pkt);
  ESPacket(const PP_ESPacket& pkt, base::SharedMemory* shm, uint32_t shm_size);

  ESPacket() = delete;
  ESPacket(const ESPacket& rhs) = delete;
  ESPacket(ESPacket&& rhs);

  ESPacket& operator=(const ESPacket& rhs) = delete;
  ESPacket& operator=(ESPacket&& rhs);

  ~ESPacket();

  double Pts() const { return pts_; }
  double Dts() const { return dts_; }
  double Duration() const { return duration_; }

  bool KeyFrame() const { return key_frame_; }

  size_t DataSize() const { return data_size_; }
  const void* Data() const { return data_ptr_; }

 private:
  double pts_;
  double dts_;
  double duration_;
  bool key_frame_;

  const void* data_ptr_;
  size_t data_size_;

  std::vector<uint8_t> data_;
};

class EncryptedESPacket : public ESPacket {
 public:
  struct SubsampleDescription {
    uint32_t clear_bytes;
    uint32_t cipher_bytes;

    SubsampleDescription(uint32_t clear, uint32_t cipher);

    SubsampleDescription();
    SubsampleDescription(const SubsampleDescription& rhs);
    SubsampleDescription(SubsampleDescription&& rhs);

    SubsampleDescription& operator=(const SubsampleDescription& rhs);
    SubsampleDescription& operator=(SubsampleDescription&& rhs);

    ~SubsampleDescription();
  };

  explicit EncryptedESPacket(const PP_ESPacket& pkt,
                             const PP_ESPacketEncryptionInfo& info);

  EncryptedESPacket(const PP_ESPacket& pkt,
                    base::SharedMemory* shm,
                    uint32_t shm_size,
                    const PP_ESPacketEncryptionInfo& info);

  EncryptedESPacket() = delete;
  EncryptedESPacket(const EncryptedESPacket& rhs) = delete;
  EncryptedESPacket(EncryptedESPacket&& rhs);

  EncryptedESPacket& operator=(const EncryptedESPacket& rhs) = delete;
  EncryptedESPacket& operator=(EncryptedESPacket&& rhs);

  ~EncryptedESPacket();

  size_t KeyIdSize() const { return key_id_.size(); }
  const void* KeyIdData() const { return key_id_.data(); }

  size_t IvSize() const { return iv_.size(); }
  const void* IvData() const { return iv_.data(); }

  const std::vector<SubsampleDescription>& Subsamples() const {
    return subsamples_;
  }

 private:
  void CopySubsamples(const PP_ESPacketEncryptionInfo& info);

  std::vector<uint8_t> key_id_;
  std::vector<uint8_t> iv_;
  std::vector<SubsampleDescription> subsamples_;
};

}  // namespace ppapi

#endif  // PPAPI_SHARED_IMPL_MEDIA_PLAYER_ES_PACKET_H_
