// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/spp_client/spp_client_impl.h"

#include "base/barrier_closure.h"
#include "base/files/file_util.h"
#include "base/sha1.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/leveldb_proto/proto_database_impl.h"

namespace sppclient {

const int kMaxRegistrations = 1000000;
const base::FilePath::CharType kSPPStoreDirname[] = FILE_PATH_LITERAL("SPP");

// static
SppClient* SppClient::Create(
    const base::FilePath& path,
    const scoped_refptr<base::SequencedTaskRunner>& task_runner,
    SppClient::Delegate* delegate) {
  return new SppClientImpl(path, task_runner, delegate);
}

// static
std::string SppClient::GetPushEndpointForRegId(const std::string& reg_id) {
  if (!reg_id.compare(0, 2, "00"))
    return std::string(
        "https://useast.push.samsungosp.com:8090"
        "/spp/pns/api/push/");
  if (!reg_id.compare(0, 2, "02"))
    return std::string(
        "https://apsoutheast.push.samsungosp.com:8090"
        "/spp/pns/api/push/");
  if (!reg_id.compare(0, 2, "03"))
    return std::string(
        "https://euwest.push.samsungosp.com:8090"
        "/spp/pns/api/push/");
  if (!reg_id.compare(0, 2, "04"))
    return std::string(
        "https://apnortheast.push.samsungosp.com:8090"
        "/spp/pns/api/push/");
  if (!reg_id.compare(0, 2, "05"))
    return std::string(
        "https://apkorea.push.samsungosp.com:8090"
        "/spp/pns/api/push/");
  if (!reg_id.compare(0, 2, "06"))
    return std::string(
        "https://apchina.push.samsungosp.com.cn:8090"
        "/spp/pns/api/push/");
  if (!reg_id.compare(0, 2, "50"))
    return std::string(
        "https://useast.gateway.push.samsungosp.com:8090"
        "/spp/pns/api/push/");
  if (!reg_id.compare(0, 2, "52"))
    return std::string(
        "https://apsoutheast.gateway.push.samsungosp.com:8090"
        "/spp/pns/api/push/");
  if (!reg_id.compare(0, 2, "53"))
    return std::string(
        "https://euwest.gateway.push.samsungosp.com:8090"
        "/spp/pns/api/push/");
  if (!reg_id.compare(0, 2, "54"))
    return std::string(
        "https://apnortheast.gateway.push.samsungosp.com:8090"
        "/spp/pns/api/push/");
  if (!reg_id.compare(0, 2, "55"))
    return std::string(
        "https://apkorea.gateway.push.samsungosp.com:8090"
        "/spp/pns/api/push/");
  if (!reg_id.compare(0, 2, "56"))
    return std::string(
        "https://apchina.gateway.push.samsungosp.com.cn:8090"
        "/spp/pns/api/push/");

  return std::string();
}

SppClientImpl::SppClientImpl(
    const base::FilePath& path,
    const scoped_refptr<base::SequencedTaskRunner>& task_runner,
    Delegate* delegate)
    : SppClient(delegate),
      ready_(false),
      database_path_(path),
      task_runner_(task_runner),
      weak_ptr_factory_(this) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&SppClientImpl::InitializeDB, weak_ptr_factory_.GetWeakPtr()));
}

void SppClientImpl::InitializeDB() {
  if (!base::DirectoryExists(database_path_)) {
    LOG(WARNING) << "DB path is not exist. " << database_path_.AsUTF8Unsafe();
    ready_ = true;
    DoDelayedTasks();
    return;
  }

  database_.reset(
      new leveldb_proto::ProtoDatabaseImpl<RegistrationData>(task_runner_));

  database_->Init("SppClient", database_path_.Append(kSPPStoreDirname),
                  leveldb_proto::CreateSimpleOptions(),
                  base::Bind(&SppClientImpl::DidInitializeDB,
                             weak_ptr_factory_.GetWeakPtr()));
}

