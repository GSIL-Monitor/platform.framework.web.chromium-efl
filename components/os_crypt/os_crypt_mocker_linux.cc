// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/os_crypt/os_crypt_mocker_linux.h"

#include <memory>

#include "base/base64.h"
#include "base/lazy_instance.h"
#include "base/memory/ptr_util.h"
#include "base/rand_util.h"
#include "components/os_crypt/key_storage_config_linux.h"
#include "components/os_crypt/os_crypt.h"

namespace {

std::unique_ptr<KeyStorageLinux> CreateNewMock() {
  return base::MakeUnique<OSCryptMockerLinux>();
}

}

std::string OSCryptMockerLinux::GetKey() {
  return key_;
}

std::string* OSCryptMockerLinux::GetKeyPtr() {
  return &key_;
}

// static
void OSCryptMockerLinux::SetUp() {
  UseMockKeyStorageForTesting(
      &CreateNewMock, nullptr /* get the key from the provider above */);
  OSCrypt::SetConfig(base::MakeUnique<os_crypt::Config>());
}

// static
void OSCryptMockerLinux::TearDown() {
  UseMockKeyStorageForTesting(nullptr, nullptr);
  ClearCacheForTesting();
}

bool OSCryptMockerLinux::Init() {
  key_ = "the_encryption_key";
  return true;
}
