// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_MEDIA_PLAYER_MEDIA_DATA_SOURCE_RESOURCE_H_
#define PPAPI_PROXY_MEDIA_PLAYER_MEDIA_DATA_SOURCE_RESOURCE_H_

#include "ppapi/proxy/plugin_resource.h"
#include "ppapi/thunk/ppb_media_data_source_api.h"

namespace ppapi {
namespace proxy {

/*
 * Implementation of plugin side Resource proxy for Media Data Source PPAPI,
 * see ppapi/api/samsung/ppb_media_data_source_samsung.idl
 * for full description of class and its methods.
 */
class MediaDataSourceResource : public PluginResource,
                                public thunk::PPB_MediaDataSource_API {
 public:
  virtual ~MediaDataSourceResource();

  // PluginResource overrides.
  thunk::PPB_MediaDataSource_API* AsPPB_MediaDataSource_API() override;

 protected:
  MediaDataSourceResource(Connection, PP_Instance);
};

}  // namespace proxy
}  // namespace ppapi

#endif  // PPAPI_PROXY_MEDIA_PLAYER_MEDIA_DATA_SOURCE_RESOURCE_H_
