// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMIUM_IMPL_CONTENT_PUSH_MESSAGING_SPP_CLIENT_H_
#define CHROMIUM_IMPL_CONTENT_PUSH_MESSAGING_SPP_CLIENT_H_

#include <string>

#include "base/callback.h"
#include "base/files/file_path.h"

namespace base {
class SequencedTaskRunner;
}

namespace content {
class PushMessagingService;
}

namespace sppclient {

// FIXME: consider defining another enum for unregister.
enum class PushRegistrationResult {
  SUCCESS,
  REGISTRATION_LIMIT_REACHED,
  NOT_REGISTERED,
  ERROR,
  // FIXME: Add different types of error.
};

class SppClient {
 public:
  class Delegate {
   public:
    virtual void OnMessage(const std::string& app_id,
                           const std::string& sender_id,
                           const std::string& data) = 0;
  };

  static SppClient* Create(
      const base::FilePath& path,
      const scoped_refptr<base::SequencedTaskRunner>& task_runner,
      SppClient::Delegate* delegate);

  static std::string GetPushEndpointForRegId(const std::string& reg_id);

  using RegisterCallback =
      base::Callback<void(const std::string& registration_id,
                          PushRegistrationResult result)>;
  using UnregisterCallback =
      base::Callback<void(PushRegistrationResult result)>;
  using GetEncryptionInfoCallback =
      base::Callback<void(const std::string& p256dh,
                          const std::string& auth_secret,
                          PushRegistrationResult result)>;
  using ForEachCallback =
      base::Callback<void(const std::string& app_id,
                          const base::Closure& barrier_closure)>;

  explicit SppClient(Delegate* delegate) : delegate_(delegate) {}
  virtual ~SppClient() {}

  virtual void Register(const std::string& app_id,
                        const std::string& sender_id,
                        const RegisterCallback& callback) = 0;
  virtual void Unregister(const std::string& app_id,
                          const UnregisterCallback& callback) = 0;

  virtual void GetEncryptionInfo(const std::string& app_id,
                                 const GetEncryptionInfoCallback& callback) = 0;

  virtual void ForEachRegistration(const ForEachCallback& callback,
                                   const base::Closure& done_closure) = 0;

  virtual void AddAppHandler(
      const char* app_identifier,
      content::PushMessagingService* push_messaging_service) = 0;
  virtual void RemoveAppHandler(const char* app_identifier) = 0;

 protected:
  Delegate* delegate_;
};

}  // namespace sppclient

#endif  // CHROMIUM_IMPL_CONTENT_PUSH_MESSAGING_SPP_CLIENT_H_
