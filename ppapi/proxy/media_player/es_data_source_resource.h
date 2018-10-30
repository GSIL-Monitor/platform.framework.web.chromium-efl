// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_MEDIA_PLAYER_ES_DATA_SOURCE_RESOURCE_H_
#define PPAPI_PROXY_MEDIA_PLAYER_ES_DATA_SOURCE_RESOURCE_H_

#include <memory>
#include "ppapi/proxy/media_player/elementary_stream_resource.h"
#include "ppapi/proxy/media_player/media_data_source_resource.h"
#include "ppapi/shared_impl/ppb_message_loop_shared.h"
#include "ppapi/thunk/ppb_media_data_source_api.h"

namespace ppapi {
namespace proxy {

class PluginResource;

/*
 * Implementation of plugin side Resource proxy for Elementary Stream Data
 * Source PPAPI,
 * see ppapi/api/samsung/ppb_media_data_source_samsung.idl
 * for full description of class and its methods.
 */
class ESDataSourceResource : public MediaDataSourceResource,
                             public thunk::PPB_ESDataSource_API {
 public:
  ESDataSourceResource(Connection, PP_Instance);
  virtual ~ESDataSourceResource();

  // PluginResource overrides.
  thunk::PPB_ESDataSource_API* AsPPB_ESDataSource_API() override;

  // PPBESDataSourceAPI implementation
  int32_t AddStream(PP_ElementaryStream_Type_Samsung,
                    const PPP_ElementaryStreamListener_Samsung_1_0*,
                    void* user_data,
                    PP_Resource*,
                    scoped_refptr<TrackedCallback>) override;
  int32_t SetDuration(PP_TimeDelta, scoped_refptr<TrackedCallback>) override;
  int32_t SetEndOfStream(scoped_refptr<TrackedCallback>) override;

 private:
  void OnAddStreamReply(const ResourceMessageReplyParams&,
                        PP_ElementaryStream_Type_Samsung,
                        int pending_host_id);

  struct AddStreamParams {
    AddStreamParams() : listener(nullptr), out_resource(0) {}

    std::unique_ptr<ElementaryStreamListener> listener;
    scoped_refptr<TrackedCallback> callback;
    PP_Resource* out_resource;
  };

  AddStreamParams add_stream_params_[PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_MAX];
  scoped_refptr<PluginResource> streams_[PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_MAX];
  scoped_refptr<TrackedCallback> set_duration_callback_;
  scoped_refptr<TrackedCallback> set_end_of_stream_callback_;

  DISALLOW_COPY_AND_ASSIGN(ESDataSourceResource);
};

}  // namespace proxy
}  // namespace ppapi

#endif  // PPAPI_PROXY_MEDIA_PLAYER_ES_DATA_SOURCE_RESOURCE_H_
