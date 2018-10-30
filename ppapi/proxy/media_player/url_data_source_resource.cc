// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/media_player/url_data_source_resource.h"

#include <string>
#include "base/logging.h"
#include "ppapi/proxy/plugin_dispatcher.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/proxy/resource_callback_helpers.h"

namespace ppapi {
namespace proxy {

URLDataSourceResource::URLDataSourceResource(Connection connection,
                                             PP_Instance instance,
                                             const std::string& file_path)
    : MediaDataSourceResource(connection, instance) {
  SendCreate(BROWSER, PpapiHostMsg_URLDataSource_Create(file_path));
}

URLDataSourceResource::~URLDataSourceResource() {}

thunk::PPB_URLDataSource_API* URLDataSourceResource::AsPPB_URLDataSource_API() {
  return this;
}

int32_t URLDataSourceResource::GetStreamingProperty(
    PP_StreamingProperty type,
    PP_Var* value,
    scoped_refptr<TrackedCallback> callback) {
  if (!value) {
    LOG(ERROR) << "Invalid PP_Var value";
    return PP_ERROR_BADARGUMENT;
  }
  Call<PpapiPluginMsg_URLDataSource_GetStreamingPropertyReply>(
      BROWSER, PpapiHostMsg_URLDataSource_GetStreamingProperty(type),
      base::Bind(&URLDataSourceResource::OnGetStreamingPropertyReply,
                 base::Unretained(this), callback, value));
  return PP_OK_COMPLETIONPENDING;
}

int32_t URLDataSourceResource::SetStreamingProperty(
    PP_StreamingProperty type,
    PP_Var value,
    scoped_refptr<TrackedCallback> callback) {
  StringVar* value_str = StringVar::FromPPVar(value);
  if (!value_str) {
    LOG(ERROR) << "Provided PP_Var is not a string";
    return PP_ERROR_BADARGUMENT;
  }

  Call<PpapiPluginMsg_URLDataSource_SetStreamingPropertyReply>(
      BROWSER,
      PpapiHostMsg_URLDataSource_SetStreamingProperty(type, value_str->value()),
      base::Bind(&OnReply, callback));
  return PP_OK_COMPLETIONPENDING;
}

void URLDataSourceResource::OnGetStreamingPropertyReply(
    scoped_refptr<TrackedCallback> callback,
    PP_Var* out,
    const ResourceMessageReplyParams& params,
    const std::string& value_str) {
  if (params.result() == PP_OK)
    *out = StringVar::StringToPPVar(value_str);
  Run(callback, params.result());
}

}  // namespace proxy
}  // namespace ppapi
