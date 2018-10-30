// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/cpp/samsung/extension_system_samsung_wrt.h"

namespace {
const char kWrtEmbedderName[] = "TizenWRT";
}  // namespace

namespace pp {
ExtensionSystemSamsungWRT::ExtensionSystemSamsungWRT(
    const InstanceHandle& instance)
    : ExtensionSystemSamsungTizen(instance) {}

ExtensionSystemSamsungWRT::~ExtensionSystemSamsungWRT() {}

bool ExtensionSystemSamsungWRT::IsValid() const {
  static bool is_valid = (GetEmbedderName() == kWrtEmbedderName);
  return is_valid;
}
}  // namespace pp
