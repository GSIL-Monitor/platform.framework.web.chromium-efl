// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_MEDIA_PLAYER_URL_DATA_SOURCE_RESOURCE_H_
#define PPAPI_PROXY_MEDIA_PLAYER_URL_DATA_SOURCE_RESOURCE_H_

#include <string>
#include "ppapi/proxy/media_player/media_data_source_resource.h"
#include "ppapi/thunk/ppb_media_data_source_api.h"

namespace ppapi {
namespace proxy {

/*
 * Implementation of plugin side Resource proxy for URL Data Source PPAPI,
 * see ppapi/api/samsung/ppb_media_data_source_samsung.idl
 * for full description of class and its methods.
 */
class URLDataSourceResource : public MediaDataSourceResource,
                              public thunk::PPB_URLDataSource_API {
 public:
  URLDataSourceResource(Connection, PP_Instance, const std::string& file_path);
  virtual ~URLDataSourceResource();

  // PluginResource overrides.
  thunk::PPB_URLDataSource_API* AsPPB_URLDataSource_API() override;

  int32_t GetStreamingProperty(PP_StreamingProperty,
                               PP_Var* value,
                               scoped_refptr<TrackedCallback>) override;
  int32_t SetStreamingProperty(PP_StreamingProperty,
                               PP_Var value,
                               scoped_refptr<TrackedCallback>) override;

 private:
  void OnGetStreamingPropertyReply(scoped_refptr<TrackedCallback> callback,
                                   PP_Var* out,
                                   const ResourceMessageReplyParams& params,
                                   const std::string& value_str);
};

}  // namespace proxy
}  // namespace ppapi

#endif  // PPAPI_PROXY_MEDIA_PLAYER_URL_DATA_SOURCE_RESOURCE_H_
