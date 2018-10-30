// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <set>
#include <string>

#include "common/gin_native_bridge_errors.h"
#include "content/common/content_export.h"
#include "ipc/ipc_message_macros.h"

#undef IPC_MESSAGE_EXPORT
#define IPC_MESSAGE_EXPORT CONTENT_EXPORT
#define IPC_MESSAGE_START GinNativeBridgeMsgStart

// Messages for handling Java objects injected into JavaScript -----------------

IPC_ENUM_TRAITS_MAX_VALUE(content::GinNativeBridgeError,  // LCOV_EXCL_LINE
                          content::kGinNativeBridgeErrorLast)

// Sent from browser to renderer to add a Native object with the given name.
// Object IDs are generated on the browser side.
IPC_MESSAGE_ROUTED2(GinNativeBridgeMsg_AddNamedObject,
                    std::string /* name */,
                    int32_t /* object_id */)

// Sent from browser to renderer to remove a Java object with the given name.
IPC_MESSAGE_ROUTED1(GinNativeBridgeMsg_RemoveNamedObject,
                    std::string /* name */)

// Sent from renderer to browser to get information about methods of
// the given object. The query will only succeed if inspection of injected
// objects is enabled on the browser side.
IPC_SYNC_MESSAGE_ROUTED1_1(GinNativeBridgeHostMsg_GetMethods,
                           int32_t /* object_id */,
                           std::set<std::string> /* returned_method_names */)

// Sent from renderer to browser to find out, if an object has a method with
// the given name.
IPC_SYNC_MESSAGE_ROUTED2_1(GinNativeBridgeHostMsg_HasMethod,
                           int32_t /* object_id */,
                           std::string /* method_name */,
                           bool /* result */)

// Sent from renderer to browser to invoke a method. Method arguments
// are chained into |arguments| list. base::ListValue is used for |result| as
// a container to work around immutability of base::Value.
// Empty result list indicates that an error has happened on the Native side
// (either bridge-induced error or an unhandled Native exception) and an
// exception must be thrown into JavaScript. |error_code| indicates the cause of
// the error.
// Some special value types that are not supported by base::Value are encoded
// as BinaryValues via GinNativeBridgeValue.
IPC_SYNC_MESSAGE_ROUTED3_2(GinNativeBridgeHostMsg_InvokeMethod,
                           int32_t /* object_id */,
                           std::string /* method_name */,
                           base::ListValue /* arguments */,
                           base::ListValue /* result */,
                           content::GinNativeBridgeError /* error_code */)
