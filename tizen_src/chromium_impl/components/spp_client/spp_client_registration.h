// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMIUM_IMPL_COMPONENTS_SPP_CLIENT_SPP_CLIENT_REGISTRATION_H_
#define CHROMIUM_IMPL_COMPONENTS_SPP_CLIENT_SPP_CLIENT_REGISTRATION_H_

#include "components/spp_client/proto/spp_client_data.pb.h"
#include "components/spp_client/spp_client.h"

#include <push-service.h>
#include <string>

namespace sppclient {

class SppClientImpl;

class SppClientRegistration {
 public:
  SppClientRegistration(const std::string& sender_id, SppClientImpl* client);
  ~SppClientRegistration();

  void SetAppID(const std::string* app_id);
  const std::string& GetAppID() const { return *app_id_; }

  const std::string& GetSenderID() const { return sender_id_; }

  void SetEncryptionData(const EncryptionData& encryption_data);
  const EncryptionData& GetEncryptionData();

  std::string GetRegistrationID() const;

  bool Connect(const SppClient::RegisterCallback& callback);
  void Disconnect(const SppClient::UnregisterCallback& callback);

 private:
  static void OnStateChange(push_service_state_e state,
                            const char* err,
                            void* user_data);
  static void OnNotify(push_service_notification_h noti, void* user_data);

  void HandleUnregisteredState();
  void HandleRegisteredState();
  void HandleErrorState(const char* err);

  static void RegisterResultCallback(push_service_result_e result,
                                     const char* msg,
                                     void* user_data);
  static void UnregisterResultCallback(push_service_result_e result,
                                       const char* msg,
                                       void* user_data);

  const std::string* app_id_;
  std::string sender_id_;
  EncryptionData encryption_data_;
  push_service_connection_h connection_;
  SppClient::RegisterCallback reg_callback_;
  SppClient::UnregisterCallback unreg_callback_;
  SppClientImpl* client_;
};

}  // namespace sppclient

#endif  // CHROMIUM_IMPL_COMPONENTS_SPP_CLIENT_SPP_CLIENT_REGISTRATION_H_
