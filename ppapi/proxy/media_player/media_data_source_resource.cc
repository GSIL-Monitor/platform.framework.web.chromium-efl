// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/media_player/media_data_source_resource.h"

namespace ppapi {
namespace proxy {

MediaDataSourceResource::MediaDataSourceResource(Connection connection,
                                                 PP_Instance instance)
    : PluginResource(connection, instance) {}

MediaDataSourceResource::~MediaDataSourceResource() {}

thunk::PPB_MediaDataSource_API*
MediaDataSourceResource::AsPPB_MediaDataSource_API() {
  return this;
}

}  // namespace proxy
}  // namespace ppapi
