// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/media_player/pepper_media_player_message_filter.h"

#include "components/nacl/browser/nacl_process_host.h"
#include "components/nacl/common/nacl_process_type.h"
#include "content/browser/ppapi_plugin_process_host.h"
#include "content/browser/renderer_host/pepper/media_player/pepper_media_player_host.h"
#include "ppapi/host/ppapi_host.h"
#include "ppapi/proxy/ppapi_messages.h"

namespace content {

namespace {

PepperMediaPlayerHost* GetPepperMediaPlayerHostFromBrowserHost(
    BrowserPpapiHost* host,
    PP_Instance instance,
    PP_Resource resource) {
  if (!host || !host->IsValidInstance(instance))
    return nullptr;
  auto resource_host = host->GetPpapiHost()->GetResourceHost(resource);
  if (resource_host && resource_host->IsMediaPlayerHost())
    return static_cast<PepperMediaPlayerHost*>(resource_host);
  return nullptr;
}

}  // anonymous namespace

PepperMediaPlayerMessageFilter::PepperMediaPlayerMessageFilter(
    int /* render_process_id */)
    : BrowserMessageFilter(PpapiMsgStart) {}

PepperMediaPlayerMessageFilter::~PepperMediaPlayerMessageFilter() {}

bool PepperMediaPlayerMessageFilter::OnMessageReceived(
    const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(PepperMediaPlayerMessageFilter, message)
  IPC_MESSAGE_HANDLER(PpapiHostMsg_MediaPlayer_OnGeometryChanged,
                      OnGeometryChanged)
  IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void PepperMediaPlayerMessageFilter::OnGeometryChanged(
    PP_Instance pp_instance,
    PP_Resource pp_resource,
    const PP_Rect& plugin_rect,
    const PP_Rect& clip_rect) {
  auto media_player_host = GetPepperMediaPlayerHost(pp_instance, pp_resource);
  if (media_player_host)
    media_player_host->UpdatePluginView(plugin_rect, clip_rect);
  else
    LOG(WARNING) << "Received a geometry update event for a non-existing "
                    "PepperMediaPlayerHost.";
}

PepperMediaPlayerHost* PepperMediaPlayerMessageFilter::GetPepperMediaPlayerHost(
    PP_Instance instance,
    PP_Resource resource) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  // check untrusted plugins
  for (BrowserChildProcessHostTypeIterator<nacl::NaClProcessHost> iter(
           PROCESS_TYPE_NACL_LOADER);
       !iter.Done(); ++iter) {
    if (auto media_player_host = GetPepperMediaPlayerHostFromBrowserHost(
            iter->browser_ppapi_host(), instance, resource))
      return media_player_host;
  }

  // check trusted plugins
  for (PpapiPluginProcessHostIterator iter; !iter.Done(); ++iter) {
    if (auto media_player_host = GetPepperMediaPlayerHostFromBrowserHost(
            iter->host_impl(), instance, resource))
      return media_player_host;
  }

  return nullptr;
}

}  // namespace content
