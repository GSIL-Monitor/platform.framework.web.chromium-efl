// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_TIZEN_DRM_MANAGER_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_TIZEN_DRM_MANAGER_H_

#include <emeCDM/IEME.h>

#include <memory>
#include <queue>
#include <unordered_map>

#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_tizen_drm_session.h"

#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/synchronization/lock.h"
#include "ppapi/c/samsung/pp_media_common_samsung.h"

namespace content {

class PepperTizenDRMListener
    : public base::RefCountedThreadSafe<PepperTizenDRMListener> {
 public:
  virtual ~PepperTizenDRMListener();
  virtual void OnLicenseRequest(uint32_t size, const void* request) = 0;
};

// Class managing DRM sessions. Be weary that while this manager is not
// associated with any particular thread, it should be used from one thread
// (due to underlying WeakPtrs).
class PepperTizenDRMManager : public eme::IEventListener {
 public:
  explicit PepperTizenDRMManager(scoped_refptr<PepperTizenDRMListener>);

  ~PepperTizenDRMManager();

  // Get a session for the given DRM system and initialize it with the given
  // initialization data.
  //
  // Returns empty pointer if session cannot be created.
  scoped_refptr<PepperTizenDRMSession> GetSession(PP_MediaPlayerDRMType,
                                                  const std::string& init_data);

  bool InstallLicense(const std::string& response);

  // eme::IEventListener implementation
  void onMessage(const std::string& session_id,
                 eme::MessageType message_type,
                 const std::string& message) override;

  // eme::IEventListener implementation
  void onKeyStatusesChange(const std::string& session_id) override;

  // eme::IEventListener implementation
  void onRemoveComplete(const std::string& session_id) override;

 private:
  typedef std::pair<std::string, std::string> LicenceRequest;

  scoped_refptr<PepperTizenDRMListener> drm_listener_;
  std::queue<LicenceRequest> pending_licence_requests_;
  base::Lock pending_licence_requests_lock_;
  std::unordered_map<std::string, base::WeakPtr<PepperTizenDRMSession>>
      sessions_;

  void EnqueueLicenceRequest(const std::string& session_id,
                             const std::string& message);
  void ProcessNextPendingLicenceRequest();
  PepperTizenDRMSession* GetPendingLicenceRequest();

  DISALLOW_COPY_AND_ASSIGN(PepperTizenDRMManager);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_TIZEN_DRM_MANAGER_H_
