// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMIUM_IMPL_CONTENT_PUSH_MESSAGING_SPP_CLIENT_IMPL_H_
#define CHROMIUM_IMPL_CONTENT_PUSH_MESSAGING_SPP_CLIENT_IMPL_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/spp_client/proto/spp_client_data.pb.h"
#include "components/spp_client/spp_client.h"
#include "components/spp_client/spp_client_registration.h"

#include <map>
#include <string>

namespace leveldb_proto {
template <typename T>
class ProtoDatabase;
}

namespace sppclient {

class SppClientImpl : public SppClient {
 public:
  explicit SppClientImpl(
      const base::FilePath& path,
      const scoped_refptr<base::SequencedTaskRunner>& task_runner,
      Delegate* delegate);

  void OnMessage(const SppClientRegistration& registration,
                 const std::string& data);

 private:
  void InitializeDB();
  void DidInitializeDB(bool success);
  void DidLoadKeys(bool success,
                   std::unique_ptr<std::vector<std::string>> keys);
  void DidLoadKeysAndRegistrations(
      std::unique_ptr<std::vector<std::string>> keys,
      bool success,
      std::unique_ptr<std::vector<RegistrationData>> entries);
  void DidReregister(const std::string& app_id,
                     const std::string& prev_registration_id_hash,
                     const EncryptionData& encryption_data,
                     const base::Closure& barrier_closure,
                     const std::string& registration_id,
                     PushRegistrationResult result);
  void DoDelayedTasks();

  void Register(const std::string& app_id,
                const std::string& sender_id,
                const RegisterCallback& callback) override;
  void DoRegister(const std::string& app_id,
                  const std::string& sender_id,
                  const RegisterCallback& callback);
  void DidRegister(const std::string& app_id,
                   const RegisterCallback& callback,
                   const std::string& registration_id,
                   PushRegistrationResult result);
  void StoreRegistration(const std::string& app_id,
                         const std::string& registration_id,
                         const RegistrationData& registration_data,
                         const RegisterCallback& callback);
  void DidStoreRegistration(const RegisterCallback& callback,
                            const std::string& registration_id,
                            bool success);

  void Unregister(const std::string& app_id,
                  const UnregisterCallback& callback) override;
  void DidUnregister(const std::string& app_id,
                     const UnregisterCallback& callback,
                     PushRegistrationResult result);
  void RemoveRegistration(const std::string& app_id,
                          const UnregisterCallback& callback);
  void DidRemoveRegistration(const UnregisterCallback& callback, bool success);

  void GetEncryptionInfo(const std::string& app_id,
                         const GetEncryptionInfoCallback& callback) override;

  void ForEachRegistration(const ForEachCallback& callback,
                           const base::Closure& done_closure) override;

  void AddAppHandler(
      const char* app_identifier,
      content::PushMessagingService* push_messaging_service) override;
  void RemoveAppHandler(const char* app_identifier) override;

  bool ready_;

  base::FilePath database_path_;
  std::unique_ptr<leveldb_proto::ProtoDatabase<RegistrationData>> database_;

  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  std::vector<base::Closure> delayed_tasks_;

  using SppClientRegistrationMap = std::map<std::string, SppClientRegistration>;
  SppClientRegistrationMap registrations_;

  base::WeakPtrFactory<SppClientImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(SppClientImpl);
};

}  // namespace sppclient

#endif  // CHROMIUM_IMPL_CONTENT_PUSH_MESSAGING_SPP_CLIENT_IMPL_H_
