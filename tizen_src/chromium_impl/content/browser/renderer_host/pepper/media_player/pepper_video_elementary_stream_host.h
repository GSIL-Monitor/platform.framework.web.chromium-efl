// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_VIDEO_ELEMENTARY_STREAM_HOST_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_VIDEO_ELEMENTARY_STREAM_HOST_H_

#include <memory>

#include "content/browser/renderer_host/pepper/media_player/pepper_elementary_stream_host.h"
#include "content/browser/renderer_host/pepper/media_player/pepper_video_elementary_stream_private.h"
#include "ppapi/host/resource_host.h"
#include "ppapi/shared_impl/media_player/media_codecs_configs.h"

namespace content {

class BrowserPpapiHost;

class PepperVideoElementaryStreamHost : public PepperElementaryStreamHost {
 public:
  static std::unique_ptr<PepperVideoElementaryStreamHost> Create(
      BrowserPpapiHost*,
      PP_Instance,
      PP_Resource,
      scoped_refptr<PepperVideoElementaryStreamPrivate>);
  virtual ~PepperVideoElementaryStreamHost();

 protected:
  int32_t OnResourceMessageReceived(const IPC::Message& msg,
                                    ppapi::host::HostMessageContext*) override;
  PepperVideoElementaryStreamPrivate* ElementaryStreamPrivate() override;

 private:
  PepperVideoElementaryStreamHost(
      BrowserPpapiHost*,
      PP_Instance,
      PP_Resource,
      scoped_refptr<PepperVideoElementaryStreamPrivate>);
  int32_t OnInitializeDone(ppapi::host::HostMessageContext*,
                           PP_StreamInitializationMode,
                           const ppapi::VideoCodecConfig&);

  scoped_refptr<PepperVideoElementaryStreamPrivate> private_;

  friend std::unique_ptr<PepperVideoElementaryStreamHost>
  std::make_unique<PepperVideoElementaryStreamHost>(
      content::BrowserPpapiHost*&,
      PP_Instance&,
      PP_Resource&,
      scoped_refptr<content::PepperVideoElementaryStreamPrivate>&);

  DISALLOW_COPY_AND_ASSIGN(PepperVideoElementaryStreamHost);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_VIDEO_ELEMENTARY_STREAM_HOST_H_
