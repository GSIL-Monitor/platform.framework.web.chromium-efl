// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/gin_native_bridge_errors.h"

#include "base/logging.h"

namespace content {

const char* GinNativeBridgeErrorToString(GinNativeBridgeError error) {
  switch (error) {
    case kGinNativeBridgeNoError:
      return "No error";
    case kGinNativeBridgeUnknownObjectId:
      return "Unknown Native object ID";
    case kGinNativeBridgeObjectIsGone:
      return "Native object is gone";
    case kGinNativeBridgeMethodNotFound:
      return "Method not found";
    case kGinNativeBridgeAccessToObjectGetClassIsBlocked:
      return "Access to Object.getClass is blocked";
    case kGinNativeBridgeNativeExceptionRaised:
      return "Native exception was raised during method invocation";
    case kGinNativeBridgeNonAssignableTypes:
      return "The type of the object passed to the method is incompatible "
             "with the type of method's argument";
    case kGinNativeBridgeRenderFrameDeleted:
      return "RenderFrame has been deleted";
    case kGinNativeBridgeNotSupportedTypes:
      return "This type is not supported";
    case kGinNativeBridgeMessageNameIsWrong:
      return "Message name is wrong";
  }
  NOTREACHED();
  return "Unknown error";
}

}  // namespace content
