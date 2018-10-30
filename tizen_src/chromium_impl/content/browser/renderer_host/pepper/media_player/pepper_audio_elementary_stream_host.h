// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_AUDIO_ELEMENTARY_STREAM_HOST_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_AUDIO_ELEMENTARY_STREAM_HOST_H_

#include <memory>

#include "base/macros.h"
#include "content/browser/renderer_host/pepper/browser_host_callback_helpers.h"
#include "content/browser/renderer_host/pepper/media_player/pepper_audio_elementary_stream_private.h"
#include "content/browser/renderer_host/pepper/media_player/pepper_elementary_stream_host.h"
#include "ppapi/host/host_message_context.h"
#include "ppapi/host/resource_host.h"
#include "ppapi/shared_impl/media_player/media_codecs_configs.h"

namespace content {

class BrowserPpapiHost;

class PepperAudioElementaryStreamHost : public PepperElementaryStreamHost {
 public:
  static std::unique_ptr<PepperAudioElementaryStreamHost> Create(
      BrowserPpapiHost* host,
      PP_Instance instance,
      PP_Resource resource,
      scoped_refptr<PepperAudioElementaryStreamPrivate> audio_private);

  virtual ~PepperAudioElementaryStreamHost();

 protected:
  int32_t OnResourceMessageReceived(const IPC::Message& msg,
                                    ppapi::host::HostMessageContext*) override;
  PepperAudioElementaryStreamPrivate* ElementaryStreamPrivate() override;

 private:
  PepperAudioElementaryStreamHost(
      BrowserPpapiHost*,
      PP_Instance,
      PP_Resource,
      scoped_refptr<PepperAudioElementaryStreamPrivate>);

  int32_t OnInitializeDone(ppapi::host::HostMessageContext*,
                           PP_StreamInitializationMode,
                           const ppapi::AudioCodecConfig&);

  scoped_refptr<PepperAudioElementaryStreamPrivate> private_;

  friend std::unique_ptr<PepperAudioElementaryStreamHost>
  std::make_unique<PepperAudioElementaryStreamHost>(
      content::BrowserPpapiHost*&,
      PP_Instance&,
      PP_Resource&,
      scoped_refptr<content::PepperAudioElementaryStreamPrivate>&);

  DISALLOW_COPY_AND_ASSIGN(PepperAudioElementaryStreamHost);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_AUDIO_ELEMENTARY_STREAM_HOST_H_