void SppClientImpl::DidInitializeDB(bool success) {
  if (!success || ready_) {
    DidLoadKeysAndRegistrations(nullptr, success ? true : false, nullptr);
    return;
  }

  database_->LoadKeys(
      base::Bind(&SppClientImpl::DidLoadKeys, weak_ptr_factory_.GetWeakPtr()));
}

void SppClientImpl::DidLoadKeys(
    bool success,
    std::unique_ptr<std::vector<std::string>> keys) {
  if (success) {
    database_->LoadEntries(
        base::Bind(&SppClientImpl::DidLoadKeysAndRegistrations,
                   weak_ptr_factory_.GetWeakPtr(), base::Passed(&keys)));
  } else
    DidLoadKeysAndRegistrations(nullptr, false, nullptr);
}

void SppClientImpl::DidLoadKeysAndRegistrations(
    std::unique_ptr<std::vector<std::string>> keys,
    bool success,
    std::unique_ptr<std::vector<RegistrationData>> entries) {
  ready_ = true;

  if (keys && entries) {
    DCHECK_EQ(keys->size(), entries->size());
    std::vector<std::string>::const_iterator it = keys->begin();

    base::Closure barrier_closure = base::BarrierClosure(
        entries->size(), base::Bind(&SppClientImpl::DoDelayedTasks,
                                    weak_ptr_factory_.GetWeakPtr()));

    for (const RegistrationData& entry : *entries) {
      DCHECK_EQ(1, entry.encryption_data().keys_size());

      std::string app_id = *it++;
      DoRegister(app_id, entry.sender_id(),
                 base::Bind(&SppClientImpl::DidReregister,
                            weak_ptr_factory_.GetWeakPtr(), app_id,
                            entry.registration_id_hash(),
                            entry.encryption_data(), barrier_closure));
    }
  } else
    DoDelayedTasks();
  if (!success)
    database_.reset();
}

void DidUnregisterInternal(const base::Closure& barrier_closure,
                           sppclient::PushRegistrationResult result) {
  barrier_closure.Run();
}

void SppClientImpl::DidReregister(const std::string& app_id,
                                  const std::string& prev_registration_id_hash,
                                  const EncryptionData& encryption_data,
                                  const base::Closure& barrier_closure,
                                  const std::string& registration_id,
                                  sppclient::PushRegistrationResult result) {
  if (result != PushRegistrationResult::SUCCESS) {
    barrier_closure.Run();
    return;
  }

  if (prev_registration_id_hash != base::SHA1HashString(registration_id)) {
    Unregister(app_id, base::Bind(&DidUnregisterInternal, barrier_closure));
    return;
  }

  SppClientRegistrationMap::iterator iter = registrations_.find(app_id);
  DCHECK(iter != registrations_.end());

  SppClientRegistration& registration = iter->second;
  registration.SetEncryptionData(encryption_data);

  barrier_closure.Run();
}

void SppClientImpl::DoDelayedTasks() {
  for (size_t i = 0; i < delayed_tasks_.size(); ++i)
    delayed_tasks_[i].Run();
  delayed_tasks_.clear();
}

void SppClientImpl::Register(const std::string& app_id,
                             const std::string& sender_id,
                             const RegisterCallback& callback) {
  if (!ready_) {
    delayed_tasks_.push_back(base::Bind(&SppClientImpl::Register,
                                        weak_ptr_factory_.GetWeakPtr(), app_id,
                                        sender_id, callback));
    return;
  }

  if (registrations_.size() >= kMaxRegistrations) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::Bind(callback, std::string(),
                   PushRegistrationResult::REGISTRATION_LIMIT_REACHED));
    return;
  }

  DoRegister(app_id, sender_id,
             base::Bind(&SppClientImpl::DidRegister,
                        weak_ptr_factory_.GetWeakPtr(), app_id, callback));
}

