// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMIUM_IMPL_COMPONENTS_SPP_CLIENT_SPP_CLIENT_ENCRYPTION_UTIL_H_
#define CHROMIUM_IMPL_COMPONENTS_SPP_CLIENT_SPP_CLIENT_ENCRYPTION_UTIL_H_

#include "components/spp_client/proto/spp_client_data.pb.h"

namespace sppclient {

bool CreateEncryptionData(EncryptionData& encryption_data);

}  // namespace sppclient

#endif  // CHROMIUM_IMPL_COMPONENTS_SPP_CLIENT_SPP_CLIENT_ENCRYPTION_UTIL_H_
