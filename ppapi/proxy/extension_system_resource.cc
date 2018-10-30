// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/extension_system_resource.h"

#include "base/json/json_string_value_serializer.h"
#include "base/logging.h"
#include "ppapi/proxy/plugin_dispatcher.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/proxy/raw_var_data.h"
#include "ppapi/proxy/serialized_var.h"
#include "ppapi/proxy/var_value_converter.h"
#include "ppapi/shared_impl/var.h"

namespace ppapi {
namespace proxy {

ExtensionSystemResource::ExtensionSystemResource(Connection connection,
                                                 PP_Instance instance)
    : PluginResource(connection, instance) {
  SendCreate(BROWSER, PpapiHostMsg_ExtensionSystem_Create());
}

ExtensionSystemResource::~ExtensionSystemResource() {}

thunk::PPB_ExtensionSystem_API*
ExtensionSystemResource::AsPPB_ExtensionSystem_API() {
  return this;
}

PP_Var ExtensionSystemResource::GetEmbedderName() {
  if (!embedder_name_.empty())
    return StringVar::StringToPPVar(embedder_name_);

  int32_t result =
      SyncCall<PpapiPluginMsg_ExtensionSystem_GetEmbedderNameReply>(
          BROWSER, PpapiHostMsg_ExtensionSystem_GetEmbedderName(),
          &embedder_name_);
  if (result != PP_OK)
    return PP_MakeUndefined();

  return StringVar::StringToPPVar(embedder_name_);
}

PP_Var ExtensionSystemResource::GetCurrentExtensionInfo() {
  if (current_extension_info_.get().type == PP_VARTYPE_UNDEFINED) {
    std::string json_result;

    int32_t sync_call_result =
        SyncCall<PpapiPluginMsg_ExtensionSystem_GetCurrentExtensionInfoReply>(
            BROWSER, PpapiHostMsg_ExtensionSystem_GetCurrentExtensionInfo(),
            &json_result);
    if (sync_call_result != PP_OK)
      return PP_MakeUndefined();

    base::StringPiece result_str(json_result);
    JSONStringValueDeserializer json_deserializer(result_str);
    std::unique_ptr<base::Value> result_ptr(
        json_deserializer.Deserialize(nullptr, nullptr));
    if (!result_ptr)
      return PP_MakeUndefined();

    ScopedPPVar var = VarFromValue(result_ptr.get());
    if (var.get().type != PP_VARTYPE_DICTIONARY)
      return PP_MakeUndefined();

    // cache the value of the extension info
    current_extension_info_ = var;
  }

  ScopedPPVar info_copy = current_extension_info_;
  return info_copy.Release();
}

int32_t ExtensionSystemResource::GenericSyncCall(PP_Var operation_name,
                                                 PP_Var operation_data,
                                                 PP_Var* operation_result) {
  if (!operation_result) {
    LOG(ERROR) << "Operation result is NULL";
    return PP_ERROR_BADARGUMENT;
  }
  StringVar* op_name = StringVar::FromPPVar(operation_name);
  if (!op_name) {
    LOG(ERROR) << "Invalid operation name";
    return PP_ERROR_BADARGUMENT;
  }
  if (op_name->value().empty()) {
    LOG(ERROR) << "Empty operation name";
    return PP_ERROR_BADARGUMENT;
  }

  // We do not support cycles in operation_data parameter.
  // We check it by trying to create RawVarDataGraph (will return NULL if
  // operation_data parameter does contain any cycles) because neither
  // JSONStringValueSerializer or ValueFromVar check for it.
  std::unique_ptr<RawVarDataGraph> cycle_check(
      RawVarDataGraph::Create(operation_data, pp_instance()));
  if (!cycle_check) {
    LOG(ERROR) << "Cycle detected in operation data";
    return PP_ERROR_BADARGUMENT;
  }

  std::string serialized_operation_data;
  JSONStringValueSerializer json_serializer(&serialized_operation_data);
  if (!json_serializer.Serialize(*(ValueFromVar(operation_data).get()))) {
    LOG(ERROR) << "JSON serialization failed";
    return PP_ERROR_BADARGUMENT;
  }

  std::string json_result;

  int32_t sync_call_result =
      SyncCall<PpapiPluginMsg_ExtensionSystem_GenericSyncCallReply>(
          BROWSER,
          PpapiHostMsg_ExtensionSystem_GenericSyncCall(
              op_name->value(), serialized_operation_data),
          &json_result);
  if (sync_call_result != PP_OK)
    return sync_call_result;

  base::StringPiece result_str(json_result);
  JSONStringValueDeserializer json_deserializer(result_str);
  std::unique_ptr<base::Value> result_ptr(
      json_deserializer.Deserialize(nullptr, nullptr));
  if (!result_ptr) {
    LOG(ERROR) << "JSON deserialization failed";
    return PP_ERROR_FAILED;
  }

  *operation_result = VarFromValue(result_ptr.get()).Release();
  return PP_OK;
}

}  // namespace proxy
}  // namespace ppapi
