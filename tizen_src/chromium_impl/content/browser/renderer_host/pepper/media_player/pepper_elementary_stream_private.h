// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_ELEMENTARY_STREAM_PRIVATE_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_ELEMENTARY_STREAM_PRIVATE_H_

#include <memory>
#include <vector>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "ppapi/shared_impl/media_player/es_packet.h"

namespace content {

class ElementaryStreamListenerPrivate;

// Platform depended part of Elementary Stream PPAPI implementation,
// see ppapi/api/samsung/ppb_media_data_source_samsung.idl.

class PepperElementaryStreamPrivate
    : public base::RefCountedThreadSafe<PepperElementaryStreamPrivate> {
 public:
  virtual void AppendPacket(std::unique_ptr<ppapi::ESPacket>,
                            const base::Callback<void(int32_t)>&) = 0;
  virtual void AppendEncryptedPacket(
      std::unique_ptr<ppapi::EncryptedESPacket>,
      const base::Callback<void(int32_t)>& callback) = 0;
  virtual void AppendTrustZonePacket(
      std::unique_ptr<ppapi::ESPacket>,
      const PP_TrustZoneReference& handle,
      const base::Callback<void(int32_t)>& callback) = 0;
  virtual void Flush(const base::Callback<void(int32_t)>&) = 0;
  virtual void SetDRMInitData(const std::vector<uint8_t>& type,
                              const std::vector<uint8_t>& initData,
                              const base::Callback<void(int32_t)>&) = 0;
  virtual void SetListener(
      base::WeakPtr<ElementaryStreamListenerPrivate> listener) = 0;
  virtual void RemoveListener(ElementaryStreamListenerPrivate* listener) = 0;

 protected:
  friend class base::RefCountedThreadSafe<PepperElementaryStreamPrivate>;
  virtual ~PepperElementaryStreamPrivate() {}
};

}  // namespace  content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_ELEMENTARY_STREAM_PRIVATE_H_
