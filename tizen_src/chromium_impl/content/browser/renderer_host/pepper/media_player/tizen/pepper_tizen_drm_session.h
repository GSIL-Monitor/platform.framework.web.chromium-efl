// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_TIZEN_DRM_SESSION_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_TIZEN_DRM_SESSION_H_

#include <drmdecrypt_api.h>
#include <emeCDM/IEME.h>
#include <memory>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "ppapi/c/samsung/pp_media_player_samsung.h"
#include "ppapi/shared_impl/media_player/es_packet.h"

namespace content {

class PepperTizenDRMListener;

struct HandleAndSizeDeleter {
  void operator()(handle_and_size_s* ptr) const;
};

using ScopedHandleAndSize =
    std::unique_ptr<handle_and_size_s, HandleAndSizeDeleter>;

class PepperTizenDRMSession
    : public base::RefCountedThreadSafe<PepperTizenDRMSession>,
      public base::SupportsWeakPtr<PepperTizenDRMSession> {
 public:
  static scoped_refptr<PepperTizenDRMSession> Create(PP_MediaPlayerDRMType,
                                                     eme::IEventListener*);

  ~PepperTizenDRMSession();

  bool Initialize(const std::string& init_data);

  ScopedHandleAndSize Decrypt(const ppapi::EncryptedESPacket&);

  bool InstallLicense(const std::string& response);

  bool is_valid() const { return ieme_ && !session_id_.empty(); }

  std::string session_id() const { return session_id_; }

 private:
  friend class scoped_refptr<PepperTizenDRMSession>;
  PepperTizenDRMSession(PP_MediaPlayerDRMType drm_type,
                        const std::string& keysystem_name,
                        eme::IEventListener* listener);

  PP_MediaPlayerDRMType drm_type_;
  std::string session_id_;
  std::unique_ptr<eme::IEME, decltype(&eme::IEME::destroy)> ieme_;

  bool InitializeSession();
};

ScopedHandleAndSize AdoptHandle(handle_and_size_s* handle_and_size);

void ClearHandle(handle_and_size_s* handle_and_size);

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_TIZEN_DRM_SESSION_H_
