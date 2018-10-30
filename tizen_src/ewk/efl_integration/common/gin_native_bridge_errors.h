// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_EFL_INTEGRATION_COMMON_GIN_NATIVE_BRIDGE_ERRORS_H_
#define EWK_EFL_INTEGRATION_COMMON_GIN_NATIVE_BRIDGE_ERRORS_H_

#include "content/common/content_export.h"

namespace content {

enum GinNativeBridgeError {
  kGinNativeBridgeNoError = 0,
  kGinNativeBridgeUnknownObjectId,
  kGinNativeBridgeObjectIsGone,
  kGinNativeBridgeMethodNotFound,
  kGinNativeBridgeAccessToObjectGetClassIsBlocked,
  kGinNativeBridgeNativeExceptionRaised,
  kGinNativeBridgeNonAssignableTypes,
  kGinNativeBridgeRenderFrameDeleted,
  kGinNativeBridgeNotSupportedTypes,
  kGinNativeBridgeMessageNameIsWrong,
  kGinNativeBridgeErrorLast = kGinNativeBridgeRenderFrameDeleted
};

CONTENT_EXPORT const char* GinNativeBridgeErrorToString(
    GinNativeBridgeError error);

}  // namespace content

#endif  // EWK_EFL_INTEGRATION_COMMON_GIN_NATIVE_BRIDGE_ERRORS_H_
