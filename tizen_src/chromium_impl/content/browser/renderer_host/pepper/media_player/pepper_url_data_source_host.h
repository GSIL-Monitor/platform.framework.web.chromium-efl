// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_URL_DATA_SOURCE_HOST_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_URL_DATA_SOURCE_HOST_H_

#include <string>
#include "content/browser/renderer_host/pepper/media_player/pepper_media_data_source_host.h"
#include "content/browser/renderer_host/pepper/media_player/pepper_url_data_source_private.h"
#include "ppapi/host/resource_host.h"
#include "ppapi/proxy/serialized_var.h"

namespace content {

class PepperURLDataSourceHost : public PepperMediaDataSourceHost {
 public:
  PepperURLDataSourceHost(BrowserPpapiHost*,
                          PP_Instance,
                          PP_Resource,
                          const std::string& url);
  virtual ~PepperURLDataSourceHost();

  PepperURLDataSourcePrivate* MediaDataSourcePrivate() override;

 protected:
  int32_t OnResourceMessageReceived(const IPC::Message& msg,
                                    ppapi::host::HostMessageContext*) override;

 private:
  int32_t OnSetStreamingProperty(ppapi::host::HostMessageContext*,
                                 PP_StreamingProperty,
                                 std::string value);
  int32_t OnGetStreamingProperty(ppapi::host::HostMessageContext*,
                                 PP_StreamingProperty);

  void OnGetStreamingPropertyCompleted(ppapi::host::ReplyMessageContext,
                                       int32_t result,
                                       const std::string& data);

  scoped_refptr<PepperURLDataSourcePrivate> private_;

  DISALLOW_COPY_AND_ASSIGN(PepperURLDataSourceHost);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_URL_DATA_SOURCE_HOST_H_