void SppClientImpl::DoRegister(const std::string& app_id,
                               const std::string& sender_id,
                               const RegisterCallback& callback) {
  std::pair<SppClientRegistrationMap::iterator, bool> ret =
      registrations_.emplace(app_id, SppClientRegistration(sender_id, this));
  SppClientRegistrationMap::iterator& iter = ret.first;
  SppClientRegistration& registration = iter->second;

  if (!ret.second) {
    std::string registration_id = registration.GetRegistrationID();
    if (!registration_id.empty()) {
      base::ThreadTaskRunnerHandle::Get()->PostTask(
          FROM_HERE, base::Bind(callback, registration_id,
                                PushRegistrationResult::SUCCESS));
      return;
    }
  }

  registration.SetAppID(&iter->first);
  if (!registration.Connect(callback)) {
    registrations_.erase(ret.first);
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::Bind(callback, std::string(), PushRegistrationResult::ERROR));
  }
}

void SppClientImpl::DidRegister(const std::string& app_id,
                                const RegisterCallback& callback,
                                const std::string& registration_id,
                                sppclient::PushRegistrationResult result) {
  if (result != PushRegistrationResult::SUCCESS) {
    callback.Run(std::string(), PushRegistrationResult::ERROR);
    return;
  }

  SppClientRegistrationMap::iterator iter = registrations_.find(app_id);
  if (iter == registrations_.end()) {
    callback.Run(std::string(), PushRegistrationResult::ERROR);
    return;
  }

  SppClientRegistration& registration = iter->second;
  if (!registration.GetEncryptionData().IsInitialized()) {
    callback.Run(std::string(), PushRegistrationResult::ERROR);
    return;
  }

  RegistrationData registration_data;
  registration_data.set_sender_id(registration.GetSenderID());
  registration_data.set_registration_id_hash(
      base::SHA1HashString(registration_id));

  EncryptionData* encryption_data =
      new EncryptionData(registration.GetEncryptionData());
  registration_data.set_allocated_encryption_data(encryption_data);

  if (database_) {
    StoreRegistration(registration.GetAppID(), registration_id,
                      registration_data, callback);
  } else {
    delayed_tasks_.push_back(base::Bind(
        &SppClientImpl::StoreRegistration, weak_ptr_factory_.GetWeakPtr(),
        registration.GetAppID(), registration_id, registration_data, callback));
    InitializeDB();
  }
}

void SppClientImpl::StoreRegistration(const std::string& app_id,
                                      const std::string& registration_id,
                                      const RegistrationData& registration_data,
                                      const RegisterCallback& callback) {
  using EntryVectorType =
      leveldb_proto::ProtoDatabase<RegistrationData>::KeyEntryVector;

  std::unique_ptr<EntryVectorType> entries_to_save(new EntryVectorType());
  std::unique_ptr<std::vector<std::string>> keys_to_remove(
      new std::vector<std::string>());

  entries_to_save->push_back(std::make_pair(app_id, registration_data));

  database_->UpdateEntries(
      std::move(entries_to_save), std::move(keys_to_remove),
      base::Bind(&SppClientImpl::DidStoreRegistration,
                 weak_ptr_factory_.GetWeakPtr(), callback, registration_id));
}

void SppClientImpl::DidStoreRegistration(const RegisterCallback& callback,
                                         const std::string& registration_id,
                                         bool success) {
  if (!success) {
    callback.Run(std::string(), PushRegistrationResult::ERROR);
    return;
  }

  callback.Run(registration_id, PushRegistrationResult::SUCCESS);
}

void SppClientImpl::Unregister(const std::string& app_id,
                               const UnregisterCallback& callback) {
  if (!ready_) {
    delayed_tasks_.push_back(base::Bind(&SppClientImpl::Unregister,
                                        weak_ptr_factory_.GetWeakPtr(), app_id,
                                        callback));
    return;
  }

  SppClientRegistrationMap::iterator iter = registrations_.find(app_id);
  if (iter == registrations_.end()) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::Bind(callback, PushRegistrationResult::NOT_REGISTERED));
    return;
  }

  SppClientRegistration& registration = iter->second;
  registration.Disconnect(base::Bind(&SppClientImpl::DidUnregister,
                                     weak_ptr_factory_.GetWeakPtr(), app_id,
                                     callback));
}

