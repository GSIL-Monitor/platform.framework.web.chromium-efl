// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_ES_DATA_SOURCE_HOST_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_ES_DATA_SOURCE_HOST_H_

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/renderer_host/pepper/media_player/pepper_es_data_source_private.h"
#include "content/browser/renderer_host/pepper/media_player/pepper_media_data_source_host.h"
#include "ppapi/host/resource_host.h"

namespace content {

class BrowserPpapiHost;

class PepperAudioElementaryStreamPrivate;
class PepperVideoElementaryStreamPrivate;

class PepperESDataSourceHost
    : public PepperMediaDataSourceHost,
      public base::SupportsWeakPtr<PepperESDataSourceHost> {
 public:
  PepperESDataSourceHost(BrowserPpapiHost*, PP_Instance, PP_Resource);
  virtual ~PepperESDataSourceHost();

  PepperESDataSourcePrivate* MediaDataSourcePrivate() override;

 protected:
  int32_t OnResourceMessageReceived(const IPC::Message&,
                                    ppapi::host::HostMessageContext*) override;

 private:
  int32_t OnAddStream(ppapi::host::HostMessageContext*,
                      PP_ElementaryStream_Type_Samsung);
  int32_t OnSetDuration(ppapi::host::HostMessageContext*, PP_TimeDelta);
  int32_t OnSetEndOfStream(ppapi::host::HostMessageContext*);

  void OnAudioStreamAdded(ppapi::host::ReplyMessageContext,
                          int32_t result,
                          scoped_refptr<PepperAudioElementaryStreamPrivate>);
  void OnVideoStreamAdded(ppapi::host::ReplyMessageContext,
                          int32_t result,
                          scoped_refptr<PepperVideoElementaryStreamPrivate>);

  BrowserPpapiHost* host_;
  scoped_refptr<PepperESDataSourcePrivate> private_;

  DISALLOW_COPY_AND_ASSIGN(PepperESDataSourceHost);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_ES_DATA_SOURCE_HOST_H_
