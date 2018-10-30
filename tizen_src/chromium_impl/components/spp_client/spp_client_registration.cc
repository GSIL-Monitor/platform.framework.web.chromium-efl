// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/spp_client/spp_client_registration.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/free_deleter.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/spp_client/spp_client_encryption_util.h"
#include "components/spp_client/spp_client_impl.h"
#include "components/spp_client/spp_client_registration.h"

namespace sppclient {

SppClientRegistration::SppClientRegistration(const std::string& sender_id,
                                             SppClientImpl* client)
    : app_id_(nullptr),
      sender_id_(sender_id),
      connection_(nullptr),
      client_(client) {}

SppClientRegistration::~SppClientRegistration() {
  if (connection_)
    push_service_disconnect(connection_);
}

void SppClientRegistration::SetAppID(const std::string* app_id) {
  app_id_ = app_id;
}

void SppClientRegistration::SetEncryptionData(
    const EncryptionData& encryption_data) {
  DCHECK(!encryption_data_.IsInitialized());
  encryption_data_ = encryption_data;
}

const EncryptionData& SppClientRegistration::GetEncryptionData() {
  if (!encryption_data_.IsInitialized())
    CreateEncryptionData(encryption_data_);
  return encryption_data_;
}

std::string SppClientRegistration::GetRegistrationID() const {
  char* buffer;
  if (!connection_ || push_service_get_registration_id(connection_, &buffer) !=
                          PUSH_SERVICE_ERROR_NONE)
    return std::string();

  std::unique_ptr<char, base::FreeDeleter> reg_id(buffer);

  return std::string(buffer);
}

bool SppClientRegistration::Connect(
    const SppClient::RegisterCallback& callback) {
  if (push_service_connect(sender_id_.c_str(), OnStateChange, OnNotify, this,
                           &connection_) != PUSH_SERVICE_ERROR_NONE)
    return false;

  reg_callback_ = callback;
  return true;
}

void SppClientRegistration::Disconnect(
    const SppClient::UnregisterCallback& callback) {
  unreg_callback_ = callback;

  if (push_service_deregister(connection_, UnregisterResultCallback, this) !=
      PUSH_SERVICE_ERROR_NONE)
    HandleUnregisteredState();
}

// static
void SppClientRegistration::OnStateChange(push_service_state_e state,
                                          const char* err,
                                          void* user_data) {
  SppClientRegistration* registration =
      static_cast<SppClientRegistration*>(user_data);
  switch (state) {
    case PUSH_SERVICE_STATE_UNREGISTERED:
      registration->HandleUnregisteredState();
      break;
    case PUSH_SERVICE_STATE_REGISTERED:
      registration->HandleRegisteredState();
      break;
    case PUSH_SERVICE_STATE_ERROR:
      registration->HandleErrorState(err);
      break;
    default:
      break;
  }
}

// static
void SppClientRegistration::OnNotify(push_service_notification_h noti,
                                     void* user_data) {
  SppClientRegistration* registration =
      static_cast<SppClientRegistration*>(user_data);

  char* buffer;
  int result = push_service_get_notification_data(noti, &buffer);
  if (result != PUSH_SERVICE_ERROR_NONE && result != PUSH_SERVICE_ERROR_NO_DATA)
    return;

  std::unique_ptr<char, base::FreeDeleter> data(buffer);

  registration->client_->OnMessage(
      *registration, buffer ? std::string(buffer) : std::string());
}

void SppClientRegistration::HandleUnregisteredState() {
  if (unreg_callback_.is_null()) {
    push_service_register(connection_, RegisterResultCallback, this);
  } else {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::Bind(unreg_callback_, PushRegistrationResult::SUCCESS));
  }
}

void SppClientRegistration::HandleRegisteredState() {
  std::string registration_id = GetRegistrationID();

  if (registration_id.empty()) {
    reg_callback_.Run(registration_id, PushRegistrationResult::ERROR);
    return;
  }

  reg_callback_.Run(registration_id, PushRegistrationResult::SUCCESS);

  // ask for any waiting notifications
  push_service_request_unread_notification(connection_);
}

void SppClientRegistration::HandleErrorState(const char* err) {
  LOG(ERROR) << "ERROR state for app_id: " << *app_id_ << " - message: " << err;
}

// static
void SppClientRegistration::RegisterResultCallback(push_service_result_e result,
                                                   const char* msg,
                                                   void* user_data) {
  if (result == PUSH_SERVICE_RESULT_SUCCESS)
    return;

  SppClientRegistration* registration =
      static_cast<SppClientRegistration*>(user_data);
  registration->reg_callback_.Run(std::string(), PushRegistrationResult::ERROR);
}

// static
void SppClientRegistration::UnregisterResultCallback(
    push_service_result_e result,
    const char* msg,
    void* user_data) {
  SppClientRegistration* registration =
      static_cast<SppClientRegistration*>(user_data);

  if (result != PUSH_SERVICE_RESULT_SUCCESS)
    registration->HandleUnregisteredState();
}

}  // namespace sppclient
