// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/spp_client/fake_spp_client_impl.h"

#include "base/compiler_specific.h"
#include "base/logging.h"

namespace sppclient {

// static
SppClient* SppClient::Create(
    const base::FilePath& path,
    const scoped_refptr<base::SequencedTaskRunner>& task_runner,
    SppClient::Delegate* delegate) {
  return new FakeSppClientImpl(delegate);
}

// static
std::string SppClient::GetPushEndpointForRegId(const std::string& reg_id) {
  return std::string();
}

FakeSppClientImpl::FakeSppClientImpl(Delegate* delegate)
    : SppClient(delegate) {}

FakeSppClientImpl::~FakeSppClientImpl() {
  timer_.Stop();
}

void FakeSppClientImpl::OnMessageReceived(std::string app_id) {
  if (delegate_)
    delegate_->OnMessage(app_id, std::string(), std::string());
}

void FakeSppClientImpl::Register(const std::string& app_id,
                                 const std::string& sender_id,
                                 const RegisterCallback& callback) {
  std::string fake_reg_id("0412345678");
  callback.Run(fake_reg_id, PushRegistrationResult::SUCCESS);

  // every 15 seconds trigger fake push message for test purposes
  timer_.Start(FROM_HERE, base::TimeDelta::FromSeconds(15),
               base::Bind(&FakeSppClientImpl::OnMessageReceived,
                          base::Unretained(this), app_id));
}

void FakeSppClientImpl::Unregister(const std::string& app_id,
                                   const UnregisterCallback& callback) {
  timer_.Stop();
  callback.Run(PushRegistrationResult::SUCCESS);
}

void FakeSppClientImpl::GetEncryptionInfo(
    const std::string& app_id,
    const GetEncryptionInfoCallback& callback) {
  NOTIMPLEMENTED();
  callback.Run(std::string(), std::string(), PushRegistrationResult::ERROR);
}

void FakeSppClientImpl::ForEachRegistration(const ForEachCallback& callback,
                                            const base::Closure& done_closure) {
  NOTIMPLEMENTED();
  done_closure.Run();
}

void FakeSppClientImpl::AddAppHandler(
    const char* app_identifier,
    content::PushMessagingService* push_messaging_service) {
  NOTIMPLEMENTED();
}

void FakeSppClientImpl::RemoveAppHandler(const char* app_identifier) {
  NOTIMPLEMENTED();
}

}  // namespace sppclient
