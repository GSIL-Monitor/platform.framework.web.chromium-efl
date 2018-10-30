// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/javascript_interface/gin_native_bridge_message_filter.h"

#include "base/auto_reset.h"
#include "browser/javascript_interface/gin_native_bridge_dispatcher_host.h"
#include "common/gin_native_bridge_messages.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"

namespace {
const char kGinNativeBridgeMessageFilterKey[] = "GinNativeBridgeMessageFilter";
}  // namespace

namespace content {

GinNativeBridgeMessageFilter::GinNativeBridgeMessageFilter()
    : BrowserMessageFilter(GinNativeBridgeMsgStart),
      current_routing_id_(MSG_ROUTING_NONE) {}

GinNativeBridgeMessageFilter::~GinNativeBridgeMessageFilter() {}

void GinNativeBridgeMessageFilter::OnDestruct() const {
  if (BrowserThread::CurrentlyOn(BrowserThread::UI))
    delete this;
  else
    /* LCOV_EXCL_START */
    BrowserThread::DeleteSoon(BrowserThread::UI, FROM_HERE, this);
  /* LCOV_EXCL_STOP */
}

/* LCOV_EXCL_START */
bool GinNativeBridgeMessageFilter::OnMessageReceived(
    const IPC::Message& message) {
  base::AutoReset<int32_t> routing_id(&current_routing_id_,
                                      message.routing_id());
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(GinNativeBridgeMessageFilter, message)
    IPC_MESSAGE_HANDLER(GinNativeBridgeHostMsg_HasMethod, OnHasMethod)
    IPC_MESSAGE_HANDLER(GinNativeBridgeHostMsg_InvokeMethod, OnInvokeMethod)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

base::TaskRunner* GinNativeBridgeMessageFilter::OverrideTaskRunnerForMessage(
    const IPC::Message& message) {
  // As the filter is only invoked for the messages of the particular class,
  // we can return the task runner unconfitionally.
  return nullptr;
}
/* LCOV_EXCL_STOP */

void GinNativeBridgeMessageFilter::AddRoutingIdForHost(
    GinNativeBridgeDispatcherHost* host,
    RenderFrameHost* render_frame_host) {
  base::AutoLock locker(hosts_lock_);
  hosts_[render_frame_host->GetRoutingID()] = host;
}

void GinNativeBridgeMessageFilter::RemoveHost(
    GinNativeBridgeDispatcherHost* host) {
  base::AutoLock locker(hosts_lock_);
  auto iter = hosts_.begin();
  while (iter != hosts_.end()) {
    if (iter->second == host)
      hosts_.erase(iter++);
    else
      ++iter;  // LCOV_EXCL_LINE
  }
}

// static
scoped_refptr<GinNativeBridgeMessageFilter>
GinNativeBridgeMessageFilter::FromHost(GinNativeBridgeDispatcherHost* host,
                                       bool create_if_not_exists) {
  RenderProcessHost* rph = host->web_contents()->GetMainFrame()->GetProcess();
  scoped_refptr<GinNativeBridgeMessageFilter> filter =
      base::UserDataAdapter<GinNativeBridgeMessageFilter>::Get(
          rph, kGinNativeBridgeMessageFilterKey);
  if (!filter && create_if_not_exists) {
    filter = new GinNativeBridgeMessageFilter();
    rph->AddFilter(filter.get());
    rph->SetUserData(
        kGinNativeBridgeMessageFilterKey,
        base::WrapUnique(
            new base::UserDataAdapter<GinNativeBridgeMessageFilter>(
                filter.get())));
  }
  return filter;
}

/* LCOV_EXCL_START */
GinNativeBridgeDispatcherHost* GinNativeBridgeMessageFilter::FindHost() {
  base::AutoLock locker(hosts_lock_);
  auto iter = hosts_.find(current_routing_id_);
  if (iter != hosts_.end())
    return iter->second;
  // This is usually OK -- we can receive messages form RenderFrames for
  // which the corresponding host part has already been destroyed. That means,
  // any references to Native objects that the host was holding were already
  // released (with the death of ContentViewCore), so we can just drop such
  // messages.
  LOG(WARNING) << "WebView: Unknown frame routing id: "
               << current_routing_id_;  // LCOV_EXCL_LINE
  return nullptr;
}

void GinNativeBridgeMessageFilter::OnHasMethod(
    GinNativeBoundObject::ObjectID object_id,
    const std::string& method_name,
    bool* result) {
  GinNativeBridgeDispatcherHost* host = FindHost();
  if (host)
    host->OnHasMethod(object_id, method_name, result);
  else
    *result = false;
}

void GinNativeBridgeMessageFilter::OnInvokeMethod(
    GinNativeBoundObject::ObjectID object_id,
    const std::string& method_name,
    const base::ListValue& arguments,
    base::ListValue* wrapped_result,
    content::GinNativeBridgeError* error_code) {
  GinNativeBridgeDispatcherHost* host = FindHost();
  if (host) {
    host->OnInvokeMethod(current_routing_id_, object_id, method_name, arguments,
                         wrapped_result, error_code);
  } else {
    wrapped_result->Append(base::MakeUnique<base::Value>());
    *error_code = kGinNativeBridgeRenderFrameDeleted;
  }
}
/* LCOV_EXCL_STOP */

}  // namespace content