void SppClientImpl::DidUnregister(const std::string& app_id,
                                  const UnregisterCallback& callback,
                                  sppclient::PushRegistrationResult result) {
  registrations_.erase(registrations_.find(app_id));

  if (database_)
    RemoveRegistration(app_id, callback);
  else
    callback.Run(result);
}

void SppClientImpl::RemoveRegistration(const std::string& app_id,
                                       const UnregisterCallback& callback) {
  using EntryVectorType =
      leveldb_proto::ProtoDatabase<RegistrationData>::KeyEntryVector;

  std::unique_ptr<EntryVectorType> entries_to_save(new EntryVectorType());
  std::unique_ptr<std::vector<std::string>> keys_to_remove(
      new std::vector<std::string>());

  keys_to_remove->push_back(app_id);

  database_->UpdateEntries(
      std::move(entries_to_save), std::move(keys_to_remove),
      base::Bind(&SppClientImpl::DidRemoveRegistration,
                 weak_ptr_factory_.GetWeakPtr(), callback));
}

void SppClientImpl::DidRemoveRegistration(const UnregisterCallback& callback,
                                          bool success) {
  if (!success) {
    callback.Run(PushRegistrationResult::ERROR);
    return;
  }

  callback.Run(PushRegistrationResult::SUCCESS);
}

void SppClientImpl::GetEncryptionInfo(
    const std::string& app_id,
    const GetEncryptionInfoCallback& callback) {
  if (!ready_) {
    delayed_tasks_.push_back(base::Bind(&SppClientImpl::GetEncryptionInfo,
                                        weak_ptr_factory_.GetWeakPtr(), app_id,
                                        callback));
    return;
  }

  std::string p256dh;
  std::string auth_secret;
  SppClientRegistrationMap::iterator iter = registrations_.find(app_id);
  if (iter == registrations_.end()) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(callback, p256dh, auth_secret,
                              PushRegistrationResult::NOT_REGISTERED));
    return;
  }

  SppClientRegistration& registration = iter->second;
  const EncryptionData& encryption_data = registration.GetEncryptionData();

  const KeyPair& pair = encryption_data.keys(0);
  DCHECK_EQ(KeyPair::ECDH_P256, pair.type());
  p256dh = pair.public_key();

  auth_secret = encryption_data.auth_secret();

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::Bind(callback, p256dh, auth_secret,
                            PushRegistrationResult::SUCCESS));
}

void SppClientImpl::ForEachRegistration(const ForEachCallback& callback,
                                        const base::Closure& done_closure) {
  if (!ready_) {
    delayed_tasks_.push_back(base::Bind(&SppClientImpl::ForEachRegistration,
                                        weak_ptr_factory_.GetWeakPtr(),
                                        callback, done_closure));
    return;
  }

  base::Closure barrier_closure =
      base::BarrierClosure(registrations_.size(), done_closure);

  for (const auto& entry : registrations_) {
    const SppClientRegistration& registration = entry.second;
    callback.Run(registration.GetAppID(), barrier_closure);
  }
}

void SppClientImpl::OnMessage(const SppClientRegistration& registration,
                              const std::string& data) {
  delegate_->OnMessage(registration.GetAppID(), registration.GetSenderID(),
                       data);
}

void SppClientImpl::AddAppHandler(
    const char* app_identifier,
    content::PushMessagingService* push_messaging_service) {
  NOTIMPLEMENTED();
}

void SppClientImpl::RemoveAppHandler(const char* app_identifier) {
  NOTIMPLEMENTED();
}

}  // namespace sppclient
