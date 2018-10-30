// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_tizen_drm_manager.h"

#include <utility>

#include "base/logging.h"
#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_tizen_player_dispatcher.h"
#include "ppapi/c/pp_errors.h"

namespace content {

PepperTizenDRMListener::~PepperTizenDRMListener() = default;

PepperTizenDRMManager::PepperTizenDRMManager(
    scoped_refptr<PepperTizenDRMListener> drm_listener)
    : drm_listener_(std::move(drm_listener)) {}

PepperTizenDRMManager::~PepperTizenDRMManager() = default;

scoped_refptr<PepperTizenDRMSession> PepperTizenDRMManager::GetSession(
    PP_MediaPlayerDRMType drm_type,
    const std::string& init_data) {
  auto session = PepperTizenDRMSession::Create(drm_type, this);
  if (!session)
    return nullptr;

  auto iter = sessions_.find(session->session_id());
  // check if weak pointer contains valid session
  if (iter != sessions_.end() && iter->second.get()) {
    LOG(ERROR) << "Duplicate session id: " << session->session_id();
    return nullptr;
  }

  sessions_[session->session_id()] = session->AsWeakPtr();

  if (!session->Initialize(init_data)) {
    sessions_.erase(session->session_id());
    return nullptr;
  }
  return session;
}

bool PepperTizenDRMManager::InstallLicense(const std::string& response) {
  auto session = GetPendingLicenceRequest();
  if (!session) {
    LOG(ERROR) << "No pending licence requests, cannot install a license.";
    return false;
  }
  auto result = session->InstallLicense(response);
  ProcessNextPendingLicenceRequest();
  return result;
}

PepperTizenDRMSession* PepperTizenDRMManager::GetPendingLicenceRequest() {
  base::AutoLock pending_licence_requests_lock(pending_licence_requests_lock_);
  if (pending_licence_requests_.empty())
    return nullptr;

  auto request = pending_licence_requests_.front();
  pending_licence_requests_.pop();
  auto session_id = request.first;
  auto iter = sessions_.find(session_id);
  if (iter == sessions_.end()) {
    LOG(WARNING) << "unknown session id == " << session_id;
    return nullptr;
  }

  return iter->second.get();
}

void PepperTizenDRMManager::ProcessNextPendingLicenceRequest() {
  DCHECK(drm_listener_);

  base::AutoLock pending_licence_requests_lock(pending_licence_requests_lock_);
  if (!pending_licence_requests_.empty()) {
    auto request = pending_licence_requests_.front();
    drm_listener_->OnLicenseRequest(request.second.size(),
                                    request.second.data());
  }
}

void PepperTizenDRMManager::EnqueueLicenceRequest(const std::string& session_id,
                                                  const std::string& message) {
  DCHECK(drm_listener_);

  base::AutoLock pending_licence_requests_lock(pending_licence_requests_lock_);
  pending_licence_requests_.push(std::make_pair(session_id, message));
  // Only our new request exists, so we can process it now
  if (pending_licence_requests_.size() == 1)
    drm_listener_->OnLicenseRequest(message.size(), message.data());
}

// eme::IEventListener implementation
void PepperTizenDRMManager::onMessage(const std::string& session_id,
                                      eme::MessageType message_type,
                                      const std::string& message) {
  if (sessions_.find(session_id) == sessions_.end()) {
    LOG(WARNING) << "unknown session id == " << session_id;
    return;
  }

  switch (message_type) {
    case eme::kLicenseRequest:
    case eme::kIndividualizationRequest:
      LOG(INFO) << "Requesting a license for " << session_id << "...";
      EnqueueLicenceRequest(session_id, message);
      break;
    default:
      LOG(WARNING) << "Received an unsupported message == " << message_type;
  }
}

// eme::IEventListener implementation
void PepperTizenDRMManager::onKeyStatusesChange(const std::string& session_id) {
  LOG(WARNING) << "Not implemented";
}

// eme::IEventListener implementation
void PepperTizenDRMManager::onRemoveComplete(const std::string& session_id) {
  LOG(WARNING) << "Not implemented";
}

}  // namespace content
