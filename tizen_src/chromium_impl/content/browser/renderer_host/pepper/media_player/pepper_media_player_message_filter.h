// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_MEDIA_PLAYER_MESSAGE_FILTER_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_MEDIA_PLAYER_MESSAGE_FILTER_H_

#include "content/public/browser/browser_message_filter.h"

#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_rect.h"
#include "ppapi/c/pp_resource.h"

namespace content {

class PepperMediaPlayerHost;

class PepperMediaPlayerMessageFilter : public BrowserMessageFilter {
 public:
  explicit PepperMediaPlayerMessageFilter(int render_process_id);
  virtual ~PepperMediaPlayerMessageFilter();

  bool OnMessageReceived(const IPC::Message& message) override;

 private:
  void OnGeometryChanged(PP_Instance,
                         PP_Resource,
                         const PP_Rect& plugin_rect,
                         const PP_Rect& clip_rect);

  PepperMediaPlayerHost* GetPepperMediaPlayerHost(PP_Instance, PP_Resource);

  DISALLOW_COPY_AND_ASSIGN(PepperMediaPlayerMessageFilter);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_MEDIA_PLAYER_MESSAGE_FILTER_H_
