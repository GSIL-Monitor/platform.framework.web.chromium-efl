// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMIUM_IMPL_CONTENT_PUSH_MESSAGING_FAKE_SPP_CLIENT_IMPL_H_
#define CHROMIUM_IMPL_CONTENT_PUSH_MESSAGING_FAKE_SPP_CLIENT_IMPL_H_

#include <string>

#include "base/macros.h"
#include "base/timer/timer.h"
#include "components/spp_client/spp_client.h"

namespace sppclient {

class FakeSppClientImpl : public SppClient {
 public:
  explicit FakeSppClientImpl(Delegate* delegate);
  ~FakeSppClientImpl() override;

  void Register(const std::string& app_id,
                const std::string& sender_id,
                const RegisterCallback& callback) override;
  void Unregister(const std::string& app_id,
                  const UnregisterCallback& callback) override;

  void GetEncryptionInfo(const std::string& app_id,
                         const GetEncryptionInfoCallback& callback) override;

  void ForEachRegistration(const ForEachCallback& callback,
                           const base::Closure& done_closure) override;

  void AddAppHandler(
      const char* app_identifier,
      content::PushMessagingService* push_messaging_service) override;
  void RemoveAppHandler(const char* app_identifier) override;

 private:
  base::RepeatingTimer timer_;

  void OnMessageReceived(std::string app_id);

  DISALLOW_COPY_AND_ASSIGN(FakeSppClientImpl);
};

}  // namespace sppclient

#endif  // CHROMIUM_IMPL_CONTENT_PUSH_MESSAGING_FAKE_SPP_CLIENT_IMPL_H_
