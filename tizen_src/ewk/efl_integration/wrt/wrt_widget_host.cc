// Copyright (c) 2014,2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "wrt_widget_host.h"

#include "base/lazy_instance.h"
#include "common/render_messages_ewk.h"
#include "content/public/browser/browser_message_filter.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/resource_request_info.h"
#include "ipc/ipc_message_macros.h"
#include "ipc_message_start_ewk.h"
#include "net/url_request/url_request.h"
#include "url/gurl.h"

namespace {
// TODO(z.kostrzewa) I would prefer not make it a singleton, check out
// if it can't be a member of ContentMainDelegateEfl (but keep the static
// getter, maybe?).
base::LazyInstance<std::unique_ptr<WrtWidgetHost>>::Leaky g_wrt_widget_host =
    LAZY_INSTANCE_INITIALIZER;

bool SendToAllRenderers(IPC::Message* message) {
  bool result = false;
  content::RenderProcessHost::iterator it =
      content::RenderProcessHost::AllHostsIterator();
  while (!it.IsAtEnd()) {
    if (it.GetCurrentValue()->Send(new IPC::Message(*message)))
      result = true;
    it.Advance();
  }
  delete message;
  return result;
}

bool SendToRenderer(int renderer_id, IPC::Message* message) {
  return content::RenderProcessHost::FromID(renderer_id)->Send(message);
}
}

class WrtWidgetHostMessageFilter : public content::BrowserMessageFilter {
 public:
  explicit WrtWidgetHostMessageFilter(WrtWidgetHost* wrt_widget_host);

 private:
  bool OnMessageReceived(const IPC::Message& message) override;

  WrtWidgetHost* wrt_widget_host_;
};

WrtWidgetHostMessageFilter::WrtWidgetHostMessageFilter(
    WrtWidgetHost* wrt_widget_host)
    : content::BrowserMessageFilter(EwkMsgStart),
      wrt_widget_host_(wrt_widget_host) {
}

bool WrtWidgetHostMessageFilter::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(WrtWidgetHostMessageFilter, message)
    IPC_MESSAGE_FORWARD(WrtMsg_ParseUrlResponse, wrt_widget_host_, WrtWidgetHost::OnUrlRetrieved)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

WrtWidgetHost* WrtWidgetHost::Get() {
  // TODO(z.kostrzewa) LazyInstance is thread-safe but creating
  // WrtWidgetHost is not - make it thread-safe.
  if (!g_wrt_widget_host.Get().get())
    g_wrt_widget_host.Get().reset(new WrtWidgetHost);
  return g_wrt_widget_host.Get().get();
}

WrtWidgetHost::WrtWidgetHost()
    : message_filter_(new WrtWidgetHostMessageFilter(this)),
      enable_power_lock_(true) {}

void WrtWidgetHost::GetUrlForRequest(
    net::URLRequest* request,
    base::Callback<void(const GURL&)> callback) {
  // TODO(z.kostrzewa) Check on which thread(s) callbacks_ is touched
  // and provide synchronization if required (either via a lock or
  // by assuring that it is referenced only on one thread)
  int callback_id = callback_id_generator_.GetNext();
  callbacks_[callback_id] = callback;

  int renderer_id, frame_id;
  if (content::ResourceRequestInfo::GetRenderFrameForRequest(request, &renderer_id,
                                                             &frame_id))
    // TODO(is46.kim) Remove WrtMsg_ParseUrl. ParseUrl() can be invoked from
    // browser directly with IPC. It also looks like dead code.
    if (SendToRenderer(renderer_id, new WrtMsg_ParseUrl(callback_id, request->url())))
      return;

  callbacks_.erase(callback_id);
  callback.Run(GURL());
}

void WrtWidgetHost::SendWrtMessage(
    const Ewk_Wrt_Message_Data& message) {
  // webapi sends messages related to power lock.
  // When playing video, chromium has to check these messages
  // to decides to lock internally.
  if (!message.value.compare(0, 2, "__")) {
    if (!message.value.compare("__EnableChromiumInternalPowerLock")) {
      EnablePowerLock(true);
      return;
    } else if (!message.value.compare("__DisableChromiumInternalPowerLock")) {
      EnablePowerLock(false);
      return;
    }
  }

  SendToAllRenderers(new WrtMsg_SendWrtMessage(message));
}

// It's only used by the wrt_file_protocol_handler which is not going to be used in the future
// Candidate for deletion.
bool WrtWidgetHost::InWrt() const {
  return false;
}

// It's only used by the wrt_file_protocol_handler which is not going to be used in the future
// Candidate for deletion.
std::string WrtWidgetHost::TizenAppId() const {
  return std::string();
}

void WrtWidgetHost::OnUrlRetrieved(int callback_id, const GURL& url) {
  callbacks_type::iterator it = callbacks_.find(callback_id);
  if (callbacks_.end() == it)
    return;

  callbacks_type::mapped_type callback = it->second;
  callbacks_.erase(callback_id);
  callback.Run(url);
}

